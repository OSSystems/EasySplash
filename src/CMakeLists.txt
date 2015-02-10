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
  set(DISPLAY_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/swrender" "${PIXMAN_INCLUDE_DIRS}")
  set(DISPLAY_LIBRARIES "${PIXMAN_LIBRARIES}")
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
