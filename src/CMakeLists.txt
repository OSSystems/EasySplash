#
# EasySplash - Linux Bootsplash
# Copyright (C) 2014  O.S. Systems Software LTDA.
#
# This file is part of EasySplash.
#
# Please refer to the LICENSE file included in the source code for
# licensing terms.
#

find_package(PkgConfig)

pkg_check_modules(ZLIB zlib)
pkg_check_modules(LIBPNG libpng)

add_definitions(-Wextra -Wall -Wno-variadic-macros -std=c++11 -pedantic)

if (NOT CTL_FIFO_PATH)
  set(CTL_FIFO_PATH "/tmp/easysplash-ctl")
endif()

if (DISPLAY_TYPE_SWRENDER)

  pkg_check_modules(PIXMAN pixman-1)
  set(DISPLAY_CODE swrender/swrender_display.cpp)
  set(DISPLAY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/swrender" ${PIXMAN_INCLUDE_DIRS})
  set(DISPLAY_LIBRARIES ${PIXMAN_LIBRARIES})

elseif(DISPLAY_TYPE_G2D)

  find_path(G2D_INCLUDE_DIR g2d.h)
  find_library(G2D_LIBRARY NAMES g2d)
  set(DISPLAY_CODE g2d/g2d_display.cpp)
  set(DISPLAY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/g2d" "${G2D_INCLUDE_DIR}")
  set(DISPLAY_LIBRARIES "${G2D_LIBRARY}")

elseif(DISPLAY_TYPE_GLES)

  add_definitions(-DLINUX) # necessary for the Vivante EGL headers

  if (EGL_PLATFORM_X11)
    find_path(X11_INCLUDE_DIR X11/Xlib.h)
    find_library(X11_LIBRARY NAMES X11)
    set(EGL_PLATFORM_CODE eglgles/egl_platform_x11.cpp)
    set(EGL_PLATFORM_INCLUDE_DIRS "${X11_INCLUDE_DIR}")
    set(EGL_PLATFORM_LIBRARIES "${X11_LIBRARY}")
  elseif (EGL_PLATFORM_VIV_FB)
    add_definitions(-DEGL_API_FB) # necessary for the Vivante EGL headers in Framebuffer mode
    set(EGL_PLATFORM_CODE eglgles/egl_platform_viv_fb.cpp)
  else()
    message(FATAL_ERROR "No EGL platform set")
  endif()

  find_path(EGL_INCLUDE_DIR EGL/eglvivante.h)
  find_path(EGL_INCLUDE_DIR EGL/egl.h)
  find_library(EGL_LIBRARY EGL)
  find_path(GLES_INCLUDE_DIR GLES2/gl2.h)
  find_path(GLES_INCLUDE_DIR GLES2/gl2ext.h)
  find_library(GLES_LIBRARY GLESv2)

  set(DISPLAY_CODE ${EGL_PLATFORM_CODE} eglgles/gles_display.cpp eglgles/gl_misc.cpp eglgles/texture.cpp)
  set(DISPLAY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/eglgles" ${EGL_PLATFORM_INCLUDE_DIRS} "${EGL_INCLUDE_DIR}" "${GLES_INCLUDE_DIR}")
  set(DISPLAY_LIBRARIES ${EGL_PLATFORM_LIBRARIES} "${EGL_LIBRARY}" "${GLES_LIBRARY}")

else()
  message(FATAL_ERROR "Display type not set. Set one in the command line arguments. Example: -DDISPLAY_TYPE_SWRENDER=1")
endif()

include_directories(${LIBPNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${DISPLAY_INCLUDE_DIRS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

add_executable(
  easysplash
  animation.cpp
  event_loop.cpp
  linux_framebuffer.cpp
  load_png.cpp
  log.cpp
  main.cpp
  types.cpp
  zip_archive.cpp
  ${DISPLAY_CODE}
)

add_executable(
  easysplashctl
  easysplashctl.cpp
)

target_link_libraries(easysplash ${LIBPNG_LIBRARIES} ${ZLIB_LIBRARIES} ${DISPLAY_LIBRARIES})

install(TARGETS easysplash DESTINATION bin)
install(TARGETS easysplashctl DESTINATION bin)
