/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#include "texture.hpp"
#include "log.hpp"


namespace easysplash
{


texture::texture()
	: m_width(0)
	, m_height(0)
	, m_name(0)
{
}


texture::texture(GLsizei const p_width, GLsizei const p_height, GLint const p_internal_format, GLenum const p_format, GLenum const p_type)
	: m_width(p_width)
	, m_height(p_height)
	, m_internal_format(p_internal_format)
{
	glGenTextures(1, &m_name);
	glBindTexture(target, m_name);
	glTexImage2D(target, 0, p_internal_format, p_width, p_height, 0, p_format, p_type, 0);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(target, 0);
}


texture::texture(texture &&p_other)
	: m_width(p_other.m_width)
	, m_height(p_other.m_height)
	, m_internal_format(p_other.m_internal_format)
	, m_name(p_other.m_name)
{
	p_other.m_name = 0;
}


texture::~texture()
{
	glBindTexture(target, 0);
	if (m_name != 0)
		glDeleteTextures(1, &m_name);
}


texture& texture::operator = (texture &&p_other)
{
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_internal_format = p_other.m_internal_format;
	m_name = p_other.m_name;
	p_other.m_name = 0;

	return *this;
}


GLsizei texture::get_width() const
{
	return m_width;
}


GLsizei texture::get_height() const
{
	return m_height;
}


GLuint texture::get_name() const
{
	return m_name;
}


bool is_valid(texture const &p_texture)
{
	return (p_texture.get_name() != 0);
}


texture create_texture_from(image const &p_image)
{
	if (!is_valid(p_image))
	{
		LOG_MSG(error, "could not create texture from invalid image");
		return texture();
	}

	texture tex(p_image.m_width, p_image.m_height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	if (!is_valid(tex))
	{
		LOG_MSG(error, "newly created texture is invalid");
		return texture();
	}

	glBindTexture(texture::target, tex.get_name());
	glTexSubImage2D(texture::target, 0, 0, 0, p_image.m_width, p_image.m_height, GL_RGBA, GL_UNSIGNED_BYTE, &(p_image.m_pixels[0]));
	glBindTexture(texture::target, 0);

	return tex;
}


} // namespace easysplash end
