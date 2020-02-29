/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_G2D_DISPLAY_HPP
#define EASYSPLASH_G2D_DISPLAY_HPP

#include <memory>
#include <set>
#include <g2d.h>
#include "display.hpp"
#include "noncopyable.hpp"
#include "linux_framebuffer.hpp"


namespace easysplash
{


class g2d_display
	: public display
{
public:
	struct g2d_image
		: private noncopyable
	{
		struct g2d_surface m_g2d_surface;
		struct g2d_buf *m_g2d_buffer;

		g2d_image();
		g2d_image(g2d_image && p_other);
		~g2d_image();

		g2d_image& operator = (g2d_image && p_other);

		bool load_image(image const &p_image);

		bool operator < (g2d_image const &p_other) const;
	};


	g2d_display();
	~g2d_display();

	virtual bool is_valid() const;

	virtual long get_width() const;
	virtual long get_height() const;

	virtual image_handle load_image(image const &p_image);
	virtual void unload_image(image_handle const p_image_handle);

	virtual void draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2);

	virtual void swap_buffers();


private:
	bool create_g2d_fb_image();

	typedef std::set < g2d_image > g2d_images;

	void* m_g2d_handle;
	g2d_images m_g2d_images;

	linux_framebuffer m_fbdev;
	struct g2d_surface m_g2d_fb_surface;
};


} // namespace easysplash end


#endif
