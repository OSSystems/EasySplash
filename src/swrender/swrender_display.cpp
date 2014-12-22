/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include "swrender_display.hpp"
#include "linux_framebuffer.hpp"
#include "log.hpp"


namespace easysplash
{


swrender_display::sw_image::sw_image()
	: m_pixman_image(NULL)
{
}


swrender_display::sw_image::sw_image(sw_image && p_other)
	: m_pixman_image(p_other.m_pixman_image)
{
	std::swap(m_image, p_other.m_image);
	p_other.m_pixman_image = NULL;
}


swrender_display::sw_image::~sw_image()
{
	if (m_pixman_image != NULL)
		pixman_image_unref(m_pixman_image);
}


swrender_display::sw_image& swrender_display::sw_image::operator = (sw_image && p_other)
{
	std::swap(m_image, p_other.m_image);
	m_pixman_image = p_other.m_pixman_image;
	p_other.m_pixman_image = NULL;

	return *this;
}


bool swrender_display::sw_image::load_image(image && p_image)
{
	m_image = std::unique_ptr < image > (new image(std::move(p_image)));
	if (!easysplash::is_valid(*m_image))
	{
		LOG_MSG(error, "could not create pixman image: input image is invalid");
		return false;
	}

	m_pixman_image = pixman_image_create_bits(
		PIXMAN_a8b8g8r8,
		p_image.m_width,
		p_image.m_height,
		reinterpret_cast < uint32_t* > (&(m_image->m_pixels[0])),
		p_image.m_stride
	);

	if (m_pixman_image == NULL)
	{
		LOG_MSG(error, "could not create pixman image");
		return false;
	}

	pixman_image_set_filter(m_pixman_image, PIXMAN_FILTER_BILINEAR, NULL, 0);

	return true;
}


bool swrender_display::sw_image::operator < (sw_image const &p_other) const
{
	// Using pixman image pointers as keys, since they are easy to compare,
	// and uniquely identify the image (because there is exactly one pixman
	// image for each sw_image).
	return m_pixman_image < p_other.m_pixman_image;
}




swrender_display::swrender_display()
	: m_pixman_fb_image(NULL)
{
	enable_framebuffer_cursor(false);
	create_pixman_fb_image();
}


swrender_display::~swrender_display()
{
	enable_framebuffer_cursor(true);
	if (m_pixman_fb_image != NULL)
		pixman_image_unref(m_pixman_fb_image);
}


bool swrender_display::is_valid() const
{
	return m_fbdev.is_valid() && (m_pixman_fb_image != NULL);
}


long swrender_display::get_width() const
{
	return m_fbdev.get_width();
}


long swrender_display::get_height() const
{
	return m_fbdev.get_height();
}


display::image_handle swrender_display::load_image(image &&p_image)
{
	sw_image swimg;

	if (swimg.load_image(std::move(p_image)) == false)
		return display::invalid_image_handle;

	display::image_handle imghandle = display::image_handle(swimg.m_pixman_image);

	m_pixman_images.insert(std::move(swimg));

	return imghandle;
}


display::image_handle swrender_display::load_image(image const &p_image)
{
	image temp_image(p_image);
	return load_image(std::move(temp_image));
}


void swrender_display::unload_image(image_handle const p_image_handle)
{
	sw_image swimg;
	swimg.m_pixman_image = reinterpret_cast < pixman_image_t* > (p_image_handle);
	auto iter = m_pixman_images.find(swimg);
	swimg.m_pixman_image = nullptr;

	if (iter != m_pixman_images.end())
		m_pixman_images.erase(iter);
}


void swrender_display::draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2)
{
	pixman_image_t *srcimg = reinterpret_cast < pixman_image_t* > (p_image_handle);

	long draw_w = p_x2 - p_x1 + 1;
	long draw_h = p_y2 - p_y1 + 1;

	// Blit the image with pixman, with bilinear scaling
	// The scaling factors are calculated to make sure the scaled image fits
	// with the size of the drawing area

	pixman_fixed_t scale_x = pixman_image_get_width(srcimg) * pixman_int_to_fixed(1) / draw_w;
	pixman_fixed_t scale_y = pixman_image_get_height(srcimg) * pixman_int_to_fixed(1) / draw_h;

	pixman_transform_t transform;
	pixman_transform_init_scale(&transform, scale_x, scale_y);
	pixman_image_set_transform(srcimg, &transform);

	pixman_image_composite(
		PIXMAN_OP_SRC,
		srcimg,
		NULL,
		m_pixman_fb_image,
		0, 0,
		0, 0,
		p_x1, p_y1,
		draw_w, draw_h
	);
}


void swrender_display::swap_buffers()
{
}


bool swrender_display::create_pixman_fb_image()
{
	int type;
	linux_framebuffer::format const & fbfmt = m_fbdev.get_format();

	switch (fbfmt.m_channel_order)
	{
		case linux_framebuffer::channel_order_argb: type = PIXMAN_TYPE_ARGB; break;
		case linux_framebuffer::channel_order_rgba: type = PIXMAN_TYPE_RGBA; break;
	}

	pixman_format_code_t pxfmt = pixman_format_code_t(PIXMAN_FORMAT(
		fbfmt.m_bits_per_pixel,
		type,
		fbfmt.m_num_rgba_bits[3],
		fbfmt.m_num_rgba_bits[0],
		fbfmt.m_num_rgba_bits[1],
		fbfmt.m_num_rgba_bits[2]
	));

	m_pixman_fb_image = pixman_image_create_bits(
		pxfmt,
		m_fbdev.get_width(),
		m_fbdev.get_height(),
		reinterpret_cast < uint32_t* > (m_fbdev.get_pixels()),
		m_fbdev.get_stride()
	);

	if (m_pixman_fb_image == NULL)
	{
		LOG_MSG(error, "could not create pixman framebuffer image");
		return false;
	}
	else
		return true;
}


} // namespace easysplash end
