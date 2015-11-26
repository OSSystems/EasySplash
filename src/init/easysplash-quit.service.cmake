[Unit]
Description=Terminate EasySplash Boot Screen
After=easysplash-start.service
DefaultDependencies=no

[Service]
Type=oneshot
ExecStart=@CMAKE_INSTALL_SBINDIR@/easysplashctl 100 --wait-until-finished
TimeoutSec=30

[Install]
WantedBy=multi-user.target
