/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2017  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <cerrno>
#include <cstring>
#include <cstdint>
#include <string>
#include <sstream>
#include <libudev.h>
#include <gbm.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
/* NOTE: "xf86" is correct even though we don't use X */
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "scope_guard.hpp"
#include "egl_platform.hpp"
#include "gl_misc.hpp"
#include "log.hpp"


namespace easysplash
{


// TODO: CRTC ID 0 has been confirmed by several sources as a safe
// value for designating an invalid CRTC ID. However, no source that
// can be referenced has been found so far. Find one and add a link here.
static std::uint32_t const INVALID_CRTC = std::uint32_t(0);


static std::string get_name_for_drm_connector(drmModeConnector *connector)
{
	switch (connector->connector_type)
	{
		case DRM_MODE_CONNECTOR_Unknown: return "Unknown";
		case DRM_MODE_CONNECTOR_VGA: return "VGA";
		case DRM_MODE_CONNECTOR_DVII: return "DVI-I";
		case DRM_MODE_CONNECTOR_DVID: return "DVI-D";
		case DRM_MODE_CONNECTOR_DVIA: return "DVI-A";
		case DRM_MODE_CONNECTOR_Composite: return "Composite";
		case DRM_MODE_CONNECTOR_SVIDEO: return "S-Video";
		case DRM_MODE_CONNECTOR_LVDS: return "LVDS";
		case DRM_MODE_CONNECTOR_Component: return "Component";
		case DRM_MODE_CONNECTOR_9PinDIN: return "9-Pin DIN";
		case DRM_MODE_CONNECTOR_DisplayPort: return "DisplayPort";
		case DRM_MODE_CONNECTOR_HDMIA: return "HDMI-A";
		case DRM_MODE_CONNECTOR_HDMIB: return "HDMI-B";
		case DRM_MODE_CONNECTOR_TV: return "TV";
		case DRM_MODE_CONNECTOR_eDP: return "eDP";
		case DRM_MODE_CONNECTOR_VIRTUAL: return "Virtual";
		case DRM_MODE_CONNECTOR_DSI: return "DSI";
		default:
		{
			std::stringstream sstr;
			sstr << "< unknown connector type " << connector->connector_type << " >";
			return sstr.str();
		}
	}
}


static std::string get_name_for_drm_encoder(drmModeEncoder *encoder)
{
	switch (encoder->encoder_type)
	{
		case DRM_MODE_ENCODER_NONE: return "none";
		case DRM_MODE_ENCODER_DAC: return "DAC";
		case DRM_MODE_ENCODER_TMDS: return "TMDS";
		case DRM_MODE_ENCODER_LVDS: return "LVDS";
		case DRM_MODE_ENCODER_TVDAC: return "TVDAC";
		case DRM_MODE_ENCODER_VIRTUAL: return "Virtual";
		case DRM_MODE_ENCODER_DSI: return "DSI";
		default:
		{
			std::stringstream sstr;
			sstr << "< unknown encoder type " << encoder->encoder_type << " >";
			return sstr.str();
		}
	}
}


static std::string gbm_format_to_string(uint32_t format)
{
	if (format == GBM_BO_FORMAT_XRGB8888)
		format = GBM_FORMAT_XRGB8888;
	if (format == GBM_BO_FORMAT_ARGB8888)
		format = GBM_FORMAT_ARGB8888;

	switch(format)
	{
		case GBM_FORMAT_C8: return "C8";
		case GBM_FORMAT_RGB332: return "RGB332";
		case GBM_FORMAT_BGR233: return "BGR233";
		case GBM_FORMAT_NV12: return "NV12";
		case GBM_FORMAT_XRGB4444: return "XRGB4444";
		case GBM_FORMAT_XBGR4444: return "XBGR4444";
		case GBM_FORMAT_RGBX4444: return "RGBX4444";
		case GBM_FORMAT_BGRX4444: return "BGRX4444";
		case GBM_FORMAT_XRGB1555: return "XRGB1555";
		case GBM_FORMAT_XBGR1555: return "XBGR1555";
		case GBM_FORMAT_RGBX5551: return "RGBX5551";
		case GBM_FORMAT_BGRX5551: return "BGRX5551";
		case GBM_FORMAT_ARGB4444: return "ARGB4444";
		case GBM_FORMAT_ABGR4444: return "ABGR4444";
		case GBM_FORMAT_RGBA4444: return "RGBA4444";
		case GBM_FORMAT_BGRA4444: return "BGRA4444";
		case GBM_FORMAT_ARGB1555: return "ARGB1555";
		case GBM_FORMAT_ABGR1555: return "ABGR1555";
		case GBM_FORMAT_RGBA5551: return "RGBA5551";
		case GBM_FORMAT_BGRA5551: return "BGRA5551";
		case GBM_FORMAT_RGB565: return "RGB565";
		case GBM_FORMAT_BGR565: return "BGR565";
		case GBM_FORMAT_YUYV: return "YUYV";
		case GBM_FORMAT_YVYU: return "YVYU";
		case GBM_FORMAT_UYVY: return "UYVY";
		case GBM_FORMAT_VYUY: return "VYUY";
		case GBM_FORMAT_RGB888: return "RGB888";
		case GBM_FORMAT_BGR888: return "BGR888";
		case GBM_FORMAT_XRGB8888: return "XRGB8888";
		case GBM_FORMAT_XBGR8888: return "XBGR8888";
		case GBM_FORMAT_RGBX8888: return "RGBX8888";
		case GBM_FORMAT_BGRX8888: return "BGRX8888";
		case GBM_FORMAT_AYUV: return "AYUV";
		case GBM_FORMAT_XRGB2101010: return "XRGB2101010";
		case GBM_FORMAT_XBGR2101010: return "XBGR2101010";
		case GBM_FORMAT_RGBX1010102: return "RGBX1010102";
		case GBM_FORMAT_BGRX1010102: return "BGRX1010102";
		case GBM_FORMAT_ARGB8888: return "ARGB8888";
		case GBM_FORMAT_ABGR8888: return "ABGR8888";
		case GBM_FORMAT_RGBA8888: return "RGBA8888";
		case GBM_FORMAT_BGRA8888: return "BGRA8888";
		case GBM_FORMAT_ARGB2101010: return "ARGB2101010";
		case GBM_FORMAT_ABGR2101010: return "ABGR2101010";
		case GBM_FORMAT_RGBA1010102: return "RGBA1010102";
		case GBM_FORMAT_BGRA1010102: return "BGRA1010102";

		default:
		{
			std::stringstream sstr;
			sstr << "< unknown format " << format << " >";
			return sstr.str();
		}
	}

	return 0;
}


static int gbm_depth_from_format(uint32_t format)
{
	if (format == GBM_BO_FORMAT_XRGB8888)
		format = GBM_FORMAT_XRGB8888;
	if (format == GBM_BO_FORMAT_ARGB8888)
		format = GBM_FORMAT_ARGB8888;

	switch(format)
	{
		case GBM_FORMAT_C8:
		case GBM_FORMAT_RGB332:
		case GBM_FORMAT_BGR233:
			return 8;

		case GBM_FORMAT_NV12:
		case GBM_FORMAT_XRGB4444:
		case GBM_FORMAT_XBGR4444:
		case GBM_FORMAT_RGBX4444:
		case GBM_FORMAT_BGRX4444:
			return 12;

		case GBM_FORMAT_XRGB1555:
		case GBM_FORMAT_XBGR1555:
		case GBM_FORMAT_RGBX5551:
		case GBM_FORMAT_BGRX5551:
			return 15;

		case GBM_FORMAT_ARGB4444:
		case GBM_FORMAT_ABGR4444:
		case GBM_FORMAT_RGBA4444:
		case GBM_FORMAT_BGRA4444:
		case GBM_FORMAT_ARGB1555:
		case GBM_FORMAT_ABGR1555:
		case GBM_FORMAT_RGBA5551:
		case GBM_FORMAT_BGRA5551:
		case GBM_FORMAT_RGB565:
		case GBM_FORMAT_BGR565:
		case GBM_FORMAT_YUYV:
		case GBM_FORMAT_YVYU:
		case GBM_FORMAT_UYVY:
		case GBM_FORMAT_VYUY:
			return 16;

		case GBM_FORMAT_RGB888:
		case GBM_FORMAT_BGR888:
		case GBM_FORMAT_XRGB8888:
		case GBM_FORMAT_XBGR8888:
		case GBM_FORMAT_RGBX8888:
		case GBM_FORMAT_BGRX8888:
		case GBM_FORMAT_AYUV:
			return 24;

		case GBM_FORMAT_XRGB2101010:
		case GBM_FORMAT_XBGR2101010:
		case GBM_FORMAT_RGBX1010102:
		case GBM_FORMAT_BGRX1010102:
			return 30;

		case GBM_FORMAT_ARGB8888:
		case GBM_FORMAT_ABGR8888:
		case GBM_FORMAT_RGBA8888:
		case GBM_FORMAT_BGRA8888:
		case GBM_FORMAT_ARGB2101010:
		case GBM_FORMAT_ABGR2101010:
		case GBM_FORMAT_RGBA1010102:
		case GBM_FORMAT_BGRA1010102:
			return 32;

		default:
			LOG_MSG(error, "unknown GBM format " << format);
	}

	return 0;
}


static int gbm_bpp_from_format(uint32_t format)
{
	if (format == GBM_BO_FORMAT_XRGB8888)
		format = GBM_FORMAT_XRGB8888;
	if (format == GBM_BO_FORMAT_ARGB8888)
		format = GBM_FORMAT_ARGB8888;

	switch(format)
	{
		case GBM_FORMAT_C8:
		case GBM_FORMAT_RGB332:
		case GBM_FORMAT_BGR233:
			return 8;

		case GBM_FORMAT_NV12:
			return 12;

		case GBM_FORMAT_XRGB4444:
		case GBM_FORMAT_XBGR4444:
		case GBM_FORMAT_RGBX4444:
		case GBM_FORMAT_BGRX4444:
		case GBM_FORMAT_ARGB4444:
		case GBM_FORMAT_ABGR4444:
		case GBM_FORMAT_RGBA4444:
		case GBM_FORMAT_BGRA4444:
		case GBM_FORMAT_XRGB1555:
		case GBM_FORMAT_XBGR1555:
		case GBM_FORMAT_RGBX5551:
		case GBM_FORMAT_BGRX5551:
		case GBM_FORMAT_ARGB1555:
		case GBM_FORMAT_ABGR1555:
		case GBM_FORMAT_RGBA5551:
		case GBM_FORMAT_BGRA5551:
		case GBM_FORMAT_RGB565:
		case GBM_FORMAT_BGR565:
		case GBM_FORMAT_YUYV:
		case GBM_FORMAT_YVYU:
		case GBM_FORMAT_UYVY:
		case GBM_FORMAT_VYUY:
			return 16;

		case GBM_FORMAT_RGB888:
		case GBM_FORMAT_BGR888:
			return 24;

		case GBM_FORMAT_XRGB8888:
		case GBM_FORMAT_XBGR8888:
		case GBM_FORMAT_RGBX8888:
		case GBM_FORMAT_BGRX8888:
		case GBM_FORMAT_ARGB8888:
		case GBM_FORMAT_ABGR8888:
		case GBM_FORMAT_RGBA8888:
		case GBM_FORMAT_BGRA8888:
		case GBM_FORMAT_XRGB2101010:
		case GBM_FORMAT_XBGR2101010:
		case GBM_FORMAT_RGBX1010102:
		case GBM_FORMAT_BGRX1010102:
		case GBM_FORMAT_ARGB2101010:
		case GBM_FORMAT_ABGR2101010:
		case GBM_FORMAT_RGBA1010102:
		case GBM_FORMAT_BGRA1010102:
		case GBM_FORMAT_AYUV:
			return 32;

		default:
			LOG_MSG(error, "unknown GBM format " << format);
	}

	return 0;
}


struct drm_fb
{
	struct gbm_bo *bo;
	uint32_t fb_id;
};


static void drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
	int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
	drm_fb *fb = reinterpret_cast < drm_fb * > (data);

	if (fb->fb_id)
		drmModeRmFB(drm_fd, fb->fb_id);

	delete fb;
}

static drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
	// We want to use this buffer object (abbr. "bo") as a scanout buffer.
	// To that end, we associate the bo with the DRM by using drmModeAddFB().
	// However, this needs to be called only once, and the counterpart,
	// drmModeRmFB(), needs to be called when the bo is cleaned up.
	//
	// To fulfill these requirements, add extra framebuffer information to the
	// bo as "user data". This way, if this user data pointer is NULL, it means
	// that no framebuffer information was generated yet & the bo was not set
	// as a scanout buffer with drmModeAddFB() yet, and we have perform these
	// steps. Otherwise, if it is non-NULL, we know we do not have to set up
	// anything (since it was done already) and just return the pointer to the
	// framebuffer information.
	drm_fb *fb = reinterpret_cast < drm_fb * > (gbm_bo_get_user_data(bo));
	if (fb != nullptr)
	{
		// The bo was already set up as a scanout framebuffer. Just
		// return the framebuffer information.
		return fb;
	}

	// If this point is reached, then we have to setup the bo as a scanout framebuffer.

	int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));

	fb = new drm_fb;
	fb->bo = bo;

	uint32_t width = gbm_bo_get_width(bo);
	uint32_t height = gbm_bo_get_height(bo);
	uint32_t stride = gbm_bo_get_stride(bo);
	uint32_t format = gbm_bo_get_format(bo);
	uint32_t handle = gbm_bo_get_handle(bo).u32;

	int depth = gbm_depth_from_format(format);
	int bpp = gbm_bpp_from_format(format);

	LOG_MSG(debug, "Attempting to add GBM BO as scanout framebuffer "
	            << " width/height: " << width << "/" << height << " pixels "
	            << " stride: " << stride << " bytes  format: " << gbm_format_to_string(format)
	            << " depth: " << depth << " bits  total bpp: " << bpp << " bits"
	);

	// Set the bo as a scanout framebuffer
	int ret = drmModeAddFB(
		drm_fd,
		width, height,
		depth, bpp,
		stride,
		handle,
		&fb->fb_id
	);
	if (ret != 0)
	{
		LOG_MSG(error, "Failed to add GBM BO as scanout framebuffer: " << std::strerror(errno) << " (" << errno << ")");
		free(fb);
		return nullptr;
	}

	// Add the framebuffer information to the bo as user data, and also install a callback
	// that cleans up this extra information whenever the bo itself is discarded
	gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

	return fb;
}


struct egl_platform::internal
{
	struct gbm_device *m_gbm_dev;
	struct gbm_surface *m_gbm_surf;
	struct gbm_bo *m_gbm_current_bo;

	int m_drm_fd;
	drmModeRes *m_drm_mode_resources;
	drmModeConnector *m_drm_mode_connector;
	drmModeModeInfo *m_drm_mode_info;
	int m_crtc_index;
	std::uint32_t m_crtc_id;


	internal()
		: m_gbm_dev(nullptr)
		, m_gbm_surf(nullptr)
		, m_gbm_current_bo(nullptr)
		, m_drm_fd(-1)
		, m_drm_mode_resources(nullptr)
		, m_drm_mode_connector(nullptr)
		, m_drm_mode_info(nullptr)
		, m_crtc_index(-1)
		, m_crtc_id(INVALID_CRTC)
	{
		// Structures are not initialized here, but in the main egl_platform constructor.
	}


	~internal()
	{
		// NOTE: Unlike with the constructor, the destructor *does* shut down structures
		// and states. This makes it easier to use the emergency scoped cleanup in the
		// egl_platform constructor (its cleanup steps are exactly what the egl_platform
		// destructor also needs to do, so by doing the actual shutdown in here, we avoid
		// code duplication in the cleanup & in the egl_platform destructor).

		shutdown_gbm();
		shutdown_drm();

		if (m_drm_fd < 0)
			close(m_drm_fd);
	}


	bool find_drm_node()
	{
		struct udev *udev_ctx = nullptr;
		udev_enumerate *udev_enum = nullptr;

		auto cleanup = make_scope_guard([&]() {
			if (udev_enum != nullptr)
			{
				udev_enumerate_unref(udev_enum);
				udev_enum = nullptr;
				LOG_MSG(debug, "Cleaned up udev enumerate object");
			}

			if (udev_ctx != nullptr)
			{
				udev_unref(udev_ctx);
				udev_ctx = nullptr;
				LOG_MSG(debug, "Cleaned up udev context");
			}
		});

		// XXX: What if udev isn't available while booting? Use a hardcoded
		// /dev/dri/cardX node name as fallback?
		udev_ctx = udev_new();
		if (udev_ctx == nullptr)
		{
			LOG_MSG(error, "Could not create udev context");
			return false;
		}
		LOG_MSG(debug, "Created udev context");

		udev_enum = udev_enumerate_new(udev_ctx);
		if (udev_enum == nullptr)
		{
			LOG_MSG(error, "Could not create udev enumerate object");
			return false;
		}
		LOG_MSG(debug, "Created udev enumerate object");

		// TODO: To be 100% sure we pick the right device, also check
		// if this is a GPU, because a pure scanout device could also
		// have a DRM subsystem for example. However, currently it is
		// unclear how to do that. By trying to create an EGL context?
		udev_enumerate_add_match_subsystem(udev_enum, "drm");
		if (udev_enumerate_scan_devices(udev_enum) < 0)
		{
			LOG_MSG(error, "Could not scan with udev for devices with a drm subsystem");
			return false;
		}
		LOG_MSG(debug, "Scanned for udev devices with a drm subsytem");

		m_drm_fd = -1;

		udev_list_entry *entry;
		udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(udev_enum))
		{
			char const *syspath = udev_list_entry_get_name(entry);
			udev_device *udevice = udev_device_new_from_syspath(udev_ctx, syspath);
			char const *devnode = udev_device_get_devnode(udevice);

			if (std::strstr(devnode, "/dev/dri/card") != devnode)
				continue;

			LOG_MSG(debug, "Found udev device with syspath \"" << syspath << "\" and device node \"" << devnode << "\"");

			m_drm_fd = open(devnode, O_RDWR | O_CLOEXEC);
			if (m_drm_fd < 0)
			{
				LOG_MSG(warning, "Cannot open device node \"" << devnode << "\": " << std::strerror(errno) << " (" << errno << ")");
				continue;
			}

			LOG_MSG(debug, "Device node \"" << devnode << "\" is a valid DRM device node");
			break;
		}

		return (m_drm_fd >= 0);
	}


	bool setup_drm()
	{
		// Scoped cleanup in case an error happens in the middle of this function
		auto cleanup = make_scope_guard([&] { shutdown_drm(); });


		// Get the DRM mode resources

		m_drm_mode_resources = drmModeGetResources(m_drm_fd);
		if (m_drm_mode_resources == nullptr)
		{
			LOG_MSG(error, "Could not get DRM resources: " << std::strerror(errno) << " (" << errno << ")");
			return false;
		}

		LOG_MSG(debug, "Got DRM resources");


		// Find a connected connector. The connector is where the pixel data is finally
		// sent to, and typically connects to some form of display, like an HDMI TV,
		// an LVDS panel etc.

		{
			drmModeConnector *connector = nullptr;

			LOG_MSG(debug, "Checking " << m_drm_mode_resources->count_connectors << " DRM connector(s)");
			for (int i = 0; i < m_drm_mode_resources->count_connectors; ++i)
			{
				connector = drmModeGetConnector(m_drm_fd, m_drm_mode_resources->connectors[i]);
				LOG_MSG(debug, "Found DRM connector #" << i << " \"" << get_name_for_drm_connector(connector)
				            << "\" with ID " << connector->connector_id);

				if (connector->connection == DRM_MODE_CONNECTED)
				{
					LOG_MSG(debug, "DRM connector #" << i << " is connected");
					break;
				}

				drmModeFreeConnector(connector);
				connector = nullptr;
			}

			if (connector == nullptr)
			{
				LOG_MSG(error, "No connected DRM connector found");
				return false;
			}

			m_drm_mode_connector = connector;
		}


		// Check out what modes are supported by the chosen connector,
		// and pick either the "preferred" mode or the one with the largest
		// pixel area.

		{
			int selected_mode_index = -1;
			int selected_mode_area = -1;
			LOG_MSG(debug, "Checking " << m_drm_mode_connector->count_modes << " DRM mode(s) from selected connector");
			for (int i = 0; i < m_drm_mode_connector->count_modes; ++i)
			{
				drmModeModeInfo *current_mode = &(m_drm_mode_connector->modes[i]);
				int current_mode_area = current_mode->hdisplay * current_mode->vdisplay;

				LOG_MSG(debug, "Found DRM mode #" << i
					    << " width/height " << current_mode->hdisplay << "/" << current_mode->vdisplay
					    << " hsync/vsync start " << current_mode->hsync_start << "/" << current_mode->hsync_end
					    << " hsync/vsync end " << current_mode->hsync_end << "/" << current_mode->hsync_end
					    << " htotal/vtotal " << current_mode->htotal << "/" << current_mode->vtotal
					    << " hskew " << current_mode->hskew
					    << " vscan " << current_mode->vscan
					    << " vrefresh " << current_mode->vrefresh
					    << " preferred " << !!(current_mode->type & DRM_MODE_TYPE_PREFERRED)
				);

				if ((current_mode->type & DRM_MODE_TYPE_PREFERRED) || (current_mode_area > selected_mode_area))
				{
					m_drm_mode_info = current_mode;
					selected_mode_area = current_mode_area;
					selected_mode_index = i;

					if (current_mode->type & DRM_MODE_TYPE_PREFERRED)
						break;
				}
			}

			if (m_drm_mode_info == nullptr)
			{
				LOG_MSG(error, "No usable DRM mode found");
				return false;
			}

			LOG_MSG(debug, "Selected DRM mode #" << selected_mode_index);
		}


		// Find an encoder that is attached to the chosen connector. Also find the index/id of
		// the CRTC associated with this encoder. The encoder takes pixel data from the CRTC and
		// transmits it to the connector. The CRTC roughly represents the scanout framebuffer.
		//
		// Ultimately, we only care about the CRTC index & ID, so the encoder reference is discarded
		// here once these are found. The CRTC index is the index in the m_drm_mode_resources'
		// CRTC array, while the ID is an identifier used by the DRM to refer to the CRTC universally.
		// (We need the CRTC information for page flipping and DRM scanout framebuffer configuration.)

		{
			drmModeEncoder *encoder = nullptr;

			LOG_MSG(debug, "Checking " << m_drm_mode_resources->count_encoders << " DRM encoder(s)");
			for (int i = 0; i < m_drm_mode_resources->count_encoders; ++i)
			{
				encoder = drmModeGetEncoder(m_drm_fd, m_drm_mode_resources->encoders[i]);
				LOG_MSG(debug, "Found DRM encoder #" << i << " \"" << get_name_for_drm_encoder(encoder) << "\"");

				if (encoder->encoder_id == m_drm_mode_connector->encoder_id)
				{
					LOG_MSG(debug, "DRM encoder #" << i << " corresponds to selected DRM connector -> selected");
					break;
				}
				drmModeFreeEncoder(encoder);
				encoder = nullptr;
			}

			if (encoder == nullptr)
			{
				LOG_MSG(debug, "No encoder found; searching for CRTC ID in the connector");
				m_crtc_id = find_crtc_id_for_connector();
			}
			else
			{
				LOG_MSG(debug, "Using CRTC ID from selected encoder");
				m_crtc_id = encoder->crtc_id;
				drmModeFreeEncoder(encoder);
			}

			if (m_crtc_id == INVALID_CRTC)
			{
				LOG_MSG(error, "No CRTC found");
				return false;
			}

			LOG_MSG(debug, "CRTC with ID " << m_crtc_id << " found; now locating it in the DRM mode resources CRTC array");

			for (int i = 0; i < m_drm_mode_resources->count_crtcs; ++i)
			{
				if (m_drm_mode_resources->crtcs[i] == m_crtc_id)
				{
					m_crtc_index = i;
					break;
				}
			}

			if (m_crtc_index < 0)
			{
				LOG_MSG(error, "No matching CRTC entry in DRM resources found");
				return false;
			}

			LOG_MSG(debug, "CRTC with ID " << m_crtc_id << " can be found at index #" << m_crtc_index << " in the DRM mode resources CRTC array");
		}


		// We are done. Dismiss the emergency scoped cleanup.
		cleanup.dismiss();

		LOG_MSG(debug, "DRM structures initialized");

		return true;
	}


	void shutdown_drm()
	{
		m_drm_mode_info = nullptr;

		m_crtc_index = -1;
		m_crtc_id = INVALID_CRTC;

		if (m_drm_mode_connector != nullptr)
		{
			drmModeFreeConnector(m_drm_mode_connector);
			m_drm_mode_connector = nullptr;
		}

		if (m_drm_mode_resources != nullptr)
		{
			drmModeFreeResources(m_drm_mode_resources);
			m_drm_mode_resources = nullptr;
		}

		LOG_MSG(debug, "DRM structures shut down");
	}


	bool setup_gbm()
	{
		m_gbm_dev = gbm_create_device(m_drm_fd);
		if (m_gbm_dev == nullptr)
		{
			LOG_MSG(error, "Creating GBM device failed");
			return false;
		}

		LOG_MSG(debug, "GBM structures initialized");

		return true;
	}


	void shutdown_gbm()
	{
		if (m_gbm_surf != nullptr)
		{
			if (m_gbm_current_bo != nullptr)
			{
				gbm_surface_release_buffer(m_gbm_surf, m_gbm_current_bo);
				m_gbm_current_bo = nullptr;
			}

			gbm_surface_destroy(m_gbm_surf);
			m_gbm_surf = nullptr;
		}

		if (m_gbm_dev != nullptr)
		{
			gbm_device_destroy(m_gbm_dev);
			m_gbm_dev = nullptr;
		}

		LOG_MSG(debug, "GBM structures shut down");
	}


private:
	std::uint32_t find_crtc_id_for_encoder(drmModeEncoder const *encoder)
	{
		for (int i = 0; i < m_drm_mode_resources->count_crtcs; ++i)
		{
			// possible_crtcs is a bitmask as described here:
			// https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
			std::uint32_t const crtc_mask = 1 << i;
			std::uint32_t const crtc_id = m_drm_mode_resources->crtcs[i];

			if (encoder->possible_crtcs & crtc_mask)
				return crtc_id;
		}

		// No match found
		return INVALID_CRTC;
	}


	std::uint32_t find_crtc_id_for_connector()
	{
		for (int i = 0; i < m_drm_mode_connector->count_encoders; ++i)
		{
			std::uint32_t encoder_id = m_drm_mode_connector->encoders[i];
			drmModeEncoder *encoder = drmModeGetEncoder(m_drm_fd, encoder_id);

			if (encoder != nullptr)
			{
				std::uint32_t crtc_id = find_crtc_id_for_encoder(encoder);
				drmModeFreeEncoder(encoder);

				if (crtc_id != INVALID_CRTC)
					return crtc_id;
			}
		}

		// No match found
		return INVALID_CRTC;
	}
}; // internal class end




egl_platform::egl_platform()
	: m_native_display(0)
	, m_native_window(0)
	, m_egl_display(EGL_NO_DISPLAY)
	, m_egl_context(EGL_NO_CONTEXT)
	, m_egl_surface(EGL_NO_SURFACE)
	, m_is_valid(false)
{
	// Scoped cleanup in case an error happens in the middle of the constructor

	m_internal = new internal;
	auto internal_guard = make_scope_guard([&]() {
		// The internal class' destructor takes care of cleaning up GBM, DRM etc.
		delete m_internal;
		// Make sure the m_internal pointer is set to null to avoid segfaults
		// in the egl_platform destructor
		m_internal = nullptr;

		m_native_display = 0;
		m_native_window = 0;
	});


	// Initialize display

	if (!m_internal->find_drm_node())
	{
		LOG_MSG(error, "Could not find DRM node");
		return;
	}

	if (!m_internal->setup_drm())
	{
		LOG_MSG(error, "Could not setup DRM connection");
		return;
	}

	if (!m_internal->setup_gbm())
	{
		LOG_MSG(error, "Could not setup GBM device");
		return;
	}

	{
		EGLint ver_major, ver_minor;

		// With Mesa, the GBM device *is* the "native display";
		// See the EGL_MESA_platform_gbm documentation for more
		m_native_display = m_internal->m_gbm_dev;

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

		LOG_MSG(info, "EGL initialized, version " << ver_major << "." << ver_minor);
	}


	// Initialize window & context

	{
		EGLint num_configs;
		EGLConfig chosen_config;
		EGLint gbm_format;

		// We want a config for an EGL context with OpenGL ES 2.x support and an RGB colorbuffer.
		// Also, we want to be able to use a window with this context.
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

		if (!eglGetConfigs(m_egl_display, nullptr, 0, &num_configs))
		{
			LOG_MSG(error, "eglGetConfigs failed: " << egl_get_last_error_string());
			return;
		}

		if (!eglChooseConfig(m_egl_display, eglconfig_attribs, &chosen_config, 1, &num_configs))
		{
			LOG_MSG(error, "eglChooseConfig failed: " << egl_get_last_error_string());
			return;
		}
		if (num_configs == 0)
		{
			LOG_MSG(error, "eglChooseConfig found no matching config");
			return;
		}

		if (!eglGetConfigAttrib(m_egl_display, chosen_config, EGL_NATIVE_VISUAL_ID, &gbm_format))
		{
			LOG_MSG(error, "eglGetConfigAttrib failed: " << egl_get_last_error_string());
			return;
		}

		LOG_MSG(debug, "GBM format (= native EGL visual ID): " << gbm_format_to_string(gbm_format));

		// Setup the GBM surface that will act as window, a container
		// for the EGL context. We'll use this surface to render into
		// with the GPU, and also as a scanout buffer for the DRM CRTC.
		// Note that unlike with the "classic framebuffer", we do not paint
		// into an existing framebuffer memory block; we have to explicitely
		// assign a buffer object to the CRTC as scanout framebuffer instead.
		m_internal->m_gbm_surf = gbm_surface_create(
			m_internal->m_gbm_dev,
			m_internal->m_drm_mode_info->hdisplay, m_internal->m_drm_mode_info->vdisplay,
			gbm_format,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING
		);
		if (m_internal->m_gbm_surf == nullptr)
		{
			LOG_MSG(error, "Could not create GBM surface");
			return;
		}
		LOG_MSG(debug, "Created GBM surface for rendering and scanout");

		// With Mesa, the GBM surface *is* the "native window";
		// See the EGL_MESA_platform_gbm documentation for more
		m_native_window = m_internal->m_gbm_surf;
		
		eglBindAPI(EGL_OPENGL_ES_API);

		m_egl_context = eglCreateContext(m_egl_display, chosen_config, EGL_NO_CONTEXT, ctx_attribs);
		if (m_egl_context == EGL_NO_CONTEXT)
		{
			LOG_MSG(error, "eglCreateContext failed: " << egl_get_last_error_string());
			return;
		}
		LOG_MSG(debug, "Created EGL context with OpenGL ES 2.x support");

		m_egl_surface = eglCreateWindowSurface(m_egl_display, chosen_config, m_native_window, nullptr);
		if (m_egl_surface == EGL_NO_SURFACE)
		{
			LOG_MSG(error, "eglCreateWindowSurface failed: " << egl_get_last_error_string());
			return;
		}
		LOG_MSG(debug, "Created EGL window with context attached");

		if (!make_current())
			return;

		// Explicitely set the viewport to make sure it is expanded to
		// encompass the whole GBM surface
		glViewport(0, 0, m_internal->m_drm_mode_info->hdisplay, m_internal->m_drm_mode_info->vdisplay);

		// First swapbuffers call as part of the page flipping setup. According
		// to the gbm_surface_lock_front_buffer(), this must be called prior to
		// calling that function.
		if (!eglSwapBuffers(m_egl_display, m_egl_surface))
		{
			LOG_MSG(error, "eglSwapBuffers failed: " << egl_get_last_error_string());
			return;
		}

		// Lock the bo's front buffer as the "current bo". Needed for page flipping
		// in the egl_platform::swap_buffers() function below. The "current bo" is the
		// one that will be used for scanout.
		m_internal->m_gbm_current_bo = gbm_surface_lock_front_buffer(m_internal->m_gbm_surf);
		// Setup framebuffer information & set m_gbm_current_bo as a scanout framebuffer
		drm_fb *framebuf = drm_fb_get_from_bo(m_internal->m_gbm_current_bo);
		// Set m_gbm_current_bo as the data source for the CRTC
		if (drmModeSetCrtc(m_internal->m_drm_fd, m_internal->m_crtc_id, framebuf->fb_id, 0, 0, &(m_internal->m_drm_mode_connector->connector_id), 1, m_internal->m_drm_mode_info) != 0)
		{
			LOG_MSG(error, "Could not set DRM CRTC: " << std::strerror(errno) << " (" << errno << ")");
		}

		LOG_MSG(debug, "Page flipping set up");
	}


	// Construction complete. Dismiss the scoped guard and mark ourselves as valid.
	internal_guard.dismiss();
	LOG_MSG(debug, "Mesa GBM EGL platform initialized");
	m_is_valid = true;
}


egl_platform::~egl_platform()
{
	// Cleanup the EGL display before everything else, since it relies
	// on a valid DRM & GBM setup.
	if (m_egl_display != EGL_NO_DISPLAY)
	{
		LOG_MSG(debug, "Shutting down Mesa GBM EGL display");
		eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(m_egl_display);
	}

	delete m_internal;

	LOG_MSG(debug, "Mesa GBM EGL platform shut down");
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
	// When using GBM and DRM directly like we do here, we have
	// to worry about page flipping ourselves - eglSwapBuffers()
	// won't do it for us. However, after the eglSwapBuffers()
	// call, calling gbm_surface_lock_front_buffer() will return
	// a new framebuffer bo to use as "front buffer". We add it
	// too as a scanout buffer, then invoke drmModePageFlip().

	if (!eglSwapBuffers(m_egl_display, m_egl_surface))
	{
		LOG_MSG(error, "eglSwapBuffers failed: " << egl_get_last_error_string());
		return false;
	}

	auto page_flip_handler = [](int /*fd*/, unsigned int /*frame*/, unsigned int /*sec*/, unsigned int /*usec*/, void *data) -> void
	{
		int *waiting_for_flip = reinterpret_cast < int* > (data);
		*waiting_for_flip = 0;
	};

	// Get the new "front buffer" and set it up as a scanout buffer.
	struct gbm_bo *next_bo = gbm_surface_lock_front_buffer(m_internal->m_gbm_surf);
	drm_fb *framebuf = drm_fb_get_from_bo(next_bo);

	// Setup an emergency scoped cleanup in case there is an error,
	// to make sure the new "front buffer" is cleaned up and nothing leaks.
	auto cleanup = make_scope_guard([&]() {
		gbm_surface_release_buffer(m_internal->m_gbm_surf, next_bo);
	});

	// Set up a poll() structure that we'll use to listen for DRM events
	struct pollfd pfd;
	pfd.fd = m_internal->m_drm_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	drmEventContext evctx;
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.page_flip_handler = page_flip_handler;
	evctx.vblank_handler = nullptr;

	// Initiate the page flipping. This will finish asynchronously,
	// so we need the poll() based wait loop below.
	int waiting_for_flip = 1;
	int ret = drmModePageFlip(m_internal->m_drm_fd, m_internal->m_crtc_id, framebuf->fb_id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
	if (ret != 0)
	{
		// NOTE: According to libdrm sources, the page is _not_
		// considered flipped if drmModePageFlip() reports an error,
		// so we do not update the m_gbm_current_bo pointer here
		LOG_MSG(error, "Could not flip DRM pages");
		return false;
	}

	// NOTE: The page flip handler is called by drmHandleEvent(), so waiting_for_flip
	// does not have to be protected by any mutex.
	while (waiting_for_flip)
	{
		ret = poll(&pfd, 1, -1);

		if (ret < 0)
		{
			if (errno == EINTR)
				LOG_MSG(debug, "Signal caught during poll() call");
			else
				LOG_MSG(error, "poll() call failed: " << std::strerror(errno) << " (" << errno << ")");

			return false;;
		}

		drmHandleEvent(m_internal->m_drm_fd, &evctx);
	}

	// Page flipping finished, meaning that the back buffer (where the GPU finished
	// painting the new frame in prior to the eglSwapBuffers() call) is now the front
	// buffers that is used for scanout by the CRTC. We need to update our pointers.
	// Release the old "current bo" (that is not used for scanout anymore) and set the
	// next bo as the current one (which is now the new current scanout framebuffer).
	// (We call this process page flipping, because bo's keep being reused, so it is
	// just like how hardware video buffer pages were flipped in older hardware.)
	gbm_surface_release_buffer(m_internal->m_gbm_surf, m_internal->m_gbm_current_bo);
	m_internal->m_gbm_current_bo = next_bo;

	// All done. We can dismiss the emergency cleanup procedure.
	cleanup.dismiss();

	return true;
}


long egl_platform::get_display_width() const
{
	return m_internal->m_drm_mode_info->hdisplay;
}


long egl_platform::get_display_height() const
{
	return m_internal->m_drm_mode_info->vdisplay;
}


} // namespace easysplash end
