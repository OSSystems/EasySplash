/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <cstring>
#include "g2d_display.hpp"
#include "linux_framebuffer.hpp"
#include "log.hpp"


namespace easysplash
{


#define ALIGN_VAL_TO(LENGTH, ALIGN_SIZE)  ( ((guintptr)((LENGTH) + (ALIGN_SIZE) - 1) / (ALIGN_SIZE)) * (ALIGN_SIZE) )


g2d_display::g2d_image::g2d_image::g2d_image()
	: m_g2d_buffer(nullptr)
{
}


g2d_display::g2d_image::g2d_image(g2d_image && p_other)
	: m_g2d_surface(p_other.m_g2d_surface)
	, m_g2d_buffer(p_other.m_g2d_buffer)
{
	p_other.m_g2d_buffer = nullptr;
}


g2d_display::g2d_image::~g2d_image()
{
	if (m_g2d_buffer != nullptr)
	{
		LOG_MSG(debug, "Shutting down G2D image");
		g2d_free(m_g2d_buffer);
	}
}


g2d_display::g2d_image& g2d_display::g2d_image::operator = (g2d_image && p_other)
{
	m_g2d_surface = p_other.m_g2d_surface;
	m_g2d_buffer = p_other.m_g2d_buffer;
	p_other.m_g2d_buffer = nullptr;
	return *this;
}


bool g2d_display::g2d_image::load_image(image const &p_image)
{
	// Need to align the stride to 16-byte boundaries for G2D
	unsigned long stride = ((p_image.m_stride + 15) / 16) * 16;
	m_g2d_buffer = g2d_alloc(stride * p_image.m_height, 0);

	if (m_g2d_buffer == nullptr)
	{
		LOG_MSG(error, "could not allocate buffer for G2D surface");
		return false;
	}

	for (unsigned long y = 0; y < p_image.m_height; ++y)
	{
		std::uint8_t const * srcrow = &(p_image.m_pixels[0]) + y * p_image.m_stride;
		std::uint8_t* destrow = reinterpret_cast < std::uint8_t* > (m_g2d_buffer->buf_vaddr) + y * stride;
		std::memcpy(destrow, srcrow, p_image.m_width * 4); // *4, since image is always 32-bit RGBA
	}

	m_g2d_surface.format = G2D_BGRA8888;
	m_g2d_surface.planes[0] = m_g2d_buffer->buf_paddr;
	m_g2d_surface.left = 0;
	m_g2d_surface.top = 0;
	m_g2d_surface.right = p_image.m_width;
	m_g2d_surface.bottom = p_image.m_height;
	m_g2d_surface.stride = stride / 4; // /4, since for some odd reason, stride is in pixels with G2D surfaces, and not in bytes
	m_g2d_surface.width = p_image.m_width;
	m_g2d_surface.height = p_image.m_height;
	m_g2d_surface.blendfunc = G2D_ONE;
	m_g2d_surface.global_alpha = 255;
	m_g2d_surface.clrcolor = 0x00000000;
	m_g2d_surface.rot = G2D_ROTATION_0;

	return true;
}


bool g2d_display::g2d_image::operator < (g2d_image const &p_other) const
{
	// The actual value of the buffer pointer is not relevant. The pointer
	// is just used as a key, since it is easy to compare, and uniquely
	// identifies the image (because there is exactly one buffer for each
	// G2D image).
	return m_g2d_buffer < p_other.m_g2d_buffer;
}




g2d_display::g2d_display()
	: m_g2d_handle(nullptr)
{
	m_g2d_fb_surface.planes[0] = 0;

	if (!create_g2d_fb_image())
		return;

	if (g2d_open(&m_g2d_handle) != 0)
	{
		LOG_MSG(error, "opening g2d device failed");
		return;
	}

	if (g2d_make_current(m_g2d_handle, G2D_HARDWARE_2D) != 0)
	{
		LOG_MSG(error, "g2d_make_current() failed");
		if (g2d_close(m_g2d_handle) != 0)
			LOG_MSG(error, "closing g2d device failed");
		return;
	}

	enable_framebuffer_cursor(false);
}


g2d_display::~g2d_display()
{
	enable_framebuffer_cursor(true);

	if (m_g2d_handle != nullptr)
	{
		if (g2d_close(m_g2d_handle) != 0)
			LOG_MSG(error, "closing g2d device failed");
	}
}


bool g2d_display::is_valid() const
{
	return m_fbdev.is_valid() && (m_g2d_handle != nullptr) && (m_g2d_fb_surface.planes[0] != 0);
}


long g2d_display::get_width() const
{
	return m_fbdev.get_width();
}


long g2d_display::get_height() const
{
	return m_fbdev.get_height();
}


display::image_handle g2d_display::load_image(image const &p_image)
{
	g2d_image g2dimg;

	if (g2dimg.load_image(p_image) == false)
		return display::invalid_image_handle;

	display::image_handle imghandle = display::image_handle(g2dimg.m_g2d_buffer);

	m_g2d_images.insert(std::move(g2dimg));

	return imghandle;
}


void g2d_display::unload_image(image_handle const p_image_handle)
{
	g2d_image g2dimg;
	g2dimg.m_g2d_buffer = reinterpret_cast < struct g2d_buf * > (p_image_handle);
	auto iter = m_g2d_images.find(g2dimg);
	g2dimg.m_g2d_buffer = nullptr;

	if (iter != m_g2d_images.end())
		m_g2d_images.erase(iter);
}


void g2d_display::draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2)
{
	m_g2d_fb_surface.left = p_x1;
	m_g2d_fb_surface.top = p_y1;
	m_g2d_fb_surface.right = p_x2 + 1;
	m_g2d_fb_surface.bottom = p_y2 + 1;

	g2d_image g2dimg;
	g2dimg.m_g2d_buffer = reinterpret_cast < struct g2d_buf * > (p_image_handle);
	auto iter = m_g2d_images.find(g2dimg);
	g2dimg.m_g2d_buffer = nullptr;

	if (iter == m_g2d_images.end())
	{
		LOG_MSG(error, "could not find image for given handle");
		return;
	}

	if (g2d_blit(m_g2d_handle, const_cast < g2d_surface * > (&(iter->m_g2d_surface)), &m_g2d_fb_surface) != 0)
		LOG_MSG(error, "blitting failed");

	if (g2d_finish(m_g2d_handle) != 0)
		LOG_MSG(error, "finishing g2d device operations failed");
}


void g2d_display::swap_buffers()
{
}


bool g2d_display::create_g2d_fb_image()
{
	linux_framebuffer::format const & fbfmt = m_fbdev.get_format();

	if ((fbfmt.m_num_rgba_bits[0] == 5) && (fbfmt.m_num_rgba_bits[1] == 6) && (fbfmt.m_num_rgba_bits[2] == 5))
		m_g2d_fb_surface.format = G2D_BGR565;
	else if ((fbfmt.m_num_rgba_bits[0] == 8) && (fbfmt.m_num_rgba_bits[1] == 8) && (fbfmt.m_num_rgba_bits[2] == 8) && (fbfmt.m_channel_order == linux_framebuffer::channel_order_argb))
	{
		if (fbfmt.m_num_rgba_bits[3] == 8)
			m_g2d_fb_surface.format = G2D_RGBA8888;
		else
			m_g2d_fb_surface.format = G2D_RGBX8888;
	}
	else
	{
		LOG_MSG(error, "framebuffer format is not supported by G2D display");
		return false;
	}

	m_g2d_fb_surface.planes[0] = int(m_fbdev.get_physical_address());
	m_g2d_fb_surface.left = 0;
	m_g2d_fb_surface.top = 0;
	m_g2d_fb_surface.right = m_fbdev.get_width();
	m_g2d_fb_surface.bottom = m_fbdev.get_height();
	// division is necessary, since for some reason, the
	// G2D API expects stride in pixels, not bytes
	m_g2d_fb_surface.stride = m_fbdev.get_stride() / m_fbdev.get_format().m_bytes_per_pixel;
	m_g2d_fb_surface.width = m_fbdev.get_width();
	m_g2d_fb_surface.height = m_fbdev.get_height();
	m_g2d_fb_surface.blendfunc = G2D_ZERO;
	m_g2d_fb_surface.global_alpha = 255;
	m_g2d_fb_surface.clrcolor = 0xFF000000;
	m_g2d_fb_surface.rot = G2D_ROTATION_0;

	return true;
}


} // namespace  end
