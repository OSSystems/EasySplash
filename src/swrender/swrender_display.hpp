/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_SWRENDER_DISPLAY_HPP
#define EASYSPLASH_SWRENDER_DISPLAY_HPP

#include <memory>
#include <set>
#include <pixman.h>
#include "display.hpp"
#include "noncopyable.hpp"
#include "linux_framebuffer.hpp"


namespace easysplash
{


class swrender_display
	: public display
{
public:
	struct sw_image
		: private noncopyable
	{
		pixman_image_t *m_pixman_image;
		std::unique_ptr < image > m_image;

		sw_image();
		sw_image(sw_image && p_other);
		~sw_image();

		sw_image& operator = (sw_image && p_other);

		bool load_image(image && p_image);

		bool operator < (sw_image const &p_other) const;
	};


	swrender_display();
	~swrender_display();

	virtual bool is_valid() const;

	virtual long get_width() const;
	virtual long get_height() const;

	virtual image_handle load_image(image &&p_image);
	virtual image_handle load_image(image const &p_image);
	virtual void unload_image(image_handle const p_image_handle);

	virtual void draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2);

	virtual void swap_buffers();


private:
	bool create_pixman_fb_image();

	typedef std::set < sw_image > pixman_images;
	pixman_images m_pixman_images;

	linux_framebuffer m_fbdev;
	pixman_image_t *m_pixman_fb_image;
};


} // namespace easysplash end


#endif
