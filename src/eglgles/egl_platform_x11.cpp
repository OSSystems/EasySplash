/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <cstring>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

#include "egl_platform.hpp"
#include "gl_misc.hpp"
#include "log.hpp"


namespace easysplash
{


struct egl_platform::internal
{
	Atom m_wm_delete_atom;
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

	Display *x11_display;


	// Initialize display

	{
		EGLint ver_major, ver_minor;

		x11_display = XOpenDisplay(NULL);
		if (x11_display == NULL)
		{
			LOG_MSG(error, "could not open X display");
			return;
		}

		m_native_display = EGLNativeDisplayType(x11_display);

		m_egl_display = eglGetDisplay(m_native_display);
		if (m_egl_display == EGL_NO_DISPLAY)
		{
			LOG_MSG(error, "eglGetDisplay failed: " << egl_get_last_error_string());
			XCloseDisplay(x11_display);
			return;
		}

		if (!eglInitialize(m_egl_display, &ver_major, &ver_minor))
		{
			LOG_MSG(error, "eglInitialize failed: " << egl_get_last_error_string());
			XCloseDisplay(x11_display);
			return;
		}

		LOG_MSG(info, "X11 EGL platform initialized, using EGL " << ver_major << "." << ver_minor);
	}


	// Initialize window & context

	{
		EGLint num_configs;
		EGLConfig config;
		Window x11_window;

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

		// Create X11 window

		{
			EGLint native_visual_id;
			XVisualInfo visual_info_template;
			XVisualInfo *visual_info;
			int num_matching_visuals;
			XSetWindowAttributes attr;
			int screen_num;
			Window root_window;
			int x_coord, y_coord;
			unsigned int chosen_width, chosen_height;

			LOG_MSG(debug, "creating new X11 window with EGL context");

			if (!eglGetConfigAttrib(m_egl_display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id))
			{
				LOG_MSG(error, "eglGetConfigAttrib failed: " << egl_get_last_error_string());
				return;
			}

			screen_num = DefaultScreen(x11_display);
			root_window = RootWindow(x11_display, screen_num);

			std::memset(&visual_info_template, 0, sizeof(visual_info_template));
			visual_info_template.visualid = native_visual_id;

			visual_info = XGetVisualInfo(x11_display, VisualIDMask, &visual_info_template, &num_matching_visuals);
			if (visual_info == nullptr)
			{
				LOG_MSG(error, "Could not get visual info for native visual ID " << native_visual_id);
				return;
			}

			std::memset(&attr, 0, sizeof(attr));
			attr.background_pixmap = None;
			attr.background_pixel  = BlackPixel(x11_display, screen_num);
			attr.border_pixmap     = CopyFromParent;
			attr.border_pixel      = BlackPixel(x11_display, screen_num);
			attr.backing_store     = NotUseful;
			attr.override_redirect = False;
			attr.cursor            = None;

			x_coord = 0;
			y_coord = 0;
			chosen_width = 1024;
			chosen_height = 768;

			x11_window = XCreateWindow(
				x11_display, root_window,
				x_coord,
				y_coord,
				chosen_width,
				chosen_height,
				0, visual_info->depth, InputOutput, visual_info->visual,
				CWBackPixel | CWColormap  | CWBorderPixel | CWBackingStore | CWOverrideRedirect,
				&attr
			);

			XFree(visual_info);

			m_native_window = EGLNativeWindowType(x11_window);

			m_internal->m_wm_delete_atom = XInternAtom(x11_display, "WM_DELETE_WINDOW", True);
			XSetWMProtocols(x11_display, x11_window, &(m_internal->m_wm_delete_atom), 1);

			XStoreName(x11_display, x11_window, "EGL window");

			XSizeHints sizehints;
			sizehints.x = 0;
			sizehints.y = 0;
			sizehints.width = sizehints.min_width = sizehints.max_width = chosen_width;
			sizehints.height = sizehints.min_height = sizehints.max_height = chosen_height;
			sizehints.flags = PPosition | PSize;
			XSetWMNormalHints(x11_display, x11_window, &sizehints);

			XClearWindow(x11_display, x11_window);
			XMapRaised(x11_display, x11_window);

			XSync(x11_display, False);
		}

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

		{
			XWindowAttributes window_attr;
			XGetWindowAttributes(x11_display, x11_window, &window_attr);

			m_internal->m_display_width = window_attr.width;
			m_internal->m_display_height = window_attr.height;

			LOG_MSG(debug, "X11 window size is " << m_internal->m_display_width << "x" << m_internal->m_display_height);
		}

		glViewport(0, 0, m_internal->m_display_width, m_internal->m_display_height);
	}

	m_is_valid = true;
}


egl_platform::~egl_platform()
{
	if (m_egl_display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(m_egl_display);
	}

	if (m_native_display != NULL)
		XCloseDisplay(reinterpret_cast < Display* > (m_native_display));

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
