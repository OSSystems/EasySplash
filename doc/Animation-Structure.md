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
