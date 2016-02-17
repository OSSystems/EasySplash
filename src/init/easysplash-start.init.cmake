#!/bin/sh
### BEGIN INIT INFO
# Provides:             easysplash
# Required-Start:
# Required-Stop:
# Default-Start:        S
# Default-Stop:
### END INIT INFO
# Copyright (C) 2015 O.S. Systems Software Ltda.  All Rights Reserved

read CMDLINE < /proc/cmdline
for x in $CMDLINE; do
	case $x in
		easysplash=false)
		echo "Boot splashscreen disabled"
		exit 0
		;;
	esac
done

@CMAKE_INSTALL_SBINDIR@/easysplash &
