#
# EasySplash - Linux Bootsplash
# Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
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

if (NOT EASYSPLASH_PID_FILE)
  set(EASYSPLASH_PID_FILE "/tmp/easysplash.pid")
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
  elseif (EGL_PLATFORM_RPI_DISPMANX)
    find_path(BCM_HOST_INCLUDE_DIR bcm_host.h)
    find_library(VCHOSTIF_LIBRARY vchostif)
    find_library(BCM_HOST_LIBRARY bcm_host)
    find_library(DL_LIBRARY dl)
    set(EGL_PLATFORM_INCLUDE_DIRS "${BCM_HOST_INCLUDE_DIR}")
    set(EGL_PLATFORM_CODE eglgles/egl_platform_rpi_dispmanx.cpp)
    set(EGL_PLATFORM_LIBRARIES ${BCM_HOST_LIBRARY} ${VCHOSTIF_LIBRARY} ${DL_LIBRARY})
  elseif (EGL_PLATFORM_GBM)
    pkg_check_modules(LIBUDEV libudev)
    pkg_check_modules(LIBDRM libdrm)
    pkg_check_modules(GBM gbm)
    set(EGL_PLATFORM_CODE eglgles/egl_platform_gbm.cpp)
    set(EGL_PLATFORM_INCLUDE_DIRS ${LIBUDEV_INCLUDE_DIRS} ${LIBDRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS})
    set(EGL_PLATFORM_LIBRARIES ${LIBUDEV_LIBRARIES} ${LIBDRM_LIBRARIES} ${GBM_LIBRARIES})
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

option(ENABLE_SYSVINIT_SUPPORT "Enable Sysvinit support" ON)
if (ENABLE_SYSVINIT_SUPPORT)
  set(WITH_SYSVINIT "ON")
else()
  set(WITH_SYSVINIT "OFF")
endif()

option(ENABLE_SYSTEMD_SUPPORT "Enable SystemD support" ON)

if (ENABLE_SYSTEMD_SUPPORT)
  pkg_check_modules(SYSTEMD "systemd" REQUIRED)
  pkg_check_modules(LIBSYSTEMD "libsystemd" REQUIRED)
  if (SYSTEMD_FOUND AND "${SYSTEMD_SYSTEM_UNIT_DIR}" STREQUAL "")
	execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd OUTPUT_VARIABLE SYSTEMD_SYSTEM_UNIT_DIR)
	string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SYSTEM_UNIT_DIR "${SYSTEMD_SYSTEM_UNIT_DIR}")
  elseif (NOT SYSTEMD_FOUND AND SYSTEMD_SYSTEM_UNIT_DIR)
	message (FATAL_ERROR "Variable SYSTEMD_SYSTEM_UNIT_DIR is defined, but we can't find systemd using pkg-config")
  endif()

  if (SYSTEMD_FOUND)
	set(WITH_SYSTEMD "ON")
	add_definitions(-DWITH_SYSTEMD)
	message(STATUS "systemd services install dir: ${SYSTEMD_SYSTEM_UNIT_DIR}")
  else()
	set(WITH_SYSTEMD "OFF")
  endif()
endif()

include_directories(${LIBPNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${LIBSYSTEMD_INCLUDE_DIRS} ${DISPLAY_INCLUDE_DIRS})

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

target_link_libraries(easysplash ${LIBPNG_LIBRARIES} ${ZLIB_LIBRARIES} ${LIBSYSTEMD_LIBRARIES} ${DISPLAY_LIBRARIES})

if (NOT CMAKE_INSTALL_SBINDIR)
  set(CMAKE_INSTALL_SBINDIR "sbin")
endif()

install(TARGETS easysplash DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(TARGETS easysplashctl DESTINATION ${CMAKE_INSTALL_SBINDIR})

add_subdirectory(init)
