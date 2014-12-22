/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <iomanip>
#include <iostream>
#include <assert.h>
#include "log.hpp"


namespace easysplash
{


namespace
{


void stderr_logfunc(std::chrono::steady_clock::duration const p_timestamp, log_levels const p_log_level, char const *p_srcfile, int const p_srcline, char const *p_srcfunction, std::string const &p_message)
{
	static std::chrono::steady_clock::duration::rep cur_max = 1000000;
	static int cur_num_digits = 6;

	auto ms = std::chrono::duration_cast < std::chrono::milliseconds > (p_timestamp).count();
	auto s = ms / 1000;
	if (s >= cur_max)
	{
		cur_max *= 1000;
		cur_num_digits += 3;
	}

	std::cerr
		<< "[" << std::dec << std::setfill (' ') << std::setw(cur_num_digits) << s << "." << std::setfill ('0') << std::setw(3) << (ms % 1000) << std::setfill(' ') << "] "
		<< "[" << log_level_str(p_log_level) << "] "
		<< "[" << p_srcfile << ":" << p_srcline << " " << p_srcfunction << "] "
		<< "  " << p_message << "\n";
}


struct logger_internal
{
	logger_internal()
		: m_logfunc(stderr_logfunc)
		, m_min_log_level(log_level_info)
	{
		m_time_base = std::chrono::steady_clock::now();
	}

	static logger_internal& instance()
	{
		static logger_internal logger;
		return logger;
	}

	log_write_function m_logfunc;
	log_levels m_min_log_level;
	std::chrono::steady_clock::time_point m_time_base;
};


}


char const * log_level_str(log_levels const p_log_level)
{
	switch (p_log_level)
	{
		case log_level_trace: return "trace";
		case log_level_debug: return "debug";
		case log_level_info: return "info";
		case log_level_warning: return "warning";
		case log_level_error: return "error";
	}

	return "<unknown>";
}


void set_stderr_output()
{
	logger_internal::instance().m_logfunc = stderr_logfunc;
}


void set_log_write_function(log_write_function const &p_function)
{
	logger_internal::instance().m_logfunc = p_function;
}

void log_message(log_levels const p_log_level, char const *p_srcfile, int const p_srcline, char const *p_srcfunction, std::string const &p_message)
{
	assert(logger_internal::instance().m_logfunc);
	logger_internal::instance().m_logfunc(
		std::chrono::steady_clock::now() - logger_internal::instance().m_time_base,
		p_log_level,
		p_srcfile, p_srcline, p_srcfunction,
		p_message
	);
}


void set_min_log_level(log_levels const p_min_log_level)
{
	logger_internal::instance().m_min_log_level = p_min_log_level;
}


log_levels get_min_log_level()
{
	return logger_internal::instance().m_min_log_level;
}


} // namespace easysplash end

