/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <config.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>


int main(int argc, char *argv[])
{
	// Get progress percentage from arguments

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [PROGRESS (0-100)] [--wait-until-finished]\n";
		return -1;
	}

	int progress_int = std::stoi(argv[1]);
	if ((progress_int < 0) || (progress_int > 100))
	{
		std::cerr << "Progress value " << progress_int << " is outside of the bounds 0-100\n";
		return -1;
	}

	bool wait_until_finished = false;
	if ((argc >= 3) && (std::string(argv[2]) == "--wait-until-finished") && (progress_int == 100))
	{
		wait_until_finished = true;
	}


	// Read the EasySplash PID
	// Do this here, _before_ the percentage is sent. This avoids theoretical
	// race conditions where the percentage is 100 and EasySplash terminates
	// before the PID could be read.
	int easysplash_pid = 0;
	if (wait_until_finished)
	{
		std::ifstream pidfile(EASYSPLASH_PID_FILE);
		if (!pidfile)
		{
			std::cerr << "Could not open EasySplash PID file " << EASYSPLASH_PID_FILE << " - cannot wait\n";
			return -1;
		}

		pidfile >> easysplash_pid;
		if (!pidfile)
		{
			std::cerr << "Could not read PID from EasySplash PID file " << EASYSPLASH_PID_FILE << " - cannot wait\n";
			return -1;
		}

		std::cerr << "Easysplash PID: " << easysplash_pid << "\n";
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


	// Now wait for EasySplash to finish if necessary
	if (wait_until_finished)
	{
		std::cerr << "100% reached, will wait until easysplash is finished\n";
		while (true)
		{
			// The common way of watching PIDs of non-child processes is to
			// periodically call kill(pid, 0). ESRCH is returned when the
			// watched process is gone.

			int ret = kill(easysplash_pid, 0);

			if (ret >= 0)
			{
				// Process exists. Wait 200ms before the next check.
				usleep(200 * 1000);
				continue;
			}
			else
			{
				// Return value -1 means either the process ended, or
				// something else happened. Output error if it isn't
				// the former.
				int err = errno;
				if (err == ESRCH)
				{
					std::cerr << "EasySplash process terminated successfully\n";
					break;
				}
				else
				{
					std::cerr << "Error while watching the PID " << easysplash_pid << ": " << std::strerror(err) << "\n";
					return -1;
				}
			}
		}
	}

	return (ret == -1) ? -1 : 0;
}
