find_library(HAVE_LIBEVENT NAMES event)
find_library(HAVE_PTHREAD NAMES pthread)
find_library(HAVE_ZIP NAMES zip)
find_library(HAVE_PNG NAMES png)

if(NOT HAVE_LIBEVENT)
  message(FATAL_ERROR "libevent not found")
endif()

if(NOT HAVE_PTHREAD)
  message(FATAL_ERROR "pthread not found")
endif()

if(NOT HAVE_ZIP)
  message(FATAL_ERROR "zip not found")
endif()

add_definitions(-std=gnu++11)

add_executable(
  easysplash
  main.cpp
  animation.cpp
  eventloop.cpp
  framebuffer.cpp)

add_executable(
  easysplashctl
  easysplashctl.cpp)

target_link_libraries(easysplash ${HAVE_LIBEVENT} ${HAVE_PTHREAD} ${HAVE_ZIP} ${HAVE_PNG})
target_link_libraries(easysplashctl ${HAVE_LIBEVENT})

install(TARGETS easysplash DESTINATION bin)
install(TARGETS easysplashctl DESTINATION bin)
