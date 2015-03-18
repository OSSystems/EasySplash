EasySplash
==========

This is a simple program for animated splash screens using OpenGL ES for rendering. It consists
of the main splash screen program (easysplash) and a small control program (easysplashctl).

EasySplash takes as input zip archives containing a description and PNG-encoded image frames.
The frames are divided in parts.

It is possible to run EasySplash in X11 and on the PC. This is useful for debugging purposes
and for trying out new animations on the PC before uploading it to embedded devices.

EasySplash can run in realtime or non-realtime mode. realtime mode means the animaton progresses
in realtime, without user interaction. In non-realtime mode, the animation progresses only by
sending progress updates over easysplashctl. The latter is useful if a progress value can be
determined while something is loading, for example.

Please refer to the LICENSE file included in the source code for
licensing terms.


Requirements
------------

* OpenGL ES 2.0 or later
* libpng version 1.2.50 or later
* zlib version 1.2.8 or later
* C++11 capable compiler on the build machine (recommended: gcc 4.8 or newer)
* Python 2 or 3 on the build machine (for running the build system)

Note that so far, EasySplash has only been tested on the PC and on i.MX6 hardware.


Animation structure
-------------------

Animations are contained in zip archives. The archive layout must be like this:

    desc.txt
    part1/100.png
    part1/101.png
    part1/102.png
    part1/103.png
    part2/488.png
    part2/489.png

The names of the part directories can be chosen freely. Only desc.txt needs to always be
named as shown. The PNGs inside the part directories can also be named freely; they will
be lexicographically sorted inside each part directory and played in that order.

desc.txt is a text file which defines the animation.
The first row is formatted this way:

    [output width] [output height] [fps]

[output width] and [output height] specify the width and height of a rectangle that is
centered on the screen. Frames will be scaled to fit inside that triangle. [fps] defines
the framerate in frames per second.

For example, a line "320 240 25" specifies a centered 320x240 output rectangle and
playback at 25 fps.

The next rows define the parts:

    [mode] [num loops] [pause] [name of part directory]

[mode] is either "p" or "c". "p" means the part must be played completely, even if
somebody requested EasySplash to stop (by using easysplashctl). Only after this
part finished, EasySplash can stop. "c" means it can be stopped immediately.
[num loops] defines how often this part is played. Minimum value is 1; if 0 is
specified, it is treated as 1.
[pause] is a legacy value and unused.
[name of part directory] specifies the name of the directory the part's frames are stored.

Example:

    p 2 0 beginning
    p 3 0 intermediate
    p 1 0 end

Three parts, all of which cannot be interrupted until they are completed (with the
exception of the last one; more on that later). The first part is played 2 times,
the second 3 times, the third once (or all the time, as explained later).
The first part's PNG frame images are stored in the beginning/ directory in the
zip archive, the second part's in intermediate/, the third one's in end/ .

The last part has a special meaning. In realtime mode, it is ran in an infinite
loop, and [mode] is assumed to be set to "c" (the value in desc.txt is ignored).


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

Example cmake call with an OpenGL ES display on the Framebuffer:

    cmake .. -DDISPLAY_TYPE_GLES=1 -DEGL_PLATFORM_VIV_FB=1

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
an error. Its only argument is the progress indicator, which is a number in the 0-100
range. For example, to transmit the progress value 55 to EasySplash, run:

    easysplashctl 55


Implementation notes
--------------------

The EasySplash code makes extensive use of return value optimization, move semantics,
type deduction, and lambdas. Therefore, it is recommended to use a good C++11 capable
compiler.
