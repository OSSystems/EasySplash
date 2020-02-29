/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_EVENT_LOOP_HPP
#define EASYSPLASH_EVENT_LOOP_HPP

#include "display.hpp"
#include "animation.hpp"


namespace easysplash
{


/* event loop:
 *
 * This runs the main event loop that listens to the control FIFO,
 * and updates animations on screen.
 */
class event_loop
{
public:
	event_loop(display &p_display, bool const p_non_realtime_mode);
	~event_loop();

	void run(animation &p_animation);

private:
	display &m_display;
	int m_fifo_fd, m_sigint_fds[2];
	bool m_non_realtime_mode;
};


} // namespace easysplash end


#endif
