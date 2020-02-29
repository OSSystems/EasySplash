/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#include "gl_misc.hpp"
#include "log.hpp"


namespace easysplash
{


std::string egl_get_last_error_string(void)
{
	return egl_get_error_string(eglGetError());
}


std::string egl_get_error_string(EGLint err)
{
	if (err == EGL_SUCCESS)
		return "success";

	switch (err)
	{
		case EGL_NOT_INITIALIZED: return "not initialized";
		case EGL_BAD_ACCESS: return "bad access";
		case EGL_BAD_ALLOC: return "bad alloc";
		case EGL_BAD_ATTRIBUTE: return "bad attribute";
		case EGL_BAD_CONTEXT: return "bad context";
		case EGL_BAD_CONFIG: return "bad config";
		case EGL_BAD_CURRENT_SURFACE: return "bad current surface";
		case EGL_BAD_DISPLAY: return "bad display";
		case EGL_BAD_SURFACE: return "bad surface";
		case EGL_BAD_MATCH: return "bad match";
		case EGL_BAD_PARAMETER: return "bad parameter";
		case EGL_BAD_NATIVE_PIXMAP: return "bad native pixmap";
		case EGL_BAD_NATIVE_WINDOW: return "bad native window";
		case EGL_CONTEXT_LOST: return "context lost";
		default: return "<unknown error>";
	}
}


bool gles_check_gl_error(std::string const &p_category, std::string const &p_label)
{
	GLenum err = glGetError();
	if (err == GL_NO_ERROR)
		return true;

	switch (err)
	{
		case GL_INVALID_ENUM:                  LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: invalid enum"); break;
		case GL_INVALID_VALUE:                 LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: invalid value"); break;
		case GL_INVALID_OPERATION:             LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: invalid operation"); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: invalid framebuffer operation"); break;
		case GL_OUT_OF_MEMORY:                 LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: out of memory"); break;
		default:                               LOG_MSG(error, "[" << p_category << "] [" << p_label << "] error: unknown GL error 0x" << std::hex << err << std::dec); break;
	}

	return false;
}


} // namespace easysplash end
