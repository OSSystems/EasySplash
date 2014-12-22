/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <algorithm>
#include "types.hpp"


namespace easysplash
{


datablock_buf::datablock_buf(datablock const &p_datablock)
{
	char *p = (char *)(&(p_datablock[0]));
	setg(p, p, p + p_datablock.size());
}


image::image()
	: m_width(0)
	, m_height(0)
	, m_stride(0)
{
}


image::image(image &&p_other)
	: m_pixels(std::move(p_other.m_pixels))
	, m_width(p_other.m_width)
	, m_height(p_other.m_height)
	, m_stride(p_other.m_stride)
{
}


image::image(image const &p_other)
	: m_pixels(p_other.m_pixels)
	, m_width(p_other.m_width)
	, m_height(p_other.m_height)
	, m_stride(p_other.m_stride)
{
}


image& image::operator = (image &&p_other)
{
	std::swap(m_pixels, p_other.m_pixels);
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_stride = p_other.m_stride;
	return *this;
}


image& image::operator = (image const &p_other)
{
	m_pixels = p_other.m_pixels;
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_stride = p_other.m_stride;
	return *this;
}


bool is_valid(image const &p_image)
{
	return (p_image.m_width != 0) && (p_image.m_height != 0) && (p_image.m_stride != 0) && !(p_image.m_pixels.empty());
}


} // namespace easysplash end
