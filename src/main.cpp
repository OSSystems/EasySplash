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

#include <iostream>

#include <stdio.h>

#include "eventloop.hpp"
#include "framebuffer.hpp"
#include "animation.hpp"

int main()
{
    Framebuffer fb;
    if (!fb.initialize()) {
        std::cout << "Failed to initialize framebuffer backend" << std::endl;
        return 1;
    }

    EventLoop *loop = new EventLoop;

    Animation animation(&fb, loop);
    animation.start();

    loop->exec();

    return 0;
}
