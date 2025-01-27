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
#include <dc/pvr.h>
#include <stdlib.h>
#include <stdio.h>
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512
#define FRAME_RATE 30 // Adjust this value to control the frame rate (frames per second)
#define VIDEO_LENGTH 30 // Adjust this value to control the video length (seconds)

// Function to render a video frame using KOS PVR
void render_video_frame(uint16_t* frame_data);

int main() {
    uint16_t video_frames[FRAME_WIDTH * FRAME_HEIGHT]; // Array to store RGB565 pixel data
    int num_frames = 0; // Number of video frames

    plm_t* mpeg; // Declare mpeg variable

    // Initialize KOS and PVR
    pvr_init_defaults();

    // Initialize pl_mpeg library
    mpeg = plm_create_with_filename("/rd/queen.mpeg");
    if (!mpeg) {
        printf("Error opening video file\n");
        return -1;
    }

    // Get video properties
    int frame_width = plm_get_width(mpeg);
    int frame_height = plm_get_height(mpeg);
    printf("Video width: %d, height: %d\n", frame_width, frame_height);

    if (frame_width != FRAME_WIDTH || frame_height != FRAME_HEIGHT) {
        printf("Video frame size mismatch\n");
        return -1;
    }

    plm_frame_t *frame; // Declare a pointer to a plm_frame_t struct

    // Loop through the frames of the video and count them while decoding
    while (num_frames < VIDEO_LENGTH) {
        // Use plm_decode_video to decode video frames
        frame = plm_decode_video(mpeg);
        if (!frame) {
            // No more frames to decode, exit the loop
            break;
        }

        printf("Decoding frame %d\n", num_frames);

        // Copy the frame data directly to video_frames array (assuming YUV422 format)
        memcpy(video_frames, frame->y.data, FRAME_WIDTH * FRAME_HEIGHT * 2);

        num_frames++; // Increment the frame counter for each successfully decoded frame.
    }

    printf("Total frames: %d\n", num_frames);
    fflush(stdout);
    // Close the video file
    plm_destroy(mpeg);

    int current_frame = 0; // Declare current_frame here
    int done = 0;
    uint64_t start_time, end_time;
    uint64_t frame_interval = 1000000 / FRAME_RATE; // Time interval between frames (microseconds)

    while (!done) {
        // Get the start time before rendering the current video frame
        start_time = timer_us_gettime64();

        pvr_wait_ready();
        // Render the current video frame
        render_video_frame(&video_frames[current_frame * FRAME_WIDTH * FRAME_HEIGHT]);

        
        /*
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        pvr_list_finish();
        pvr_scene_finish();
        */
        // Calculate the time taken to render the frame
        end_time = timer_us_gettime64();
        printf("Rendering frame %d (Time: %" PRIu64 " us)\n", current_frame, end_time - start_time);
        
        // Delay to control video playback speed
        uint64_t elapsed_time = end_time - start_time;
        if (elapsed_time < frame_interval) {
            timer_spin_sleep(frame_interval - elapsed_time);
        }

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

// Seems wierd that the frame data is uint16_t

void render_video_frame(uint16_t* frame_data) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_scene_begin(); // Begin PVR rendering scene

    pvr_list_begin(PVR_LIST_OP_POLY); // Begin PVR rendering list for opaque polygons

    // Use PVR_TXRFMT_YUV422 format instead of PVR_TXRFMT_RGB565
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED,
                    FRAME_WIDTH, FRAME_HEIGHT, frame_data, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);

    // Set up vertex data for a fullscreen quad
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

    pvr_list_finish(); // Finish PVR rendering list for opaque polygons

    pvr_scene_finish(); // Finish PVR rendering scene
}






