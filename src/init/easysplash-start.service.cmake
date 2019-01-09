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
