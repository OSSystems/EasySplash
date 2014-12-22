/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include "animation.hpp"
#include "zip_archive.hpp"
#include "log.hpp"
#include "load_png.hpp"


namespace easysplash
{


animation::part::part()
	: m_play_until_complete(false)
{
}


animation::animation()
	: m_fps(0)
	, m_output_width(0)
	, m_output_height(0)
	, m_total_num_frames(0)
{
}


animation_position::animation_position()
	: m_part_nr(0)
	, m_part_loop_nr(0)
	, m_frame_nr_in_part(0)
{
}




bool is_valid(animation const &p_animation)
{
	return (p_animation.m_fps != 0) && (p_animation.m_output_width != 0) && (p_animation.m_output_height != 0) && !(p_animation.m_parts.empty());
}


std::ostream& operator << (std::ostream &p_out, animation_position const &p_animation_position)
{
	p_out << "part nr: " << p_animation_position.m_part_nr << "  relative frame nr in part: " << p_animation_position.m_frame_nr_in_part;
	return p_out;
}


animation load_animation(zip_archive &p_zip_archive, display &p_display)
{
	animation anim;

	zip_archive::entry const *desc_entry = find_entry_by_name(p_zip_archive, "desc.txt");
	if (desc_entry == NULL)
	{
		LOG_MSG(error, "file desc.txt was not found in zip archive");
		return animation();
	}

	datablock desc_data = p_zip_archive.uncompress_data(*desc_entry);
	datablock_buf desc_readbuf(desc_data);
	std::istream desc_input(&desc_readbuf);

	std::string line;

	{
		std::getline(desc_input, line);
		std::stringstream sstr(line);
		sstr >> anim.m_output_width >> anim.m_output_height >> anim.m_fps;
	}

	while (true)
	{
		std::getline(desc_input, line);
		if (!desc_input)
			break;

		animation::part new_part;

		std::string type, path;
		int pause; // dummy value, not currently used

		std::stringstream sstr(line);
		sstr >> type >> new_part.m_num_times_to_play >> pause >> path;
		if (path.empty() || type.empty())
		{
			LOG_MSG(error, "line \"" << line << "\" in desc.txt file is invalid");
			continue;
		}

		if (new_part.m_num_times_to_play == 0)
			new_part.m_num_times_to_play = 1;

		new_part.m_play_until_complete = (type == "p");

		// Find all entries which start with the given path, and execute the specified
		// function for eatch matching entry (the matching entries are passed to the function)
		p_zip_archive.find_entries_with_path(path, [&](zip_archive::entry const &p_entry) {
			if (p_entry.m_compressed_size == 0)
				return; // skip empty files and directories

			image frame = load_png(p_zip_archive.uncompress_data(p_entry));
			if (!is_valid(frame))
			{
				LOG_MSG(error, "could not load PNG \"" << p_entry.m_filename << "\"");
				return;
			}

			// Create a display image_handle for each frame
			display::image_handle handle = p_display.load_image(std::move(frame));
			if (handle != display::invalid_image_handle)
			{
				LOG_MSG(trace, "loaded frame \"" << p_entry.m_filename << "\"");
				new_part.m_frames.push_back(handle);
			}
			else
				LOG_MSG(error, "could not load frame \"" << p_entry.m_filename << "\" into display");
		});

		// As explained, in the header, m_total_num_frames sums up the number of all parts,
		// each number multiplied by the number of loops
		anim.m_total_num_frames += new_part.m_frames.size() * new_part.m_num_times_to_play;
		anim.m_parts.push_back(std::move(new_part));
	}

	return anim;
}


void update_position(animation_position &p_animation_position, animation const &p_animation, unsigned int const p_increase)
{
	p_animation_position.m_frame_nr_in_part += p_increase;

	// After m_frame_nr_in_part was incremented by p_increase, its value
	// may lie outside of the bounds of the part. This means the new position
	// is actually beyond the current part, so try moving to the next part,
	// and see if the m_frame_nr_in_part value there is inside the bounds. If
	// not, move to the next part again etc. until m_frame_nr_in_part fits in
	// the part's bounds, or until the last part is reached.
	while (true)
	{
		animation::part const &part = p_animation.m_parts[p_animation_position.m_part_nr];

		// If m_frame_nr_in_part lies within the bounds of the current part,
		// exit, since we are done.
		if (p_animation_position.m_frame_nr_in_part < part.m_frames.size())
			break;

		// If the current part is the last one, simply do a modulo on
		// m_frame_nr_in_part, thereby looping inside the last part.
		if (p_animation_position.m_part_nr == (p_animation.m_parts.size() - 1))
		{
			p_animation_position.m_frame_nr_in_part = p_animation_position.m_frame_nr_in_part % part.m_frames.size();
			break;
		}

		// If this point is reached, it means that (1) the current part is not
		// the last part in the animation and (2) m_frame_nr_in_part still lies
		// outside of the part's bounds. Increase the part's loop count, and
		// if the maximum number of loops is reached, advance to the next part.
		p_animation_position.m_part_loop_nr++;
		p_animation_position.m_frame_nr_in_part -= part.m_frames.size();
		if (p_animation_position.m_part_loop_nr >= part.m_num_times_to_play)
		{
			p_animation_position.m_part_loop_nr = 0;
			p_animation_position.m_part_nr++;
		}
	}
}


display::image_handle get_image_handle_at(animation_position const &p_animation_position, animation const &p_animation)
{
	if (p_animation_position.m_part_nr >= p_animation.m_parts.size())
	{
		LOG_MSG(warning, "animation position (" << p_animation_position << ") has invalid part nr");
		return display::invalid_image_handle;
	}

	animation::part const &part = p_animation.m_parts[p_animation_position.m_part_nr];

	if (p_animation_position.m_frame_nr_in_part >= part.m_frames.size())
	{
		LOG_MSG(warning, "animation position (" << p_animation_position << ") has invalid frame nr");
		return display::invalid_image_handle;
	}

	return part.m_frames[p_animation_position.m_frame_nr_in_part];
}


} // namespace easysplash end
