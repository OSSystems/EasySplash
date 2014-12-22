/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <config.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include "event_loop.hpp"
#include "animation.hpp"
#include "zip_archive.hpp"
#include "log.hpp"

#if defined(DISPLAY_TYPE_SWRENDER)
#include "swrender/swrender_display.hpp"
#else
#error Unknown display type selected
#endif


namespace
{


void print_usage(char const *p_program_name)
{
	char const *help_text =
		"  -h  --help             display this usage information and exit\n"
		"  -i  --zipfile          zipfile with animation to play\n"
		"  -v  --loglevel         minimum levels messages must have to be logged\n"
		"  -n  --non-realtime     Run in non-realtime mode (animation advances depending\n"
		"                         on the percentage value from the ctl application)\n"
		;

	std::cout << "Usage:  " << p_program_name << " [OPTION]...\n";
	std::cout << help_text;
}


} // unnamed namespace end


int main(int argc, char *argv[])
{
	bool non_realtime = false;
	char const * const short_options = "hi:l:v:n";
	struct option const long_options[] = {
		{ "help",         0, NULL, 'h' },
		{ "zipfile",      1, NULL, 'i' },
		{ "loglevel",     1, NULL, 'v' },
		{ "non-realtime", 0, NULL, 'n' },
		{ NULL,           0, NULL, 0   }
	};
	int next_option;
	std::string zipfilename, loglevel;

	while (true)
	{
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);

		if (next_option == -1)
			break;

		switch (next_option)
		{
			case 'h':
				print_usage(argv[0]);
				return 0;

			case 'i':
				zipfilename = optarg;
				break;

			case 'v':
				loglevel = optarg;
				break;

			case 'n':
				non_realtime = true;
				break;
		}
	}


	using namespace easysplash;


	if (loglevel == "trace")
		set_min_log_level(log_level_trace);
	else if (loglevel == "debug")
		set_min_log_level(log_level_debug);
	else if (loglevel == "info")
		set_min_log_level(log_level_info);
	else if (loglevel == "warning")
		set_min_log_level(log_level_trace);
	else if (loglevel == "error")
		set_min_log_level(log_level_error);


	std::ifstream in_zip_stream(zipfilename.c_str(), std::ios::binary);
	if (!in_zip_stream)
	{
		LOG_MSG(error, "could not load zip archive " << zipfilename);
		return -1;
	}

#if defined(DISPLAY_TYPE_SWRENDER)
	swrender_display display;
#endif

	if (!display.is_valid())
	{
		LOG_MSG(error, "initializing display failed");
		return -1;
	}

	LOG_MSG(info, "loading animation from zip archive " << zipfilename);
	zip_archive zip(in_zip_stream);
	animation anim = load_animation(zip, display);
	if (!is_valid(anim))
	{
		LOG_MSG(error, "could not load animation from zip archivei " << zipfilename);
		return -1;
	}

	event_loop evloop(display, anim, non_realtime);
	evloop.run();

	return 0;
}
