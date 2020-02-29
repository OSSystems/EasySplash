# EasySplash - tool for animated splash screens
# Copyright (C) 2014-2016  O.S. Systems Software LTDA.
#
# This file is part of EasySplash.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

if(WITH_SYSVINIT)

  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-start.init.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-start)

  install(PROGRAMS ${PROJECT_BINARY_DIR}/src/init/easysplash-start DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d )
endif()

if(WITH_SYSTEMD)

  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-start.service.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-start.service)
  configure_file(${CMAKE_SOURCE_DIR}/src/init/easysplash-quit.service.cmake ${PROJECT_BINARY_DIR}/src/init/easysplash-quit.service)

  install(FILES ${PROJECT_BINARY_DIR}/src/init/easysplash-start.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR} )
  install(FILES ${PROJECT_BINARY_DIR}/src/init/easysplash-quit.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR} )
endif()
