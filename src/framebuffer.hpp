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

#ifndef _FRAMEBUFFER_HPP
#define _FRAMEBUFFER_HPP

#include <string>

using std::string;

class Framebuffer
{
public:
    Framebuffer(const string &device = string("/dev/fb0"));
    ~Framebuffer();

    bool initialize();

    inline const string &device() { return _device; }
    inline int width() { return _width; }
    inline int height() { return _height; }
    inline char *data() { return _data; }

private:
    bool isCursorEnabled();
    void disableCursor();
    void enableCursor();

private:
    string _device;
    int _width;
    int _height;
    char *_data;
    bool _oldCursorState;
};

#endif
