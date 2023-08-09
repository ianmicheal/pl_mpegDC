/*

PL_MPEG - MPEG1 Video decoder, MP2 Audio decoder, MPEG-PS demuxer

Dominic Szablewski - https://phoboslab.org


-- LICENSE: The MIT License(MIT)

Copyright(c) 2019 Dominic Szablewski

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


Name:  PL_MPEGDC
Copyright: 7/31/20 
Author: Ian micheal + Magnes(Bertholet)
Date: 31/07/23 09:03
Description: Dreamcast preliminary port KallistiOS video pvr no sound 
https://www.youtube.com/@IanMicheal/videos
https://github.com/ianmicheal
Update: Ian micheal
Update this so far decode frames until the end of the video is reached.
It dynamically determines the number of frames in the video file without using a hard-coded value.
So it will play the whole video now 09/08/23 03:06
Removed mpegDC.c:66:25: warning: unused variable 'b8' [-Wunused-variable]
mpegDC.c:66:21: warning: unused variable 'g8' [-Wunused-variable]
mpegDC.c:66:17: warning: unused variable 'r8' [-Wunused-variable]
Added back maple press start to exit at any time
*/

#include <kos.h>
#include <dc/pvr.h>
#include <stdlib.h>
#include <stdio.h>
#define PL_MPEG_IMPLEMENTATION
#include "mpegDC.h"
#include <png/png.h>

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);
KOS_INIT_FLAGS(INIT_DEFAULT);

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512

void render_video_frame();

plm_frame_t *frame;
pvr_ptr_t video_frame;
pvr_poly_cxt_t cxt;
pvr_poly_hdr_t hdr;
pvr_vertex_t vert;

uint8_t raw[FRAME_WIDTH * FRAME_HEIGHT * 3];
uint16_t rgb[FRAME_WIDTH * FRAME_HEIGHT];

// Converts the 32 byte per pixel RGB frame to RGB565   
void convertTo565() {
    uint16_t _r, _g, _b;

    plm_frame_to_rgb(frame, raw, FRAME_WIDTH * 3);
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
        _r = ((raw[i * 3 + 0] >> 3) & 0x1F);
        _g = ((raw[i * 3 + 1] >> 2) & 0x3F);
        _b = ((raw[i * 3 + 2] >> 3) & 0x1F);

        rgb[i] = (_r << 11) | (_g << 5) | _b;
    }
}

int main() {
    plm_t *mpeg;
    pvr_init_defaults();
    pvr_mem_reset();
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    pvr_scene_finish();

    mpeg = plm_create_with_filename("/rd/queen.mpeg");
    if (!mpeg) {
        printf("Error opening video file\n");
        return -1;
    }
    plm_set_audio_enabled(mpeg, 0);

    int frame_width = plm_get_width(mpeg);
    int frame_height = plm_get_height(mpeg);
    printf("Video width: %d, height: %d\n", frame_width, frame_height);

    if (frame_width != FRAME_WIDTH || frame_height != FRAME_HEIGHT) {
        printf("Video frame size mismatch\n");
        return -1;
    }

    video_frame = pvr_mem_malloc(FRAME_WIDTH * FRAME_HEIGHT * 2);
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
                    FRAME_WIDTH, FRAME_HEIGHT, video_frame, PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);

    uint8_t frame_num = 0;
    uint64_t st, end, delta;

    while (1) {
        st = timer_ms_gettime64();
        frame = plm_decode_video(mpeg);
        if (!frame) {
            printf("End of video\n");
            break;
        }
        frame_num++;

        convertTo565();
        pvr_txr_load(rgb, video_frame, FRAME_WIDTH * FRAME_HEIGHT * 2);

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_set_bg_color(0.0f, 0.0f, 1.0f);
        render_video_frame(video_frame);
        pvr_scene_finish();

        end = timer_ms_gettime64();
        delta = end - st;
        printf("Frame: %d (%0.3f) took %llums\n", frame_num, frame->time, delta);
        fflush(stdout);

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
        if (st->buttons & CONT_START)
            goto exit_loop;
        MAPLE_FOREACH_END()
    }
    exit_loop:
    plm_destroy(mpeg);

    return 0;
}

void render_video_frame() {
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.flags = PVR_CMD_VERTEX;

    vert.x = 0;
    vert.y = 0;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640;
    vert.y = 0;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 0;
    vert.y = 480;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640;
    vert.y = 480;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}






