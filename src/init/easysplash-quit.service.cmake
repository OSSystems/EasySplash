# EasySplash - tool for animated splash screens
# Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
#
# This file is part of EasySplash.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

[Unit]
Description=Terminate EasySplash Boot Screen
After=easysplash-start.service
DefaultDependencies=no

[Service]
Type=oneshot
ExecStart=@CMAKE_INSTALL_PREFIX@@CMAKE_INSTALL_SBINDIR@/easysplashctl 100 --wait-until-finished
TimeoutSec=30

[Install]
WantedBy=sysinit.target
