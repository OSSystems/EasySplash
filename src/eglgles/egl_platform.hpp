/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASH_EGL_PLATFORM_HPP
#define EASYSPLASH_EGL_PLATFORM_HPP

#include "opengl.hpp"


namespace easysplash
{


class egl_platform
{
public:
	egl_platform();
	~egl_platform();

	bool make_current();
	bool swap_buffers();

	long get_display_width() const;
	long get_display_height() const;

	bool is_valid() const
	{
		return m_is_valid;
	}


private:
	EGLNativeDisplayType m_native_display;
	EGLNativeWindowType m_native_window;
	EGLDisplay m_egl_display;
	EGLContext m_egl_context;
	EGLSurface m_egl_surface;
	bool m_is_valid;

	struct internal;
	internal *m_internal;
};


} // namespace easysplash end


#endif
