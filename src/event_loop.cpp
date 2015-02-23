/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <config.h>

#include <assert.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include "event_loop.hpp"
#include "log.hpp"


namespace easysplash
{


namespace
{


volatile sig_atomic_t sigint_fd = -1;
struct sigaction old_sigint_action;

void sigint_handler(int)
{
	if (sigint_fd != -1)
		write(sigint_fd, "1", 1);
}


} // unnamed namespace end


event_loop::event_loop(display &p_display, bool const p_non_realtime_mode)
	: m_display(p_display)
	, m_fifo_fd(-1)
	, m_non_realtime_mode(p_non_realtime_mode)
{
	// Only one instance of this class may exist. Check the SIGINT FD to verify this.
	assert(sigint_fd == -1);


	// Set up the SIGINT handler

	sigaction(SIGINT, NULL, &old_sigint_action);
	if (old_sigint_action.sa_handler != SIG_IGN)
	{
		m_sigint_fds[0] = m_sigint_fds[1] = -1;
		if (pipe(m_sigint_fds) == -1)
		{
			LOG_MSG(error, "Could not create SIGINT pipe: " << std::strerror(errno));
			return;
		}
		sigint_fd = m_sigint_fds[1];

		struct sigaction new_sigint_action;
		new_sigint_action.sa_handler = sigint_handler;
		new_sigint_action.sa_flags = 0;
		sigfillset(&(new_sigint_action.sa_mask));

		sigaction(SIGINT, &new_sigint_action, NULL);
	}


	// Create and open the FIFO

	remove(CTL_FIFO_PATH); // cleanup any leftover FIFO
	if (mkfifo(CTL_FIFO_PATH, 0666) == -1)
	{
		LOG_MSG(error, "Could not create FIFO \"" << CTL_FIFO_PATH << "\": " << std::strerror(errno));
		return;
	}

	// Using O_RDWR instead of O_RDONLY, since the latter will
	// cause poll() to return POLLHUP all the time after one
	// message was sent over the FIFO
	// O_NONBLOCK is necessary for poll() to work properly
	m_fifo_fd = open(CTL_FIFO_PATH, O_RDWR | O_NONBLOCK);
}


event_loop::~event_loop()
{
	if (m_fifo_fd != -1)
		close(m_fifo_fd);
	remove(CTL_FIFO_PATH);

	if (m_sigint_fds[0] != -1)
		close(m_sigint_fds[0]);
	if (m_sigint_fds[1] != -1)
		close(m_sigint_fds[1]);

	if (sigint_fd != -1)
	{
		sigaction(SIGINT, &old_sigint_action, NULL);
		sigint_fd = -1;
	}
}


void event_loop::run(animation const &p_animation)
{
	if (m_fifo_fd == -1)
		return;

	animation_position anim_pos;
	bool stop_loop_requested = false;

	// Calculate coordinates based on the output width specified
	// by the desc.txt file in the animation.
	// The coordinates display the frames in the center of the screen.
	long dx1 = (m_display.get_width() - p_animation.m_output_width) / 2;
	long dy1 = (m_display.get_height() - p_animation.m_output_height) / 2;
	long dx2 = dx1 + p_animation.m_output_width - 1;
	long dy2 = dy1 + p_animation.m_output_height - 1;

	// Setup pollfd structure to check for bytes to read from the FIFO fd
	struct pollfd fds[2];
	std::memset(fds, 0, sizeof(fds));
	int num_poll_fds = 0;

	fds[0].fd = m_fifo_fd;
	fds[0].events = POLLIN;
	++num_poll_fds;

	if (sigint_fd != -1)
	{
		fds[1].fd = m_sigint_fds[0];
		fds[1].events = POLLIN;
		++num_poll_fds;
	}

	// Defines how many milliseconds poll() shall wait for bytes to read.
	// Initially, this value is 0, causing poll() to return immediately.
	// This is desirable, since then, it will always paint the first frame
	// immediately, meaning the first frame will appear as soon as the
	// event loop starts. In non-realtime mode, however, set this to -1,
	// since then, no timeouts shall occur (animation advances only when
	// requested by a FIFO message).
	int wait_period = m_non_realtime_mode ? -1 : 0;

	// Define period of one frame
	std::chrono::steady_clock::duration const frame_dur = std::chrono::nanoseconds(1000000000) / p_animation.m_fps;

	// Frame drawing function
	auto draw_frame = [&](){
		display::image_handle cur_image_handle = get_image_handle_at(anim_pos, p_animation);
		if (cur_image_handle != display::invalid_image_handle)
		{
			m_display.draw_image(cur_image_handle, dx1, dy1, dx2, dy2);
			m_display.swap_buffers();
		}
	};

	unsigned int old_abs_frame_pos = 0;

	while (true)
	{
		// Check if the loop should stop. Stop if a stop was requested,
		// and either if anim_pos' part nr is the last one, or the part's
		// m_play_until_complete flag is false, or non-realtime mode is
		// requested.
		// Reason: part's with m_play_until_complete set to true must be
		// played until the end. But for the last part, it would mean
		// the animation could never be stopped, since the last part runs
		// in an infinite loop. Also, in non-realtime mode, the
		// m_play_until_complete flag makes no sense.
		if (stop_loop_requested && (m_non_realtime_mode || (anim_pos.m_part_nr >= (p_animation.m_parts.size() - 1)) || !p_animation.m_parts[anim_pos.m_part_nr].m_play_until_complete))
			break;

		int poll_ret = poll(fds, num_poll_fds, wait_period);

		if (poll_ret == 0)
		{
			// poll() timed out. Draw the next frame.

			// Draw frame & swap buffers to display the frame,
			// and measure the time that passed for each frame drawing
			std::chrono::steady_clock::time_point first_time_point = std::chrono::steady_clock::now();
			draw_frame();
			std::chrono::steady_clock::time_point second_time_point = std::chrono::steady_clock::now();

			// Calculate waiting period. It should equal the period for one frame
			// stored in frame_dur. Since the time spent drawing the frame is nonzero,
			// simply subtract that time from the frame_dur period, thus ensuring that
			// the frames are drawn in frame_dur intervals.
			std::chrono::steady_clock::duration dur = second_time_point - first_time_point;
			wait_period = std::chrono::duration_cast < std::chrono::milliseconds > (frame_dur - dur).count();

			// Special case - something caused a slowdown while drawing (usually
			// happens the first time it is called), so the time spent with
			// drawing was *larger* than frame_dur. Simply set wait_period to 0
			// in that case. It is not expected to occur often.
			if (wait_period < 0)
				wait_period = 0;

			// Move frame position forward by one frame.
			update_position(anim_pos, p_animation, 1);
		}
		else if ((fds[0].revents & POLLIN) != 0)
		{
			// poll() reported there are bytes to read. Do so.

			std::uint8_t progress;
			int read_ret = read(m_fifo_fd, &progress, sizeof(progress));
			if (read_ret == 0)
			{
				continue;
			}
			else if (read_ret == -1)
			{
				if (errno == EAGAIN)
					continue;
				else
				{
					LOG_MSG(error, "could not read from FIFO fd: " << std::strerror(errno));
					break;
				}
			}

			if (progress > 100) // More than 100% are not possible
				progress = 100;

			if (m_non_realtime_mode)
			{
				// In non-realtime mode, move to the frame that corresponds
				// to the percentage defined by "progress". Do so by calculating
				// by how many frames the animation advances.

				unsigned int abs_frame_pos = progress * (p_animation.m_total_num_frames - 1) / 100;
				if (abs_frame_pos > old_abs_frame_pos)
				{
					// Only handle forward advances (backwards
					// animations are not supported).
					update_position(anim_pos, p_animation, abs_frame_pos - old_abs_frame_pos);
					old_abs_frame_pos = abs_frame_pos;
					draw_frame();
				}
			}

			if (progress == 100)
				stop_loop_requested = true;
		}
		else if ((fds[1].revents & POLLIN) != 0)
		{
			// sigint FD got a message -> quit
			LOG_MSG(info, "SIGINT received - stopping event loop");
			break;
		}
	}
}


} // namespace easysplash end
