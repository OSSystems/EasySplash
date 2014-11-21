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

#ifndef _EVENTLOOP_HPP
#define _EVENTLOOP_HPP

class EventLoopPrivate;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    void exec();

    inline void setExitPending(bool value) { _exitPending = value; }
    inline bool isExitPending() { return _exitPending; }

private:
    bool _exitPending;
};

#endif
