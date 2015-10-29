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


namespace std
{


size_t hash < ::easysplash::animation_position > ::operator()(::easysplash::animation_position const & p_animation_position) const
{
	return std::hash < unsigned int > ()(p_animation_position.m_part_nr)
	     ^ std::hash < unsigned int > ()(p_animation_position.m_part_loop_nr)
	     ^ std::hash < unsigned int > ()(p_animation_position.m_frame_nr_in_part);
}


} // namespace std end


namespace easysplash
{


animation_position::animation_position()
	: m_part_nr(0)
	, m_part_loop_nr(0)
	, m_frame_nr_in_part(0)
{
}


animation::part::part()
	: m_play_until_complete(false)
{
}


animation::animation(zip_archive &p_zip_archive, display &p_display)
	: m_fps(0)
	, m_total_num_frames(0)
	, m_output_width(0)
	, m_output_height(0)
	, m_zip_archive(p_zip_archive)
	, m_display(p_display)
	, m_image_handle_cache(10, [&](animation_position const &p_key, display::image_handle & p_value) {
		m_display.unload_image(p_value);
		LOG_MSG(trace, "cache cleanup: unloading image at position " << p_key);
	})
{
	zip_archive::entry const *desc_entry = find_entry_by_name(p_zip_archive, "desc.txt");
	if (desc_entry == NULL)
	{
		LOG_MSG(error, "file desc.txt was not found in zip archive");
		return;
	}

	datablock desc_data = p_zip_archive.uncompress_data(*desc_entry);
	datablock_buf desc_readbuf(desc_data);
	std::istream desc_input(&desc_readbuf);

	std::string line;

	{
		std::getline(desc_input, line);
		std::stringstream sstr(line);
		sstr >> m_output_width >> m_output_height >> m_fps;
	}

	while (true)
	{
		std::getline(desc_input, line);
		if (!desc_input)
			break;

		part new_part;

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

			new_part.m_frames.push_back(&p_entry);
		});

		// As explained, in the header, m_total_num_frames sums up the number of all parts,
		// each number multiplied by the number of loops
		m_total_num_frames += new_part.m_frames.size() * new_part.m_num_times_to_play;
		m_parts.push_back(std::move(new_part));
	}
}


std::size_t animation::get_num_parts() const
{
	return m_parts.size();
}


animation::part const * animation::get_part(std::size_t const p_index) const
{
	return (p_index < m_parts.size()) ? &(m_parts[p_index]) : nullptr;
}


unsigned int animation::get_fps() const
{
	return m_fps;
}


unsigned int animation::get_total_num_frames() const
{
	return m_total_num_frames;
}


long animation::get_output_width() const
{
	return m_output_width;
}


long animation::get_output_height() const
{
	return m_output_height;
}


display::image_handle animation::get_image_handle_at(animation_position const &p_animation_position)
{
	// Check if the position is beyond the list of parts
	if (p_animation_position.m_part_nr >= m_parts.size())
	{
		LOG_MSG(warning, "animation position (" << p_animation_position << ") has invalid part nr");
		return display::invalid_image_handle;
	}

	animation::part const &part = m_parts[p_animation_position.m_part_nr];

	// Check if the position is beyond the list of frames inside the part
	if (p_animation_position.m_frame_nr_in_part >= part.m_frames.size())
	{
		LOG_MSG(warning, "animation position (" << p_animation_position << ") has invalid frame nr");
		return display::invalid_image_handle;
	}

	display::image_handle * cached_handle = m_image_handle_cache.get_entry(p_animation_position);
	if (cached_handle == nullptr)
	{
		LOG_MSG(trace, "no frame at position " << p_animation_position << " in LRU cache - loading");

		// Get the zip archive entry for the current position
		zip_archive::entry const &p_entry = *(part.m_frames[p_animation_position.m_frame_nr_in_part]);

		image frame = load_png(m_zip_archive.uncompress_data(p_entry));
		if (!is_valid(frame))
		{
			LOG_MSG(error, "could not load PNG \"" << p_entry.m_filename << "\"");
			return display::invalid_image_handle;
		}

		// Create a display image_handle for each frame
		display::image_handle handle = m_display.load_image(std::move(frame));
		if (handle != display::invalid_image_handle)
		{
			LOG_MSG(trace, "loaded frame \"" << p_entry.m_filename << "\"");

			cached_handle = &(m_image_handle_cache.add_entry(p_animation_position, std::move(handle)));
		}
		else
		{
			LOG_MSG(error, "could not load frame \"" << p_entry.m_filename << "\" into display");
			return display::invalid_image_handle;
		}
	}
	else
	{
		LOG_MSG(trace, "retrieved frame at position " << p_animation_position << " from LRU cache");
	}

	return *cached_handle;
}




bool is_valid(animation const &p_animation)
{
	return (p_animation.get_fps() != 0)
	    && (p_animation.get_output_width() != 0)
	    && (p_animation.get_output_height() != 0)
	    && (p_animation.get_num_parts() != 0);
}


bool operator < (animation_position const &p_first, animation_position const &p_second)
{
	if (p_first.m_part_nr < p_second.m_part_nr)
		return true;
	else if (p_first.m_part_nr > p_second.m_part_nr)
		return false;

	if (p_first.m_part_loop_nr < p_second.m_part_loop_nr)
		return true;
	else if (p_first.m_part_loop_nr > p_second.m_part_loop_nr)
		return false;

	return p_first.m_frame_nr_in_part < p_second.m_frame_nr_in_part;
}


bool operator == (animation_position const &p_first, animation_position const &p_second)
{
	return (p_first.m_part_nr == p_second.m_part_nr)
	    && (p_first.m_part_loop_nr == p_second.m_part_loop_nr)
	    && (p_first.m_frame_nr_in_part == p_second.m_frame_nr_in_part);
}


bool operator != (animation_position const &p_first, animation_position const &p_second)
{
	return !(p_first == p_second);
}


std::ostream& operator << (std::ostream &p_out, animation_position const &p_animation_position)
{
	p_out << "[ part nr: " << p_animation_position.m_part_nr << "  relative frame nr in part: " << p_animation_position.m_frame_nr_in_part << " ]";
	return p_out;
}


void update_position(animation_position &p_animation_position, animation const &p_animation, unsigned int const p_increase)
{
	p_animation_position.m_frame_nr_in_part += p_increase;
	if (p_animation_position.m_part_nr >= p_animation.get_num_parts())
		p_animation_position.m_part_nr = p_animation.get_num_parts() - 1;

	// After m_frame_nr_in_part was incremented by p_increase, its value
	// may lie outside of the bounds of the part. This means the new position
	// is actually beyond the current part, so try moving to the next part,
	// and see if the m_frame_nr_in_part value there is inside the bounds. If
	// not, move to the next part again etc. until m_frame_nr_in_part fits in
	// the part's bounds, or until the last part is reached.
	while (true)
	{
		animation::part const &part = *(p_animation.get_part(p_animation_position.m_part_nr));

		// If m_frame_nr_in_part lies within the bounds of the current part,
		// exit, since we are done.
		if (p_animation_position.m_frame_nr_in_part < part.m_frames.size())
			break;

		// If the current part is the last one, simply do a modulo on
		// m_frame_nr_in_part, thereby looping inside the last part.
		if (p_animation_position.m_part_nr == (p_animation.get_num_parts() - 1))
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


} // namespace easysplash end
