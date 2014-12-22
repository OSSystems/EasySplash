/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASH_SWRENDER_LINUX_FRAMEBUFFER_HPP
#define EASYSPLASH_SWRENDER_LINUX_FRAMEBUFFER_HPP

#include <cstdint>
#include <string>
#include "noncopyable.hpp"
#include "types.hpp"


namespace easysplash
{


/* linux_framebuffer class:
 *
 * Opens the framebuffer device and queries information about it.
 * This class is useful for displays which are based on the framebuffer, like G2D.
 * Only truecolor framebuffers are supported. If a framebuffer uses an unsupported
 * format, is_valid() returns false.
 * The framebuffer is also memory mapped. get_pixels() returns the mapped virtual
 * address, get_physical_address() the framebuffer's physical address. Note that
 * the framebuffer is mapped in write mode, so don't try to read from it.
 */
class linux_framebuffer
	: private noncopyable
{
public:
	enum channel_order
	{
		channel_order_argb,
		channel_order_rgba
	};

	struct format
	{
		channel_order m_channel_order;
		unsigned int m_num_rgba_bits[4];
		unsigned int m_bits_per_pixel, m_bytes_per_pixel;
	};


	explicit linux_framebuffer(std::string const &p_device_name = "/dev/fb0");
	~linux_framebuffer();

	bool is_valid() const;

	std::uint8_t* get_pixels();
	imx_phys_addr_t get_physical_address() const;
	unsigned long get_width() const;
	unsigned long get_height() const;
	unsigned long get_stride() const;
	format const & get_format() const;


private:
	int m_fd;
	std::uint8_t *m_pixels;
	imx_phys_addr_t m_physical_address;
	unsigned long m_buffer_length, m_width, m_height, m_stride;
	format m_format;
};


void enable_framebuffer_cursor(bool const p_state);
bool is_framebuffer_cursor_enabled();


} // namespace easysplash end


#endif
