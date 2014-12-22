/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <png.h>
#include <cstring>
#include <utility>
#include "load_png.hpp"
#include "log.hpp"


namespace easysplash
{


namespace
{


enum { png_signature_length = 8 };


// The custom I/O functions below for libpng are written to read from a datablock instance

struct data_read_state
{
	datablock const &m_data;
	std::size_t m_cur_pos;

	data_read_state(datablock const &p_data, std::size_t p_cur_pos)
		: m_data(p_data)
		, m_cur_pos(p_cur_pos)
	{
	}
};


void png_read_proc(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count_to_read)
{
	png_voidp io_ptr = png_get_io_ptr(png_ptr);

	if (io_ptr == NULL)
	{
		LOG_MSG(error, "io_ptr is NULL");
		return;
	}

	data_read_state *read_state = reinterpret_cast < data_read_state* > (io_ptr);

	// Read either the specified amount of bytes, or whatever remains in the buffer to read
	// (pick the smaller value)
	std::size_t num_to_read = std::min(std::size_t(byte_count_to_read), std::size_t(read_state->m_data.size() - read_state->m_cur_pos));

	std::memcpy(out_bytes, &(read_state->m_data[read_state->m_cur_pos]), num_to_read);
	read_state->m_cur_pos += num_to_read;
}


void png_error_msg_proc(png_structp, png_const_charp error_msg)
{
	LOG_MSG(error, "libpng error: " << error_msg);
}


void png_warning_msg_proc(png_structp, png_const_charp warning_msg)
{
	LOG_MSG(warning, "libpng warning: " << warning_msg);
}


} // unnamed namespace end


image load_png(datablock const &p_png_data)
{
	if (p_png_data.empty())
	{
		LOG_MSG(error, "no input PNG data");
		return image();
	}

	if (!png_check_sig((png_bytep)(&(p_png_data[0])), png_signature_length))
	{
		LOG_MSG(error, "invalid PNG signature");
		return image();
	}

	png_structp png_ptr = NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(png_ptr == NULL)
	{
		LOG_MSG(error, "could not initialize png struct");
		return image();
	}

	png_infop info_ptr = NULL;
	info_ptr = png_create_info_struct(png_ptr);

	if(info_ptr == NULL)
	{
		LOG_MSG(error, "could not initialize info struct");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return image();
	}

	data_read_state read_state(p_png_data, png_signature_length);

	png_set_error_fn(png_ptr, NULL, png_error_msg_proc, png_warning_msg_proc);
	png_set_read_fn(png_ptr, &read_state, png_read_proc);

	// Inform libpng that we already checked for the signature
	png_set_sig_bytes(png_ptr, png_signature_length);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int bit_depth = 0;
	int color_type = -1;
	png_uint_32 retval = png_get_IHDR(
		png_ptr, info_ptr,
		&width,
		&height,
		&bit_depth,
		&color_type,
		NULL, NULL, NULL
	);

	if (retval != 1)
	{
		LOG_MSG(error, "could not get IHDR");
		return image();
	}

	// Make sure the color data is in RGB format
	if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
	{
		png_set_gray_to_rgb(png_ptr);
		color_type = PNG_COLOR_TYPE_RGB;
	}
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png_ptr);
		color_type = PNG_COLOR_TYPE_RGB;
	}

	// Make sure the channels use 8 bit
	if (bit_depth > 8)
		png_set_strip_16(png_ptr);

	// Add an alpha channel if necessary, to ensure the data
	// is always of RGBA format
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

	// Allocate a 32 bpp RGBA image
	image img;
	img.m_width = width;
	img.m_height = height;
	img.m_stride = width * 4; // stride is specified in bytes, and 32 bpp = 4 bytes per pixel
	img.m_pixels.resize(img.m_stride * img.m_height);

	// Read the pixels into the image
	std::vector < png_bytep > row_pointers(height);
	for (std::size_t y = 0; y < height; ++y)
		row_pointers[y] = &(img.m_pixels[img.m_stride * y]);
	png_read_image(png_ptr, &(row_pointers[0]));

	// Cleanup
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	LOG_MSG(trace, "loaded PNG: " << width << "x" << height << " pixels");

	return img;
}


} // namespace easysplash end
