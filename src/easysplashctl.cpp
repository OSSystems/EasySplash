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
#include <sstream>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>

#define UNIX_SOCK_PATH "/tmp/easysplash"

int main(int argc, char *argv[])
{
    int fd;
    int len;
    long progress;
    struct sockaddr_un addr;

    progress = strtol(argv[1], NULL, 10);
    if (errno == EINVAL || errno == ERANGE || progress < 0 || progress > 100) {
        std::cout << "Invalid progress value" << std::endl;
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, UNIX_SOCK_PATH);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    len = strlen(addr.sun_path) + sizeof(addr.sun_family);

    if (connect(fd, (struct sockaddr *)&addr, len) == -1) {
        std::cout << "Could not connect to splash socket" << std::endl;
        return 1;
    }

    std::ostringstream ss;
    ss << progress;

    send(fd, ss.str().c_str(), ss.str().length(), 0);

    return 0;
}
