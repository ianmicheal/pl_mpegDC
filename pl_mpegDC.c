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
	Author: Ian micheal 
	Date: 31/07/23 09:03
	Description: Dreamcast preliminary port KallistiOS video pvr no sound 
	https://www.youtube.com/@IanMicheal/videos
	https://github.com/ianmicheal
*/




#include <kos.h>
#include <stdlib.h>
#include <stdio.h>
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

// Function to initialize KOS and PVR
void init_kos_pvr();

// Function to render a video frame using KOS PVR
void render_video_frame(uint16_t* frame_data);

int main() {
    uint16_t video_frames[FRAME_WIDTH * FRAME_HEIGHT]; // Array to store RGB565 pixel data
    int num_frames = 0; // Number of video frames

    plm_t* mpeg; // Declare mpeg variable

    // Initialize KOS and PVR
    init_kos_pvr();

    // Initialize pl_mpeg library
    mpeg = plm_create_with_filename("/cd/video.mpeg");
    if (!mpeg) {
        printf("Error opening video file\n");
        return -1;
    }

    // Get video properties
    int frame_width = plm_get_width(mpeg);
    int frame_height = plm_get_height(mpeg);

    if (frame_width != FRAME_WIDTH || frame_height != FRAME_HEIGHT) {
        printf("Video frame size mismatch\n");
        return -1;
    }

    plm_frame_t *frame; // Declare a pointer to a plm_frame_t struct
    int x, y;

    // Loop through the frames of the video and count them while decoding
    while (1) {
        // Use plm_decode_video to decode video frames
        frame = plm_decode_video(mpeg);
        if (!frame) {
            // No more frames to decode, exit the loop
            break;
        }

        // Convert the frame data to RGB565 and store it in video_frames array
        for (y = 0; y < FRAME_HEIGHT; y++) {
            for (x = 0; x < FRAME_WIDTH; x++) {
                uint8_t* yuv_data = frame->y.data; // Get the pointer to the frame data
                uint8_t y_value, u_value, v_value;

                // Calculate the index in the data array for the current pixel
                int y_index = y * frame_width + x;
                int uv_index = (y / 2) * (frame_width / 2) + x / 2;

                // Get YUV values for the pixel
                y_value = yuv_data[y_index];
                u_value = frame->cb.data[uv_index];
                v_value = frame->cr.data[uv_index];

                // Convert YUV to RGB565
                uint8_t r, g, b;
                r = (298 * (y_value - 16) + 409 * (v_value - 128) + 128) >> 8;
                g = (298 * (y_value - 16) - 100 * (u_value - 128) - 208 * (v_value - 128) + 128) >> 8;
                b = (298 * (y_value - 16) + 516 * (u_value - 128) + 128) >> 8;

                // Clamp RGB values
                if (r < 0) r = 0;
                if (r > 255) r = 255;
                if (g < 0) g = 0;
                if (g > 255) g = 255;
                if (b < 0) b = 0;
                if (b > 255) b = 255;

                video_frames[y * FRAME_WIDTH + x] = PVR_PACK_COLOR(1.0f, r / 255.0f, g / 255.0f, b / 255.0f);
            }
        }

        num_frames++; // Increment the frame counter for each successfully decoded frame.
    }

    // Close the video file
    plm_destroy(mpeg);

    int current_frame = 0;
    int done = 0;

    while (!done) {
        // Render the current video frame
        render_video_frame(video_frames);

        // Display the rendered frame
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        pvr_list_finish();
        pvr_scene_finish();

        // Delay to control video playback speed (adjust this as needed)
        timer_spin_sleep(1000000 / 30); // Assuming 30 FPS video

        // Increment current_frame to display the next video frame
        current_frame++;
        if (current_frame >= num_frames) {
            current_frame = 0;
        }

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
        if (st->buttons & CONT_START)
            done = 1;
        MAPLE_FOREACH_END()
    }

    return 0;
}

void init_kos_pvr() {
    pvr_init_defaults(); // Initialize KOS and PVR with default settings
    vid_set_mode(DM_640x480, PM_RGB565); // Set video mode to 640x480 with RGB565 pixel format
}

void render_video_frame(uint16_t* frame_data) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, FRAME_WIDTH, FRAME_HEIGHT, frame_data, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    vert.x = 1;
    vert.y = 1;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640;
    vert.y = 1;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 1;
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

