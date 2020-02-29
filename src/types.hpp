/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_TYPES_HPP
#define EASYSPLASH_TYPES_HPP

#include <vector>
#include <cstdint>
#include <streambuf>


namespace easysplash
{


typedef unsigned long imx_phys_addr_t;


typedef std::vector < std::uint8_t > datablock;


// Helper class to operate with iostreams inside a datablock
class datablock_buf
	: public std::streambuf
{
public:
	explicit datablock_buf(datablock const &p_datablock);
};


/* image:
 *
 * This is a simple structure for holding image data. It consists
 * of a datablock with the actual pixels, width & height values,
 * and a stride value. The stride value specifies how many bytes each
 * image row contains. The pixels are assumed to be 32-bit RGBA
 * formatted:.
 */
struct image
{
	datablock m_pixels;
	unsigned long m_width, m_height, m_stride;

	image();
	image(image &&p_other);
	image(image const &p_other);

	image& operator = (image &&p_other);
	image& operator = (image const &p_other);
};


bool is_valid(image const &p_image);


} // namespace easysplash end


#endif
