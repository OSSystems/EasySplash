/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <memory>
#include "gles_display.hpp"
#include "linux_framebuffer.hpp"
#include "log.hpp"
#include "gl_misc.hpp"


namespace easysplash
{


namespace
{


static char const * simple_vertex_shader =
	"attribute vec2 position; \n"
	"attribute vec2 texcoords; \n"
	"varying vec2 uv; \n"
	"uniform vec4 frame_rect; \n"
	"void main(void) \n"
	"{ \n"
	"	uv = texcoords; \n"
	"	gl_Position = vec4(position * frame_rect.xy + frame_rect.zw, 1.0, 1.0); \n"
	"} \n"
	;

static char const * simple_fragment_shader =
	"precision mediump float;\n"
	"varying vec2 uv; \n"
	"uniform sampler2D tex; \n"
	"void main(void) \n"
	"{ \n"
	"	vec4 texel = texture2D(tex, uv); \n"
	"	gl_FragColor = vec4(texel.rgb, 1.0); \n"
	"} \n"
	;


static GLfloat const vertex_data[] = {
	-1, -1,  0, 1,
	-1,  1,  0, 0,
	 1, -1,  1, 1,
	 1,  1,  1, 0,
};
static unsigned int const vertex_data_size = sizeof(GLfloat)*16;
static unsigned int const vertex_size = sizeof(GLfloat)*4;
static unsigned int const vertex_position_num = 2;
static unsigned int const vertex_position_offset = sizeof(GLfloat)*0;
static unsigned int const vertex_texcoords_num = 2;
static unsigned int const vertex_texcoords_offset = sizeof(GLfloat)*2;


bool build_shader(GLuint &p_shader, GLenum p_shader_type, char const *p_code)
{
	GLint compilation_status, info_log_length;
	char const *shader_type_name;

	switch (p_shader_type)
	{
		case GL_VERTEX_SHADER: shader_type_name = "vertex shader"; break;
		case GL_FRAGMENT_SHADER: shader_type_name = "fragment shader"; break;
		default:
			LOG_MSG(error, "unknown shader type 0x" << std::hex << p_shader_type << std::dec);
			return false;
	}

	glGetError(); /* clear out any existing error */

	p_shader = glCreateShader(p_shader_type);
	if (!gles_check_gl_error(shader_type_name, "glCreateShader"))
		return false;

	glShaderSource(p_shader, 1, &p_code, NULL);
	if (!gles_check_gl_error(shader_type_name, "glShaderSource"))
		return false;

	glCompileShader(p_shader);
	if (!gles_check_gl_error(shader_type_name, "glCompileShader"))
		return false;

	glGetShaderiv(p_shader, GL_COMPILE_STATUS, &compilation_status);
	if (compilation_status == GL_FALSE)
	{
		LOG_MSG(error, "compiling " << shader_type_name << " failed");
		glGetShaderiv(p_shader, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log;
		info_log.resize(info_log_length);
		glGetShaderInfoLog(p_shader, info_log_length, NULL, &(info_log[0]));
		LOG_MSG(debug, "compilation log:\n" << info_log);
		return false;
	}
	else
		LOG_MSG(debug, "successfully compiled " << shader_type_name);

	return true;
}


bool destroy_shader(GLuint &p_shader, GLenum p_shader_type)
{
	char const *shader_type_name;

	if (p_shader == 0)
		return true;

	switch (p_shader_type)
	{
		case GL_VERTEX_SHADER: shader_type_name = "vertex shader"; break;
		case GL_FRAGMENT_SHADER: shader_type_name = "fragment shader"; break;
		default:
			LOG_MSG(error, "unknown shader type 0x" << std::hex << p_shader_type << std::dec);
			return false;
	}

	glGetError(); /* clear out any existing error */

	glDeleteShader(p_shader);
	p_shader = 0;
	if (!gles_check_gl_error(shader_type_name, "glDeleteShader"))
		return false;

	return true;
}


bool link_program(GLuint &p_program, GLuint p_vertex_shader, GLuint p_fragment_shader)
{
	GLint link_status, info_log_length;

	glGetError(); /* clear out any existing error */

	p_program = glCreateProgram();
	if (!gles_check_gl_error("program", "glCreateProgram"))
		return false;

	glAttachShader(p_program, p_vertex_shader);
	if (!gles_check_gl_error("program vertex", "glAttachShader"))
		return false;

	glAttachShader(p_program, p_fragment_shader);
	if (!gles_check_gl_error("program fragment", "glAttachShader"))
		return false;

	glLinkProgram(p_program);
	if (!gles_check_gl_error("program", "glLinkProgram"))
		return false;

	glGetProgramiv(p_program, GL_LINK_STATUS, &link_status);
	if (link_status == GL_FALSE)
	{
		LOG_MSG(error, "linking program failed");
		glGetProgramiv(p_program, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log;
		info_log.resize(info_log_length);
		glGetProgramInfoLog(p_program, info_log_length, NULL, &(info_log[0]));
		LOG_MSG(debug, "linker log:\n" << info_log);
		return false;
	}
	else
		LOG_MSG(debug, "successfully linked program");

	glUseProgram(p_program);

	return true;
}


bool destroy_program(GLuint &p_program, GLuint p_vertex_shader, GLuint p_fragment_shader)
{
	if (p_program == 0)
		return true;

	glGetError(); /* clear out any existing error */

	glUseProgram(0);
	if (!gles_check_gl_error("program", "glUseProgram"))
		return false;

	glDetachShader(p_program, p_vertex_shader);
	if (!gles_check_gl_error("program vertex", "glDetachShader"))
		return false;

	glDetachShader(p_program, p_fragment_shader);
	if (!gles_check_gl_error("program fragment", "glDetachShader"))
		return false;

	glDeleteProgram(p_program);
	p_program = 0;
	if (!gles_check_gl_error("program", "glDeleteProgram"))
		return false;

	return true;
}


bool build_vertex_buffer(GLuint &p_vertex_buffer)
{
	glGetError(); /* clear out any existing error */

	glGenBuffers(1, &p_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, p_vertex_buffer);
	/* TODO: This has to be called twice, otherwise the vertex data gets corrupted after the first few
	 * rendered frames. Is this a Vivante driver bug? */
	glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STATIC_DRAW);
	if (!gles_check_gl_error("vertex buffer", "glBufferData"))
		return false;

	return true;
}


bool destroy_vertex_buffer(GLuint &p_vertex_buffer)
{
	glGetError(); /* clear out any existing error */

	if (p_vertex_buffer != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &p_vertex_buffer);
		p_vertex_buffer = 0;
	}

	return true;
}


} // unnamed namespace end




gles_display::gles_display()
	: m_is_valid(false)
	, m_vertex_shader(0)
	, m_fragment_shader(0)
	, m_program(0)
	, m_vertex_buffer(0)
{
	if (!m_egl_platform.is_valid())
		return;

	glViewport(0, 0, get_width(), get_height());

	if (!build_shader(m_vertex_shader, GL_VERTEX_SHADER, simple_vertex_shader))
		return;
	if (!build_shader(m_fragment_shader, GL_FRAGMENT_SHADER, simple_fragment_shader))
		return;
	if (!link_program(m_program, m_vertex_shader, m_fragment_shader))
		return;

	m_tex_uloc = glGetUniformLocation(m_program, "tex");
	m_frame_rect_uloc = glGetUniformLocation(m_program, "frame_rect");
	m_position_aloc = glGetAttribLocation(m_program, "position");
	m_texcoords_aloc = glGetAttribLocation(m_program, "texcoords");

	/* set texture unit value for tex uniform */
	glUniform1i(m_tex_uloc, 0);

	glUniform4f(m_frame_rect_uloc, 1.0f, 1.0f, 0.0f, 0.0f);

	/* build vertex and index buffer objects */
	if (!build_vertex_buffer(m_vertex_buffer))
		return;

	/* enable vertex attrib array and set up pointers */
	glEnableVertexAttribArray(m_position_aloc);
	if (!gles_check_gl_error("position vertex attrib", "glEnableVertexAttribArray"))
		return;
	glEnableVertexAttribArray(m_texcoords_aloc);
	if (!gles_check_gl_error("texcoords vertex attrib", "glEnableVertexAttribArray"))
		return;

	glVertexAttribPointer(m_position_aloc,  vertex_position_num, GL_FLOAT, GL_FALSE, vertex_size, (GLvoid const*)((uintptr_t)vertex_position_offset));
	if (!gles_check_gl_error("position vertex attrib", "glVertexAttribPointer"))
		return;
	glVertexAttribPointer(m_texcoords_aloc, vertex_texcoords_num, GL_FLOAT, GL_FALSE, vertex_size, (GLvoid const*)((uintptr_t)vertex_texcoords_offset));
	if (!gles_check_gl_error("texcoords vertex attrib", "glVertexAttribPointer"))
		return;

	enable_framebuffer_cursor(false);

	m_is_valid = true;
}


gles_display::~gles_display()
{
	enable_framebuffer_cursor(true);

	bool ret = true;

	glDisableVertexAttribArray(m_position_aloc);
	ret = gles_check_gl_error("position vertex attrib", "glDisableVertexAttribArray") && ret;
	glDisableVertexAttribArray(m_texcoords_aloc);
	ret = gles_check_gl_error("texcoords vertex attrib", "glDisableVertexAttribArray") && ret;
	/* destroy vertex and index buffer objects */
	ret = destroy_vertex_buffer(m_vertex_buffer) && ret;

	/* destroy shaders and program */
	ret = destroy_program(m_program, m_vertex_shader, m_fragment_shader) && ret;
	ret = destroy_shader(m_vertex_shader, GL_VERTEX_SHADER) && ret;
	ret = destroy_shader(m_fragment_shader, GL_FRAGMENT_SHADER) && ret;
}


bool gles_display::is_valid() const
{
	return m_is_valid && m_egl_platform.is_valid();
}


long gles_display::get_width() const
{
	return m_egl_platform.get_display_width();
}


long gles_display::get_height() const
{
	return m_egl_platform.get_display_height();
}


display::image_handle gles_display::load_image(image const &p_image)
{
	texture tex(create_texture_from(p_image));
	display::image_handle imghandle = display::image_handle(tex.get_name());
	m_textures[tex.get_name()] = std::move(tex);
	return imghandle;
}


void gles_display::unload_image(image_handle const p_image_handle)
{
	auto iter = m_textures.find(GLuint(p_image_handle));
	if (iter != m_textures.end())
		m_textures.erase(iter);
}


void gles_display::draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2)
{
	long draw_w = p_x2 - p_x1 + 1;
	long draw_h = p_y2 - p_y1 + 1;

	glUniform4f(
		m_frame_rect_uloc,
		float(draw_w) / get_width(), float(draw_h) / get_height(),
		float((p_x1 + p_x2 - get_width()) / 2) / get_width(), float((p_y1 + p_y2 - get_height()) / 2) / get_height()
	);

	glBindTexture(texture::target, GLuint(p_image_handle));
	if (!gles_check_gl_error("render", "glBindTexture"))
		return;

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	if (!gles_check_gl_error("render", "glDrawArrays"))
		return;
}


void gles_display::swap_buffers()
{
	m_egl_platform.swap_buffers();
}


} // namespace easysplash end
