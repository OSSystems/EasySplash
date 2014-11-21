/*
 * EasySplash - Linux Bootsplash
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * This file is part of EasySplash.
 *
 * EasySplash is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * EasySplash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasySplash.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "animation.hpp"
#include "eventloop.hpp"
#include "framebuffer.hpp"

#include <iostream>
#include <list>
#include <string>
#include <algorithm>

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <zip.h>
#include <png.h>
#include <sys/param.h>

using std::string;
using std::list;

static pthread_t thread = 0;

static void *mainloop(void *data)
{
    Animation *animation = (Animation *) data;
    animation->exec();
    return NULL;
}

class Part;

class Frame
{
public:
    Framebuffer *fb;
    zip_int64_t index;
    char *data = NULL;
    int depth;
    Part *part;

    ~Frame() {
        delete data;
    }

    void load(FILE *fp)
    {
        png_structp png_ptr;
        png_infop info_ptr;
        int number_passes;

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        info_ptr = png_create_info_struct(png_ptr);

        png_init_io(png_ptr,fp);
        png_read_info(png_ptr, info_ptr);

        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE)
            png_set_expand(png_ptr);

        if (png_get_bit_depth(png_ptr, info_ptr) < 8)
            png_set_packing(png_ptr);

        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY || png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        if (png_get_bit_depth(png_ptr, info_ptr) == 16)
            png_set_strip_16(png_ptr);

        number_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr,info_ptr);

        depth = png_get_bit_depth(png_ptr, info_ptr);

        char *buffer = new char[png_get_image_width(png_ptr, info_ptr) * png_get_image_height(png_ptr, info_ptr) * depth];
        memset(buffer, 0, png_get_image_width(png_ptr, info_ptr) * png_get_image_height(png_ptr, info_ptr) * depth);

        int bytes = png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB_ALPHA ? 4 : 3;

        for (int pass = 0; pass < number_passes; pass++)
        {
            for(int i = 0; i < png_get_image_height(png_ptr, info_ptr); i++)
            {
                png_bytep row = (png_bytep) png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
                png_read_rows(png_ptr, &row, NULL, 1);

                for (int j = 0; j < png_get_image_width(png_ptr, info_ptr); j++) {
                    int location = (j * (32 / depth)) +  (i * (fb->width() * 4));

                    *(buffer + location + 2) = row[j * bytes + 0];
                    *(buffer + location + 1) = row[j * bytes + 1];
                    *(buffer + location + 0) = row[j * bytes + 2];
                    *(buffer + location + 3) = 0;
                }
            }
        }

        data = buffer;
    }
};

class Part
{
public:
    int count;
    int pause;
    string path;
    int playUntilComplete;
    list<Frame *> frames;
};

class AnimationPrivate
{
public:
    Framebuffer *fb;
    EventLoop *loop;
    struct zip *zip;
    int fps;
    int width;
    int height;
    list<Part *> parts;

    AnimationPrivate()
    {
        loop = NULL;
        zip = NULL;
        fps = 0;
    }
};

Animation::Animation(Framebuffer *fb, EventLoop *loop)
{
    d = new AnimationPrivate;
    d->fb = fb;
    d->loop = loop;

    if (access("/lib/splash/oem/bootanimation.zip", F_OK) != -1)
        d->zip = zip_open("/lib/splash/oem/bootanimation.zip", ZIP_CHECKCONS, NULL);
    else
        d->zip = zip_open("/lib/splash/bootanimation.zip", ZIP_CHECKCONS, NULL);

    if (!d->zip) {
        std::cout << "Failed to open animation file" << std::endl;
    } else {
        readDesc();
        readFrames();
    }
}

void Animation::start()
{
    if (thread == 0) {
        pthread_create(&thread, NULL, mainloop, (void *) this);
    } else {
        std::cout << "Can't start another animation thread" << std::endl;
        exit(1);
    }
}

void Animation::exec()
{
    struct animation *anim;
    struct animation_part *part;
    long frame_duration;
    time_t now;
    time_t past;

    frame_duration = (1 * 1000000) / d->fps;

    for (auto &part : d->parts) {
        for (auto &frame : part->frames) {
            FILE *fp;
            char *data;
            struct zip_file *zip;
            struct zip_stat *stat;

            stat = new struct zip_stat;

            zip = zip_fopen_index(d->zip, frame->index, 0);
            zip_stat_index(d->zip, frame->index, 0, stat);

            data = new char[stat->size];

            zip_fread(zip, data, stat->size);

            fp = fmemopen(data, stat->size, "r");

            frame->load(fp);

            free(data);
            free(stat);
            zip_fclose(zip);
        }
    }

    time(&past);

    for (auto &part : d->parts) {
        for (int i = 0; !part->count || i < part->count; i++) {
            if (d->loop->isExitPending() && !part->playUntilComplete)
                break;

            for (list<Frame *>::iterator it = part->frames.begin();
                 it != part->frames.end() && (!d->loop->isExitPending() || part->playUntilComplete); it++) {
                Frame *frame = *it;

                time(&now);

                int offsetX = ((d->fb->width() - d->width) / 2) * 4;
                int offsetY = ((d->fb->height() - d->height) / 2) * (d->fb->width() * 4);

                // draw
                memcpy(d->fb->data() + offsetX + offsetY, frame->data, d->width * d->height * frame->depth);

                time(&past);

                usleep(frame_duration - (now - past));
            }

            if (d->loop->isExitPending() && !part->count)
                break;
        }

        if (part->count != 1) {
            for (auto &frame : part->frames) {
                delete frame;
            }
        }
    }
}

bool Animation::readDesc()
{
    FILE *file;
    char *line;
    size_t line_len;
    char *data;
    struct zip_file *zip;
    struct zip_stat *stat;

    line = NULL;
    line_len = 0;
    zip = zip_fopen(d->zip, "desc.txt", 0);
    stat = new struct zip_stat;

    if (!zip) {
        return false;
    }

    zip_stat(d->zip, "desc.txt", 0, stat);

    data = new char[stat->size];

    zip_fread(zip, data, stat->size);

    file = fmemopen(data, stat->size, "r");

    while (getline(&line, &line_len, file) != -1) {
        char type;
        int count;
        int pause;
        char path[PATH_MAX];

        if (d->fps == 0) {
            sscanf(line, "%d %d %d", &d->width, &d->height, &d->fps);
        } else if (sscanf(line, "%c %d %d %s", &type, &count, &pause, path) == 4) {
            Part *part = new Part;
            part->count = count;
            part->pause = pause;
            part->path = path;
            part->playUntilComplete = (type == 'c');
            d->parts.push_back(part);
        }
    }

    fclose(file);
    free(data);
    free(zip);

    return d->fps != 0 && d->parts.size() != 0;
}

bool Animation::readFrames()
{
    struct zip_stat **zip_entries;
    zip_int64_t zip_num_entries;

    zip_num_entries = zip_get_num_entries(d->zip, 0);
    zip_entries = new struct zip_stat*[zip_num_entries];

    for (int zip_index = 0; zip_index < zip_num_entries; zip_index++) {
        struct zip_stat *stat = new struct zip_stat;

        zip_stat_index(d->zip, zip_index, 0, stat);
        zip_entries[zip_index] = stat;
    }

    std::sort(zip_entries, zip_entries + (zip_num_entries),
              [](const struct zip_stat *a, const struct zip_stat *b) -> bool {
                  return strcmp(a->name, b->name) < 0;
              }
    );

    for (auto &part : d->parts) {
        string path;

        path += string(part->path) + "/";

        for (int zip_index = 0; zip_index < zip_num_entries; zip_index++) {
            Frame *frame;
            const char *filename;

            filename = zip_entries[zip_index]->name;

            if (strncmp(path.c_str(), filename, path.length()) || path.length() == strlen(filename)) {
                continue;
            }

            frame = new Frame;
            frame->fb = d->fb;
            frame->index = zip_entries[zip_index]->index;
            frame->part = part;
            part->frames.push_back(frame);

            free(zip_entries[zip_index]);
        }
    }

    free(zip_entries);

    return true;
}
