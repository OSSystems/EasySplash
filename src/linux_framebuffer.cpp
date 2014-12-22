/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <cstring>
#include <cerrno>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "linux_framebuffer.hpp"
#include "log.hpp"


namespace easysplash
{


linux_framebuffer::linux_framebuffer(std::string const &p_device_name)
	: m_fd(-1)
	, m_pixels(NULL)
	, m_buffer_length(0)
	, m_width(0)
	, m_height(0)
	, m_stride(0)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	if ((m_fd = open(p_device_name.c_str(), O_RDWR)) < 0)
	{
		LOG_MSG(error, "could not open " << p_device_name << ": " << std::strerror(errno));
		return;
	}

	if (ioctl(m_fd, FBIOGET_FSCREENINFO, &finfo) == -1)
	{
		LOG_MSG(error, "could not open get fixed screen info: " << std::strerror(errno));
		return;
	}

	if (ioctl(m_fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
	{
		LOG_MSG(error, "could not open get variable screen info: " << std::strerror(errno));
		return;
	}

	if (finfo.type != FB_TYPE_PACKED_PIXELS)
	{
		LOG_MSG(error, "non-packed pixel formats are not supported");
		return;
	}

	if ((finfo.visual != FB_VISUAL_TRUECOLOR) || (vinfo.grayscale != 0))
	{
		LOG_MSG(error, "non-truecolor pixel formats are not supported");
		return;
	}

	if ((vinfo.red.msb_right != 0) || (vinfo.green.msb_right != 0) || (vinfo.blue.msb_right != 0))
	{
		LOG_MSG(error, "only formats with MSBs on the left are supported");
		return;
	}

	unsigned int rofs = vinfo.red.offset;
	unsigned int gofs = vinfo.green.offset;
	unsigned int bofs = vinfo.blue.offset;
	unsigned int aofs = vinfo.transp.offset;
	m_format.m_num_rgba_bits[0] = vinfo.red.length;
	m_format.m_num_rgba_bits[1] = vinfo.green.length;
	m_format.m_num_rgba_bits[2] = vinfo.blue.length;
	m_format.m_num_rgba_bits[3] = vinfo.transp.length;

	m_format.m_bits_per_pixel = vinfo.bits_per_pixel;
	
	switch (vinfo.bits_per_pixel)
	{
		case 15:
			m_format.m_bytes_per_pixel = 3; // 15 bpp formats use 1 bit as padding
			break;
		default:
			m_format.m_bytes_per_pixel = vinfo.bits_per_pixel / 8;
	}

	if (((aofs >= rofs) || (aofs == 0)) && (rofs >= gofs) && (gofs >= bofs))
		m_format.m_channel_order = channel_order_argb;
	else if ((rofs >= gofs) && (gofs >= bofs) && (bofs >= aofs))
		m_format.m_channel_order = channel_order_rgba;
	else
	{
		LOG_MSG(error,
			"unknown channel configuration:"
			"  R/G/B/A offsets: " << rofs << "/" << gofs << "/" << bofs << "/" << aofs << 
			"  R/G/B/A sizes:  " << m_format.m_num_rgba_bits[0] << "/" << m_format.m_num_rgba_bits[1] << "/" << m_format.m_num_rgba_bits[2] << "/" << m_format.m_num_rgba_bits[3]
		);
	}

	m_physical_address = imx_phys_addr_t(finfo.smem_start);
	m_buffer_length = finfo.smem_len;
	m_width = vinfo.xres;
	m_height = vinfo.yres;
	m_stride = m_width * m_format.m_bytes_per_pixel; // stride value is in bytes, not pixels

	void *pixels;
	if ((pixels = mmap(NULL, m_buffer_length, PROT_WRITE, MAP_SHARED, m_fd, 0)) == MAP_FAILED)
	{
		LOG_MSG(error, "could not map the framebuffer: " << std::strerror(errno));
		m_pixels = NULL;
		return;
	}
	else
		m_pixels = reinterpret_cast < std::uint8_t* > (pixels);

	LOG_MSG(info,
		"Linux framebuffer:  size: " << m_width << "x" << m_height << "  buffer length: " << m_buffer_length << "  bytes per pixel: " << m_format.m_bytes_per_pixel <<
		"  R/G/B/A offsets: " << rofs << "/" << gofs << "/" << bofs << "/" << aofs << 
		"  R/G/B/A sizes:  " << m_format.m_num_rgba_bits[0] << "/" << m_format.m_num_rgba_bits[1] << "/" << m_format.m_num_rgba_bits[2] << "/" << m_format.m_num_rgba_bits[3]
	);
}


linux_framebuffer::~linux_framebuffer()
{
	if (m_fd >= 0)
	{
		if (munmap(m_pixels, m_buffer_length) < 0)
			LOG_MSG(error, "could not unmap the framebuffer: " << std::strerror(errno));
		close(m_fd);
	}
}


bool linux_framebuffer::is_valid() const
{
	return (m_pixels != NULL) && (m_width != 0) && (m_height != 0) && (m_stride != 0);
}


std::uint8_t* linux_framebuffer::get_pixels()
{
	return m_pixels;
}


imx_phys_addr_t linux_framebuffer::get_physical_address() const
{
	return m_physical_address;
}


unsigned long linux_framebuffer::get_width() const
{
	return m_width;
}


unsigned long linux_framebuffer::get_height() const
{
	return m_height;
}


unsigned long linux_framebuffer::get_stride() const
{
	return m_stride;
}


linux_framebuffer::format const & linux_framebuffer::get_format() const
{
	return m_format;
}



namespace
{

char const * fbcon_path = "/sys/class/graphics/fbcon/cursor_blink";

} // unnamed namespace end


void enable_framebuffer_cursor(bool const p_state)
{
	int fd = open(fbcon_path, O_WRONLY);
	write(fd, p_state ? "1" : "0", 1);
	close(fd);
}


bool is_framebuffer_cursor_enabled()
{
	char state;
	int fd = open(fbcon_path, O_RDONLY);
	if (fd == -1)
	{
		LOG_MSG(error, "could not open" << fbcon_path << ": " << std::strerror(errno));
		return false;
	}

	read(fd, &state, 1);
	close(fd);

	return (state == '1');
}


} // namespace easysplash end
