/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASH_ANIMATION_HPP
#define EASYSPLASH_ANIMATION_HPP

#include <cstddef>
#include <vector>
#include <iostream>
#include <functional>
#include <condition_variable>

#include "display.hpp"
#include "types.hpp"
#include "zip_archive.hpp"
#include "lru_cache.hpp"


namespace easysplash
{


/* animation_position structure:
 *
 * This is the current playback position in the frame. It is defined by:
 * m_part_nr: the number of the part it is currently in
 * m_part_loop_nr: what loop nr the current position is in (this can only be nonzero if
 *                 the part's m_num_times_to_play value is greater than 1)
 * m_frame_nr_in_part: the frame number inside this part (0 = first frame of this part)
 */

struct animation_position
{
	unsigned int m_part_nr, m_part_loop_nr;
	unsigned int m_frame_nr_in_part;

	animation_position();
};


}


namespace std
{


template < >
struct hash < ::easysplash::animation_position >
{
	size_t operator()(::easysplash::animation_position const & p_animation_position) const;
};


} // namespace std end


namespace easysplash
{


/* animation structure:
 *
 * The animation is defined by a framerate (fps), output width&height, and one or more parts.
 * the parts, fps and output width & height are read from the desc.txt file in the animation
 * zip archive.
 * output width & height defines the size of the rectangle the frames will be painted in.
 * The rectangle will also be drawn in the center of the screen.
 *
 * m_total_num_frames is calculated as the sum of all number of frames of each part, multiplied
 * by how often the corresponding part is looped. For example, if desc.txt defines three parts,
 * part 1 with 5 frames and 1 loop, part 2 with 11 frames and 3 loops, part 3 with 6 frames and
 * 2 loops, m_total_num_frames = 5*1 + 11*3 + 6*2 = 50.
 *
 * Parts are defined by a m_play_until_complete flag that indicates if the splash screen
 * can be stopped immediately or only after this part is done, a m_num_times_to_play value
 * which defines how often the part shall be played, and an array of zip archive entries which
 * contain the necessary information to load the individual frame.
 *
 * The last part has a special meaning in realtime mode. It will be repeated endlessly,
 * and the m_num_times_to_play value is ignored. m_play_until_complete is also ignored;
 * the animation will behave as if m_play_until_complete were false.
 */
class animation
{
public:
	struct part
	{
		bool m_play_until_complete;
		unsigned int m_num_times_to_play;
		std::vector < zip_archive::entry const * > m_frames;

		part();
	};

	explicit animation(zip_archive &p_zip_archive, display &p_display);

	std::size_t get_num_parts() const;
	part const * get_part(std::size_t const p_index) const;

	unsigned int get_fps() const;
	unsigned int get_total_num_frames() const;

	long get_output_width() const;
	long get_output_height() const;

	/* Retrieves the image handle for the frame at the given position. If no frame can be
	 * found, display::invalid_image_handle is returned.
	 */
	display::image_handle get_image_handle_at(animation_position const &p_animation_position);

private:
	animation(animation const &) = delete;
	animation& operator = (animation const &) = delete;

	std::vector < part > m_parts;
	unsigned int m_fps;
	unsigned int m_total_num_frames;
	long m_output_width, m_output_height;

	zip_archive &m_zip_archive;
	display &m_display;

	typedef lru_cache < animation_position, display::image_handle > image_handle_cache;
	image_handle_cache m_image_handle_cache;
};


bool operator < (animation_position const &p_first, animation_position const &p_second);
bool operator == (animation_position const &p_first, animation_position const &p_second);
bool operator != (animation_position const &p_first, animation_position const &p_second);
std::ostream& operator << (std::ostream &p_out, animation_position const &p_animation_position);


/* Checks if the animation is valid. An animation is valid if it has at least one part,
 * and the m_fps, m_output_width, m_output_height values are nonzero.
 */
bool is_valid(animation const &p_animation);

/* Loads the animation from a zip archive and allocates images in the display for each frame.
 * If loading fails, the returned animation object will be invalid. Check with is_valid() for that.
 */
animation load_animation(zip_archive &p_zip_archive, display &p_display);

/* Advances the position by calling update_position() and a given number of frames to
 * advance. If m_part_nr refers to the last part in the frame, and the positiion would
 * be advanced beyond the bounds of that part, it loops back, as if m_num_times_to_play
 * were set to some infinite value for the last part.
 */
void update_position(animation_position &p_animation_position, animation const &p_animation, unsigned int const p_increase);


} // namespace easysplash end


#endif
