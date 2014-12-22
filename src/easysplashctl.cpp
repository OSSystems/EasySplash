/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <config.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <string>
#include <iostream>


int main(int argc, char *argv[])
{
	// Get progress percentage from arguments

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [PROGRESS (0-100)]\n";
		return -1;
	}

	int progress_int = std::stoi(argv[1]);
	if ((progress_int < 0) || (progress_int > 100))
	{
		std::cerr << "Progress value " << progress_int << " is outside of the bounds 0-100\n";
		return -1;
	}


	// Send percentage (stored in one byte, which can hold the 0-100 range)
	// over the FIFO to EasySplash
	std::uint8_t progress = progress_int;

	int fifo_fd = open(CTL_FIFO_PATH, O_RDWR | O_NONBLOCK);
	if (fifo_fd == -1)
	{
		std::cerr << "Could not open EasySplash FIFO \"" << CTL_FIFO_PATH << "\": " << std::strerror(errno) << "\n";
		return -1;
	}

	int ret = write(fifo_fd, &progress, sizeof(progress));
	if (ret == -1)
		std::cerr << "Could not send progress to EasySplash: " << std::strerror(errno) << "\n";

	close(fifo_fd);

	return (ret == -1) ? -1 : 0;
}
