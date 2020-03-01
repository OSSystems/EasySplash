EasySplash
==========

This is a simple program for animated splash screens using OpenGL ES for rendering. It consists
of the main splash screen program (easysplash) and a small control program (easysplashctl).

EasySplash takes as input zip archives containing a description and PNG-encoded image frames.
The frames are divided in parts. The archive format is documented [here](doc/Animation-Structure.md)

It is possible to run EasySplash in X11 and on the PC. This is useful for debugging purposes
and for trying out new animations on the PC before uploading it to embedded devices.

EasySplash can run in realtime or non-realtime mode. realtime mode means the animaton progresses
in realtime, without user interaction. In non-realtime mode, the animation progresses only by
sending progress updates over easysplashctl. The latter is useful if a progress value can be
determined while something is loading, for example.

For example, the below is the animation that has been in use for O.S. Systems' demo images:

![O.S. Systems demo boot animation](https://github.com/OSSystems/easysplash/raw/master/doc/demo-animation.gif)

There are two examples, which may be used as reference:

* [O.S. Systems glowing logo](http://bit.ly/3cufarU)
* [O.S. Systems demo boot animation](http://bit.ly/38cwJcD)


Requirements
------------

* OpenGL ES 2.0 or later
* libpng version 1.2.50 or later
* zlib version 1.2.8 or later
* C++11 capable compiler on the build machine (recommended: gcc 4.8 or newer)
* CMake (used for as build system)



Progress, realtime, and non-realtime mode
-----------------------------------------

EasySplash defines progress in a 0-100 scale. 0 means the beginning of the animation,
100 the end. At value 100, EasySplash stops.

As described above, realtime mode means the animation progresses on its own, without
having to use easysplashctl. In realtime mode, value sent by easysplashctl to
EasySplash will not have any effect unless it is the value 100, at which it exits.

In non-realtime mode, the animation is advanced to the point corresponding to the
progress value sent with easysplashctl. It automatically starts at 0. If for
example 50 is sent to EasySplash, it will advance to the middle of the animation,
and display the frame at this point.

The non-realtime mode is useful if the splash screen shall cover the time until some
operation is finished, and there are notifications available how far along this
operation has gone. In that case, one can send progress updates to EasySplash with
easysplashctl, and when the operation finishes, the end of animation is reached.

The realtime mode is useful if no such notifications are available. One example would
be a Linux boot phase. In this situation, a free-running animation is preferable.


Building EasySplash
-------------------

EasySplash uses the [CMake build system](http://www.cmake.org/). 
To configure the build, first set the following environment variables to whatever is
necessary for cross compilation for your platform:

* `CXX`
* `CXXFLAGS`
* `LDFLAGS`
* `PKG_CONFIG_PATH`
* `PKG_CONFIG_SYSROOT_DIR`

Then, run from EasySplash's root directory:

    mkdir build
    cd build
    cmake .. -D[display-type]=1

(The aforementioned environment variables are only necessary for this configure call.)

[display-type] specifies which display type to use. At this point, the following types are available:

* `DISPLAY_TYPE_SWRENDER`: Software rendering to the Linux framebuffer using the
  [Pixman library](http://www.pixman.org/) for blitting frames to screen
* `DISPLAY_TYPE_G2D`: Hardware accelerated rendering using the G2D API (i.MX specific)
* `DISPLAY_TYPE_GLES`: Hardware accelerated rendering using the OpenGL ES API

In the `DISPLAY_TYPE_GLES` case, an additional argument is necessary, `-D[egl-platform]=1`:

* `EGL_PLATFORM_X11`: Create an EGL surface in an X11 window (useful for debugging animations; also works on the PC)
* `EGL_PLATFORM_VIV_FB`: Create an EGL surface directly on the framebuffer (Vivante GPU specific)
* `EGL_PLATFORM_RPI_DISPMANX`: Create an EGL surface directly on the framebuffer (Raspberry Pi "userland" dispmanx specific)
* `EGL_PLATFORM_GBM`: Create an EGL surface using MESA GBM (supports modern acceleration, such as Etnaviv)

Example cmake call with an OpenGL ES display on the Framebuffer:

    cmake .. -DDISPLAY_TYPE_GLES=1 -DEGL_PLATFORM_VIV_FB=1

Furthermore, support for System V and SystemD are available. Those are enabled using:

* `ENABLE_SYSTEMD_SUPPORT`: Enable SystemD support
* `ENABLE_SYSVINIT_SUPPORT`: Enable Sys V init support

Now that the build is configured, simply run:

    make

Once the build is finished, install using

    make install


Running EasySplash
------------------

This is the help screen of EasySplash when ran with the -h argument:

    Usage:  build/easysplash [OPTION]...
      -h  --help             display this usage information and exit
      -i  --zipfile          zipfile with animation to play
      -v  --loglevel         minimum levels messages must have to be logged
      -n  --non-realtime     Run in non-realtime mode (animation advances depending
                             on the percentage value from the ctl application)

Valid log levels are: trace, debug, info, warning, error. Log output goes to stderr.

If EasySplash was built with the `DISPLAY_TYPE_GLES` display type and the `EGL_PLATFORM_VIV_FB`
EGL platform, it will automatically enable double buffering and vsync to prevent tearing
artifacts. If this is not desired, start EasySplash with the `EASYSPLASH_NO_FB_MULTI_BUFFER`
environment variable set (the value is unimportant).

easysplashctl expects EasySplash to be already running, otherwise it exits with
an error. It requires one argument, the progress indicator, which is a number in the 0-100
range. For example, to transmit the progress value 55 to EasySplash, run:

    easysplashctl 55

An optional second argument is `--wait-until-finished`. It is only meaningful if the progress
value is 100. It instructs easysplashctl to wait until the EasySplash process ends. This is
useful to let other applications (which access the screen) wait until EasySplash has finished
displaying the animation. Example:

    easysplashctl 100 --wait-until-finished


Mesa3D / GBM specific notes
---------------------------

When running EasySplash on a platform with Mesa3D and GBM as OpenGL implementation and
framebuffer management system respectively, there are additional environment variables
that can be used for explicitely specifying several parameters that are otherwise
autodetected. These environment variables are:

* `EASYSPLASH_GBM_DRM_DEVICE`: Path to a DRI device node. Example: `/dev/dri/card0`.
  i.MX6 mainline kernel based platforms may need this variable to be set to
  `/dev/dri/card1` in order to select the etnaviv DRI node.
* `EASYSPLASH_GBM_DRM_CONNECTOR`: The DRM connector to use. A connector is for example
  a HDMI input, LVDS pins, etc. If EasySplash runs, but nothing shows on screen, then
  perhaps the wrong connector is being used. Run EasySplash with the `-v trace` flag
  to get verbose log output, then search for lines about DRM connectors to see a list
  of available connectors and to check which one is being selected. Then, if necessary,
  specify the correct one as the value of this environment variable.



Implementation notes
--------------------

The EasySplash code makes extensive use of return value optimization, move semantics,
type deduction, and lambdas. Therefore, it is recommended to use a good C++11 capable
compiler.


License
-------

EasySplash is licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or
   http://opensource.org/licenses/MIT)

at your option.


<a name="contribution"/>

Contribution
------------

Any kinds of contributions are welcome as a pull request.

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in EasySplash by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.
