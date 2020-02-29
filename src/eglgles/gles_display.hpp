/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_GLES_DISPLAY_HPP
#define EASYSPLASH_GLES_DISPLAY_HPP

#include <map>
#include "display.hpp"
#include "texture.hpp"
#include "opengl.hpp"
#include "egl_platform.hpp"


namespace easysplash
{


class gles_display
	: public display
{
public:
	gles_display();
	~gles_display();

	virtual bool is_valid() const;

	virtual long get_width() const;
	virtual long get_height() const;

	virtual image_handle load_image(image const &p_image);
	virtual void unload_image(image_handle const p_image_handle);

	virtual void draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2);

	virtual void swap_buffers();


private:
	egl_platform m_egl_platform;

	typedef std::map < GLuint, texture > textures;
	textures m_textures;

	bool m_is_valid;

	GLuint m_vertex_shader, m_fragment_shader, m_program;
	GLuint m_vertex_buffer;
	GLint m_tex_uloc, m_frame_rect_uloc;
	GLint m_position_aloc, m_texcoords_aloc;
};


} // namespace easysplash end


#endif
