/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_TEXTURE_HPP
#define EASYSPLASH_TEXTURE_HPP

#include "opengl.hpp"
#include "noncopyable.hpp"
#include "types.hpp"


namespace easysplash
{


class texture:
	private noncopyable
{
public:
	enum { target = GL_TEXTURE_2D };

	texture();
	explicit texture(GLsizei const p_width, GLsizei const p_height, GLint const p_internal_format, GLenum const p_format, GLenum const p_type);
	texture(texture &&p_other);
	~texture();

	texture& operator = (texture &&p_other);

	GLsizei get_width() const;
	GLsizei get_height() const;
	GLuint get_name() const;


protected:
	GLsizei m_width, m_height;
	GLint m_internal_format;
	GLuint m_name;
};


bool is_valid(texture const &p_texture);
texture create_texture_from(image const &p_image);


} // namespace easysplash end


#endif
