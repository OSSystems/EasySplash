EasySplash
==========

This is a simple program for animated splash screens using GStreamer for rendering. It consists of
an application which is capable of playing the animation and control its flow.

It is possible to run EasySplash in all GStreamer supported backends. This is useful for debugging
purposes and for trying out new animations on the desktop before uploading it to embedded devices.

For example, the below is the animation that has been in use for O.S. Systems' demo images:

![O.S. Systems demo boot animation](https://github.com/OSSystems/easysplash/raw/master/doc/demo-animation.gif)

There are two animation [examples](https://github.com/OSSystems/EasySplash/tree/master/data), which may be used as reference:

* [O.S. Systems glowing logo](https://github.com/OSSystems/EasySplash/tree/master/data/glowing-logo/)
* [O.S. Systems demo boot animation](https://github.com/OSSystems/EasySplash/tree/master/data/ossystems-demo/)


Requirements
------------

* Rust 1.40.0 or newer
* GStreamer (tested with 1.16)


Running EasySplash
------------------

This is the help screen of EasySplash when ran with the -h argument:

    Usage: easysplash <command> [<args>]

    EasySplash offers a convenient boot splash for Embedded Linux devices,
    focusing of simplicity and easy to use.

    Options:
      --help            display usage information

    Commands:
      open              open the render with the specific animation
      client            control the render from the user space


Animation format
----------------

EasySplash accepts animations in a directory which layout must be like this:

    animation.toml
    part1.mp4
    part2.mp4
    part3.mp4

The names of the animation files can be chosen freely. Only the animation manifest file, named
`animation.toml`, needs to always be named as shown.

The `animation.toml` file uses the [TOML](https://github.com/toml-lang/toml) format and intends to
define the animation. Below is a full example, for reference:

    [[part]]
    file = "part1.mp4"
    mode = "complete"

    [[part]]
    file = "part2.mp4"
    mode = "complete"
    repeat = 1

    [[part]]
    file = "part3.mp4"
    mode = "interruptable"

The animation view port is always rendered on the center of screen. The parts are defined using
`[[part]]`. The parts are inserted in the order encountered. For every part, following options are
available:

- `file`: specifies the file to use for the part (required).
- `mode`: is either `complete` or `interruptable`. (optional)
    - `complete` means the part must be played completely, even if somebody requested EasySplash to
      stop (default).
    - `interruptable` means it can be stopped immediately.
- `repeat`: defines how many times a part is replayed before moving to next. (optional)


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
