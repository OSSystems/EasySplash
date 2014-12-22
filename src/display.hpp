/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASH_DISPLAY_HPP
#define EASYSPLASH_DISPLAY_HPP

#include <cstddef>
#include "types.hpp"


namespace easysplash
{


/* display:
 *
 * This is an abstract interface for display logic. A display has a width, height
 * (both in pixels), and can load/unload and draw images. "Loading" an image means
 * the display takes in an instance of image, and transfers the pixels to some form
 * of internal representation (for example, textures in OpenGL based displays).
 * These representations are referred to by their image_handle. To manually unload
 * an image, unload_image() can be used. Note that images are automatically unloaded
 * when the display is shut down.
 *
 * swap_buffers() is used to bring the current draw buffer to the screen. This is
 * internally typically done by either blitting the back buffer to the front, or
 * by flipping back- and front buffer.
 */
class display
{
public:
	typedef std::uintptr_t image_handle;
	enum { invalid_image_handle = 0 };

	virtual ~display()
	{
	}

	virtual bool is_valid() const = 0;

	virtual long get_width() const = 0;
	virtual long get_height() const = 0;

	// useful variant for temporary images
	virtual image_handle load_image(image &&p_image)
	{
		image tmp_img(std::move(p_image));
		return load_image(tmp_img); // uses the second overload below
	}

	virtual image_handle load_image(image const &p_image) = 0;
	virtual void unload_image(image_handle const p_image_handle) = 0;

	virtual void draw_image(image_handle const p_image_handle, long const p_x1, long const p_y1, long const p_x2, long const p_y2) = 0;

	virtual void swap_buffers() = 0;
};


} // namespace easysplash end


#endif
