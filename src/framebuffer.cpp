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

#include "framebuffer.hpp"

#include <iostream>

#include <stdio.h>
#include <linux/fb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

Framebuffer::Framebuffer(const string &device)
{
    _device = device;
}

Framebuffer::~Framebuffer()
{
    if (_oldCursorState) {
        enableCursor();
    }
}

bool Framebuffer::initialize()
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd == -1) {
        std::cout << "Failed to open framebuffer device" << std::endl;
        return false;
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
        std::cout << "FBIOGET_FSCREENINFO failed" << std::endl;
        close(fd);
        return false;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        std::cout << "FBIOGET_VSCREENINFO failed" << std::endl;
        close(fd);
        return false;
    }

    if (!(vinfo.bits_per_pixel == 32 &&
          vinfo.red.offset == 16 && vinfo.red.length == 8 &&
          vinfo.green.offset == 8 && vinfo.green.length == 8 &&
          vinfo.blue.offset == 0 && vinfo.blue.length == 8)) {
        std::cout << "Only BGRA8888 is supported" << std::endl;
        close(fd);
        return false;
    }

    _width = vinfo.xres;
    _height = vinfo.yres;
    _data = (char *)mmap(0, vinfo.yres * finfo.line_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (_data == MAP_FAILED) {
        std::cout << "Cannot map frame buffer memory" << std::cout;
        close(fd);
        return false;
    }

    _oldCursorState = isCursorEnabled();

    if (_oldCursorState) {
        disableCursor();
    }

    close(fd);

    return true;
}

void Framebuffer::drawFrame(char *data, int offsetX, int offsetY, int len)
{
    memcpy(_data + offsetX + offsetY, data, len);
}

bool Framebuffer::isCursorEnabled()
{
    FILE *fp = fopen("/sys/class/graphics/fbcon/cursor_blink", "r");
    if (fp == NULL)
        return false;

    bool enabled = getc(fp) == '1';

    fclose(fp);

    return enabled;
}

void Framebuffer::disableCursor()
{
    int fd = open("/sys/class/graphics/fbcon/cursor_blink", O_WRONLY);
    write(fd, "0", 1);
    close(fd);
}

void Framebuffer::enableCursor()
{
    int fd = open("/sys/class/graphics/fbcon/cursor_blink", O_WRONLY);
    write(fd, "1", 1);
    close(fd);
}
