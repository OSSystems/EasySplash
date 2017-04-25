/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include "scope_guard.hpp"
#include "egl_platform.hpp"
#include "gl_misc.hpp"
#include "log.hpp"


namespace easysplash
{


struct egl_platform::internal
{
	long m_display_width, m_display_height;
};


egl_platform::egl_platform()
	: m_native_display(0)
	, m_native_window(0)
	, m_egl_display(EGL_NO_DISPLAY)
	, m_egl_context(EGL_NO_CONTEXT)
	, m_egl_surface(EGL_NO_SURFACE)
	, m_is_valid(false)
{
	m_internal = new internal;
	auto internal_guard = make_scope_guard([&]() { delete m_internal; });

	// Initialize display

	// Force the driver to configure the framebuffer for double buffering and vsync
	// unless the EASYSPLASH_NO_FB_MULTI_BUFFER environment variable is set
	if (getenv("EASYSPLASH_NO_FB_MULTI_BUFFER") == nullptr)
	{
		LOG_MSG(info, "Enabling double buffering and vsync");
		static std::string envvar("FB_MULTI_BUFFER=2");
		putenv(&(envvar[0]));
	}

	// Disable Vivante framebuffer clear
	if (getenv("EASYSPLASH_NO_DISABLE_CLEAR") == nullptr)
	{
		LOG_MSG(info, "Disabling framebuffer clear");
		static std::string envvar("GPU_VIV_DISABLE_CLEAR_FB=1");
		putenv(&(envvar[0]));
	}

	// Initialize the EGL display
	{
		EGLint ver_major, ver_minor;

		m_native_display = fbGetDisplayByIndex(0);

		m_egl_display = eglGetDisplay(m_native_display);
		if (m_egl_display == EGL_NO_DISPLAY)
		{
			LOG_MSG(error, "eglGetDisplay failed: " << egl_get_last_error_string());
			return;
		}

		if (!eglInitialize(m_egl_display, &ver_major, &ver_minor))
		{
			LOG_MSG(error, "eglInitialize failed: " << egl_get_last_error_string());
			return;
		}

		LOG_MSG(info, "Framebuffer EGL platform initialized, using EGL " << ver_major << "." << ver_minor);
	}


	// Initialize window & context

	{
		EGLint num_configs;
		EGLConfig config;
		int actual_x, actual_y, actual_width, actual_height;

		static EGLint const eglconfig_attribs[] =
		{
			EGL_RED_SIZE, 1,
			EGL_GREEN_SIZE, 1,
			EGL_BLUE_SIZE, 1,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		static EGLint const ctx_attribs[] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
		};

		if (!eglChooseConfig(m_egl_display, eglconfig_attribs, &config, 1, &num_configs))
		{
			LOG_MSG(error, "eglChooseConfig failed: " << egl_get_last_error_string());
			return;
		}

		m_native_window = fbCreateWindow(m_native_display, 0, 0, 0, 0);

		fbGetWindowGeometry(m_native_window, &actual_x, &actual_y, &actual_width, &actual_height);
		LOG_MSG(debug, "fbGetWindowGeometry: x/y " << actual_x << "/" << actual_y << " width/height " << actual_width << "/" << actual_height);

		eglBindAPI(EGL_OPENGL_ES_API);

		m_egl_context = eglCreateContext(m_egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
		if (m_egl_context == EGL_NO_CONTEXT)
		{
			LOG_MSG(error, "eglCreateContext failed: " << egl_get_last_error_string());
			return;
		}

		m_egl_surface = eglCreateWindowSurface(m_egl_display, config, m_native_window, NULL);
		if (m_egl_surface == EGL_NO_SURFACE)
		{
			LOG_MSG(error, "eglCreateWindowSurface failed: " << egl_get_last_error_string());
			return;
		}

		if (!make_current())
			return;

		m_internal->m_display_width = actual_width;
		m_internal->m_display_height = actual_height;

		glViewport(actual_x, actual_y, actual_width, actual_height);
	}

	internal_guard.dismiss();
	m_is_valid = true;
}


egl_platform::~egl_platform()
{
	if (m_egl_display != EGL_NO_DISPLAY)
	{
		LOG_MSG(debug, "Shutting down VIV FB EGL display");
		eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(m_egl_display);
	}

	delete m_internal;
}


bool egl_platform::make_current()
{
	if (!eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_egl_context))
	{
		LOG_MSG(error, "eglMakeCurrent failed: " << egl_get_last_error_string());
		return false;
	}
	else
		return true;
}


bool egl_platform::swap_buffers()
{
	if (!eglSwapBuffers(m_egl_display, m_egl_surface))
	{
		LOG_MSG(error, "eglSwapBuffers failed: " << egl_get_last_error_string());
		return false;
	}
	else
		return true;
}


long egl_platform::get_display_width() const
{
	return m_internal->m_display_width;
}


long egl_platform::get_display_height() const
{
	return m_internal->m_display_height;
}


} // namespace easysplash end
