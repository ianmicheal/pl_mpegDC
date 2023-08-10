# PL_MPEGDC

**Name:** PL_MPEGDC  
**Copyright:** 7/31/20  
**Author:** Ian micheal + Magnes(Bertholet)  
**Date:** 31/07/23 09:03  
**Description:** Dreamcast preliminary port KallistiOS video PVR without sound

**Video Showcase:** [YouTube Channel](https://www.youtube.com/@IanMicheal/videos)  
**Repository:** [GitHub](https://github.com/ianmicheal)

## Update

Ian micheal Update this so far decode frames until the end of the video is reached. It dynamically determines the number of frames in the video file without using a hard-coded value. So it will play the whole video now.  
**Date:** 09/08/23 03:06


https://github.com/ianmicheal/pl_mpegDC/assets/59771322/b9103deb-06e6-444b-b7f9-1242687652c4


## Changelog

- **09/08/23 03:06:** Removed `mpegDC.c:66:25: warning: unused variable 'b8' [-Wunused-variable]`
- **09/08/23 03:06:** Removed `mpegDC.c:66:21: warning: unused variable 'g8' [-Wunused-variable]`
- **09/08/23 03:06:** Removed `mpegDC.c:66:17: warning: unused variable 'r8' [-Wunused-variable]`
- **09/08/23 03:06:** Added back maple press start to exit at any time
- **10/08/23 15:53:** Added using the  plm buffer instance and video decoder using the plm buffer Ian Micheal


# PL_MPEG - MPEG1 Video decoder, MP2 Audio decoder, MPEG-PS demuxer

Single-file MIT licensed library for C/C++

See [pl_mpeg.h](https://github.com/phoboslab/pl_mpeg/blob/master/pl_mpeg.h) for
the documentation.


## Why?

This is meant as a simple way to get video playback into your app or game. Other
solutions, such as ffmpeg require huge libraries and a lot of glue code.

MPEG1 is an old and inefficient codec, but it's still good enough for many use
cases. All patents related to MPEG1 and MP2 have expired, so it's completely
free now.

This library does not make use of any SIMD instructions, but because of
the relative simplicity of the codec it still manages to decode 4k60fps video
on a single CPU core (on my i7-6700k at least).

## Compilation on Linux

Use a GCC invocation like the following to build the example `pl_mpeg_player`
program:

```shell
gcc -o pl_mpeg_player pl_mpeg_player.c $(pkg-config --cflags --libs sdl2 glew)
```

## Example Usage

- [pl_mpeg_extract_frames.c](https://github.com/phoboslab/pl_mpeg/blob/master/pl_mpeg_extract_frames.c)
extracts all frames from a video and saves them as PNG.
 - [pl_mpeg_player.c](https://github.com/phoboslab/pl_mpeg/blob/master/pl_mpeg_player.c)
implements a video player using SDL2 and OpenGL for rendering.



## Encoding for PL_MPEG

Most [MPEG-PS](https://en.wikipedia.org/wiki/MPEG_program_stream) (`.mpg`) files
containing MPEG1 Video ("mpeg1") and MPEG1 Audio Layer II ("mp2") streams should
work with PL_MPEG. Note that `.mpg` files can also contain MPEG2 Video, which is
not supported by this library.

You can encode video in a suitable format using ffmpeg:

```
ffmpeg -i input.mp4 -c:v mpeg1video -q:v 0 -c:a mp2 -format mpeg output.mpg
```

`-q:v` sets a fixed video quality with a variable bitrate, where `0` is the 
highest. You may use `-b:v` to set a fixed bitrate instead; e.g. 
`-b:v 2000k` for 2000 kbit/s. Please refer to the 
[ffmpeg documentation](http://ffmpeg.org/ffmpeg.html#Options) for more details.

If you just want to quickly test the library, try this file:

https://phoboslab.org/files/bjork-all-is-full-of-love.mpg


## Limitations

- no error reporting. PL_MPEG will silently ignore any invalid data.
- the pts (presentation time stamp) for packets in the MPEG-PS container is
ignored. This may cause sync issues with some files.
- bugs, probably.
