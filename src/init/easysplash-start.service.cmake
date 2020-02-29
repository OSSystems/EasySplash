# EasySplash - tool for animated splash screens
# Copyright (C) 2014, 2015  O.S. Systems Software LTDA.
#
# This file is part of EasySplash.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

[Unit]
Description=Starts EasySplash Boot screen
Wants=systemd-vconsole-setup.service
After=systemd-vconsole-setup.service systemd-udev-trigger.service systemd-udevd.service
DefaultDependencies=no

[Service]
EnvironmentFile=-@CMAKE_INSTALL_SYSCONFDIR@/default/easysplash
Type=notify
ExecStart=@CMAKE_INSTALL_PREFIX@@CMAKE_INSTALL_SBINDIR@/easysplash

[Install]
WantedBy=sysinit.target
