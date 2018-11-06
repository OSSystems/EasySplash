/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <bcm_host.h>
#include "scope_guard.hpp"
#include "egl_platform.hpp"
#include "gl_misc.hpp"
#include "log.hpp"


namespace easysplash
{


struct egl_platform::internal
{
	DISPMANX_DISPLAY_HANDLE_T m_dispman_display;

	internal()
		: m_dispman_display(DISPMANX_NO_HANDLE)
	{
		// Device 0 is the LCD
		m_dispman_display = vc_dispmanx_display_open(0);
		if (m_dispman_display == DISPMANX_NO_HANDLE)
		{
			LOG_MSG(error, "could not open dispmanx display");
			return;
		}

		uint32_t width, height;
		graphics_get_display_size(0/* LCD */, &width, &height);
		LOG_MSG(debug, "LCD display size: " << width << "x" << height << " pixels");

		VC_RECT_T dst_rect;
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = width;
		dst_rect.height = height;

		VC_RECT_T src_rect;
		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.width = width << 16;
		src_rect.height = height << 16;

		DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
		if (dispman_update == DISPMANX_NO_HANDLE)
		{
			LOG_MSG(error, "could not start dispmanx update");
			return;
		}

		DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
			dispman_update,
			m_dispman_display,
			0,
			&dst_rect,
			0,
			&src_rect,
			DISPMANX_PROTECTION_NONE,
			0,
			0,
			DISPMANX_NO_ROTATE
		);

		vc_dispmanx_update_submit_sync(dispman_update);

		if (dispman_element == DISPMANX_NO_HANDLE)
		{
			LOG_MSG(error, "could not add dispmanx element to update");
			return;
		}

		m_dispman_window.element = dispman_element;
		m_dispman_window.width = width;
		m_dispman_window.height = height;

		LOG_MSG(debug, "dispmanx window created successfully");
	}

	~internal()
	{
		if (m_dispman_display != DISPMANX_NO_HANDLE)
		{
			DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
			vc_dispmanx_element_remove(dispman_update, m_dispman_window.element);
			vc_dispmanx_update_submit_sync(dispman_update);
			vc_dispmanx_display_close(m_dispman_display);
		}
	}

	bool is_valid() const
	{
		return m_dispman_display != DISPMANX_NO_HANDLE;
	}

	EGLNativeWindowType get_egl_native_window()
	{
		return reinterpret_cast < EGLNativeWindowType > (&m_dispman_window);
	}

	EGL_DISPMANX_WINDOW_T m_dispman_window;
};


egl_platform::egl_platform()
	: m_native_display(EGL_DEFAULT_DISPLAY)
	, m_native_window(0)
	, m_egl_display(EGL_NO_DISPLAY)
	, m_egl_context(EGL_NO_CONTEXT)
	, m_egl_surface(EGL_NO_SURFACE)
	, m_is_valid(false)
{
	bcm_host_init();
	auto bcm_host_guard = make_scope_guard([&]() { bcm_host_deinit(); });

	m_internal = new internal;
	auto internal_guard = make_scope_guard([&]() { delete m_internal; });

	if (!m_internal->is_valid())
		return;

	// Initialize the EGL display
	{
		EGLint ver_major, ver_minor;

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

		LOG_MSG(info, "Broadcom Display manager service EGL platform initialized, using EGL " << ver_major << "." << ver_minor);
	}


	// Initialize window & context

	{
		EGLint num_configs;
		EGLConfig config;

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

		eglBindAPI(EGL_OPENGL_ES_API);

		m_egl_context = eglCreateContext(m_egl_display, config, EGL_NO_CONTEXT, ctx_attribs);
		if (m_egl_context == EGL_NO_CONTEXT)
		{
			LOG_MSG(error, "eglCreateContext failed: " << egl_get_last_error_string());
			return;
		}

		m_egl_surface = eglCreateWindowSurface(m_egl_display, config, m_internal->get_egl_native_window(), NULL);
		if (m_egl_surface == EGL_NO_SURFACE)
		{
			LOG_MSG(error, "eglCreateWindowSurface failed: " << egl_get_last_error_string());
			return;
		}

		if (!make_current())
			return;

		glViewport(0, 0, m_internal->m_dispman_window.width, m_internal->m_dispman_window.height);
	}

	internal_guard.dismiss();
	bcm_host_guard.dismiss();

	m_is_valid = true;
}


egl_platform::~egl_platform()
{
	if (m_egl_display != EGL_NO_DISPLAY)
	{
		LOG_MSG(debug, "Shutting down dispmanx display");
		eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(m_egl_display);
	}

	delete m_internal;

	bcm_host_deinit();
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
	return m_internal->m_dispman_window.width;
}


long egl_platform::get_display_height() const
{
	return m_internal->m_dispman_window.height;
}


} // namespace easysplash end
