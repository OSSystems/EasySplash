/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_LOG_HPP
#define EASYSPLASH_LOG_HPP

#include <sstream>
#include <string>
#include <functional>
#include <chrono>


namespace easysplash
{


enum log_levels
{
	log_level_trace = 0,
	log_level_debug,
	log_level_info,
	log_level_warning,
	log_level_error
};


char const * log_level_str(log_levels const p_log_level);


typedef std::function < void(std::chrono::steady_clock::duration const p_timestamp, log_levels const p_log_level, char const *p_srcfile, int const p_srcline, char const *p_srcfunction, std::string const &p_message) > log_write_function;


void set_stderr_output();
void set_log_write_function(log_write_function const &p_function);
void log_message(log_levels const p_log_level, char const *p_srcfile, int const p_srcline, char const *p_srcfunction, std::string const &p_message);

void set_min_log_level(log_levels const p_min_log_level);
log_levels get_min_log_level();



#define LOG_MSG(LEVEL, MSG) \
	do \
	{ \
		if (( ::easysplash::log_level_##LEVEL) >= ::easysplash::get_min_log_level()) \
		{ \
			std::stringstream log_msg_internal_sstr_813585712987; \
			log_msg_internal_sstr_813585712987 << MSG; \
			::easysplash::log_message(::easysplash::log_level_##LEVEL, __FILE__, __LINE__, __func__, log_msg_internal_sstr_813585712987.str()); \
		} \
	} \
	while (false)


} // namespace easysplash end


#endif

