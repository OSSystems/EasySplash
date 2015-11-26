#
# EasySplash - Linux Bootsplash
# Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
#
# This file is part of EasySplash.
#
# Please refer to the LICENSE file included in the source code for
# licensing terms.
#

if(WITH_SYSVINIT)

  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-start.init.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-start)

  install(FILES ${PROJECT_BINARY_DIR}/src/init/easysplash-start.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d )
endif()

if(WITH_SYSTEMD)

  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-start.service.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-start.service)
  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-quit.service.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-quit.service)

  install(FILES ${PROJECT_BINARY_DIR}/src/init/easysplash-start.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR} )
  install(FILES ${PROJECT_BINARY_DIR}/src/init/easysplash-quit.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR} )
endif()
