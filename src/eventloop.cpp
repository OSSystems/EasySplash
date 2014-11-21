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

#include "eventloop.hpp"

#include <iostream>

#include <event.h>
#include <event2/listener.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNIX_SOCK_PATH "/tmp/easysplash"

static struct event_base *base;

static void read_cb(struct bufferevent *bev, void *ctx)
{
    EventLoop *loop;
    struct evbuffer *input;
    char *data;
    size_t len;

    loop = (EventLoop *) ctx;
    input = bufferevent_get_input(bev);
    data = new char[len];
    len = evbuffer_get_length(input);

    evbuffer_copyout(input, data, len);

    if (data == std::string("100")) {
        loop->setExitPending(true);
        event_base_loopbreak(base);
    }

    delete data;
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}
static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                           struct sockaddr *address, int socklen, void *ctx)
{
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, read_cb, NULL, NULL, ctx);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    int err;

    err = EVUTIL_SOCKET_ERROR();

    fprintf(stderr, "Got an error %d (%s) on the listener. ", err, evutil_socket_error_to_string(err));
}

EventLoop::EventLoop()
{
    struct evconnlistener *listener;
    struct sockaddr_un sin;

    _exitPending = false;

    base = event_base_new();
    if (!base) {
    }

    memset(&sin, 0, sizeof(sin));
    sin.sun_family = AF_LOCAL;
    strcpy(sin.sun_path, UNIX_SOCK_PATH);

    unlink(UNIX_SOCK_PATH);

    listener = evconnlistener_new_bind(base, accept_conn_cb, this,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                       (struct sockaddr *) &sin, sizeof(sin));
    if (!listener) {
        perror("Couldn't create listener");
    }

    evconnlistener_set_error_cb(listener, accept_error_cb);
}

EventLoop::~EventLoop()
{
}

void EventLoop::exec()
{
    event_base_dispatch(base);
}
