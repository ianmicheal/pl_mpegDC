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
#include <pthread.h>
#include <stdbool.h>
#include <inttypes.h>
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512
#define FRAME_RATE 30
#define BUFFER_SIZE 20

// Circular buffer definition
typedef struct {
    uint16_t** frames;
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

typedef struct {
    CircularBuffer buffer1;
    CircularBuffer buffer2;
    int current_buffer;
    bool decoding_done;
    pthread_mutex_t done_mutex;
    pthread_cond_t done_cond;
} DoubleBuffer;

volatile int num_frames = 0;

// Function declarations
void init_kos_pvr();
void render_video_frame(uint16_t* frame_data);
int add_frame_to_buffer(CircularBuffer* buffer, uint16_t* frame);
uint16_t* get_frame_from_buffer(CircularBuffer* buffer);
int is_buffer_empty(CircularBuffer* buffer);
int is_buffer_full(CircularBuffer* buffer);
void* decode_thread(void* arg);
void* render_thread(void* arg);
void* maple_event_thread(void* arg);

void render_video_frame(uint16_t* frame_data) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert[4];

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED,
                     FRAME_WIDTH, FRAME_HEIGHT, frame_data, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);

    vert[0].argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert[0].oargb = 0;
    vert[0].flags = PVR_CMD_VERTEX;

    vert[0].x = 1;
    vert[0].y = 1;
    vert[0].z = 1;
    vert[0].u = 0.0;
    vert[0].v = 0.0;

    vert[1].x = 640;
    vert[1].y = 1;
    vert[1].z = 1;
    vert[1].u = 1.0;
    vert[1].v = 0.0;

    vert[2].x = 1;
    vert[2].y = 480;
    vert[2].z = 1;
    vert[2].u = 0.0;
    vert[2].v = 1.0;

    vert[3].x = 640;
    vert[3].y = 480;
    vert[3].z = 1;
    vert[3].u = 1.0;
    vert[3].v = 1.0;

    pvr_prim(&vert[0], sizeof(vert[0]));
    pvr_prim(&vert[1], sizeof(vert[1]));
    pvr_prim(&vert[2], sizeof(vert[2]));
    pvr_prim(&vert[3], sizeof(vert[3]));
}

void init_double_buffer(DoubleBuffer* double_buffer) {
    double_buffer->buffer1.frames = (uint16_t**)malloc(BUFFER_SIZE * sizeof(uint16_t*));
    double_buffer->buffer1.front = 0;
    double_buffer->buffer1.rear = -1;
    double_buffer->buffer1.count = 0;
    pthread_mutex_init(&double_buffer->buffer1.mutex, NULL);
    pthread_cond_init(&double_buffer->buffer1.not_empty, NULL);
    pthread_cond_init(&double_buffer->buffer1.not_full, NULL);

    double_buffer->buffer2.frames = (uint16_t**)malloc(BUFFER_SIZE * sizeof(uint16_t*));
    double_buffer->buffer2.front = 0;
    double_buffer->buffer2.rear = -1;
    double_buffer->buffer2.count = 0;
    pthread_mutex_init(&double_buffer->buffer2.mutex, NULL);
    pthread_cond_init(&double_buffer->buffer2.not_empty, NULL);
    pthread_cond_init(&double_buffer->buffer2.not_full, NULL);

    double_buffer->current_buffer = 0;

    pthread_mutex_init(&double_buffer->done_mutex, NULL);
    pthread_cond_init(&double_buffer->done_cond, NULL);
    double_buffer->decoding_done = false;
}

int add_frame_to_double_buffer(DoubleBuffer* double_buffer, uint16_t* frame) {
    CircularBuffer* buffer = double_buffer->current_buffer == 0 ? &double_buffer->buffer1 : &double_buffer->buffer2;
    int result = add_frame_to_buffer(buffer, frame);
    if (result) {
        double_buffer->current_buffer = 1 - double_buffer->current_buffer;
    }
    return result;
}

uint16_t* get_frame_from_double_buffer(DoubleBuffer* double_buffer) {
    CircularBuffer* buffer = double_buffer->current_buffer == 0 ? &double_buffer->buffer1 : &double_buffer->buffer2;
    return get_frame_from_buffer(buffer);
}

void free_double_buffer(DoubleBuffer* double_buffer) {
    free(double_buffer->buffer1.frames);
    free(double_buffer->buffer2.frames);
}

void init_kos_pvr() {
    pvr_init_defaults();
}

int main() {
    CircularBuffer buffer;
    DoubleBuffer double_buffer;

    init_kos_pvr();

    plm_t* mpeg = plm_create_with_filename("/cd/video.mpeg");
    if (!mpeg) {
        printf("Error opening video file\n");
        return -1;
    }

    int frame_width = plm_get_width(mpeg);
    int frame_height = plm_get_height(mpeg);
    printf("Video width: %d, height: %d\n", frame_width, frame_height);

    if (frame_width != FRAME_WIDTH || frame_height != FRAME_HEIGHT) {
        printf("Video frame size mismatch\n");
        return -1;
    }

    buffer.frames = (uint16_t**)malloc(BUFFER_SIZE * sizeof(uint16_t*));
    buffer.front = 0;
    buffer.rear = -1;
    buffer.count = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);
    pthread_cond_init(&buffer.not_full, NULL);

    init_double_buffer(&double_buffer);

   pthread_t decode_tid;
    pthread_create(&decode_tid, NULL, decode_thread, (void*)&double_buffer);

    pthread_t render_tid;
    pthread_create(&render_tid, NULL, render_thread, (void*)&double_buffer);

    pthread_t maple_event_tid;
    pthread_create(&maple_event_tid, NULL, maple_event_thread, (void*)&buffer);

    pthread_join(decode_tid, NULL);

    // Signal the rendering thread that decoding is done and the buffer is not empty
    pthread_mutex_lock(&double_buffer.done_mutex);
    double_buffer.decoding_done = true;
    pthread_cond_signal(&double_buffer.done_cond);
    pthread_mutex_unlock(&double_buffer.done_mutex);

    // Wait for the rendering thread to finish
    pthread_join(render_tid, NULL);

    printf("Total frames: %d\n", num_frames);

    plm_destroy(mpeg);

    free(buffer.frames);

    return 0;
}

int add_frame_to_buffer(CircularBuffer* buffer, uint16_t* frame) {
    pthread_mutex_lock(&buffer->mutex);

    if (is_buffer_full(buffer)) {
        pthread_mutex_unlock(&buffer->mutex);
        return 0;
    }

    buffer->rear = (buffer->rear + 1) % BUFFER_SIZE;
    buffer->frames[buffer->rear] = frame;
    buffer->count++;

    pthread_cond_signal(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);
    return 1;
}

uint16_t* get_frame_from_buffer(CircularBuffer* buffer) {
    pthread_mutex_lock(&buffer->mutex);

    while (is_buffer_empty(buffer)) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    uint16_t* frame = buffer->frames[buffer->front];
    buffer->front = (buffer->front + 1) % BUFFER_SIZE;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);

    pthread_mutex_unlock(&buffer->mutex);
    return frame;
}

int is_buffer_empty(CircularBuffer* buffer) {
    return (buffer->count == 0);
}

int is_buffer_full(CircularBuffer* buffer) {
    return (buffer->count == BUFFER_SIZE);
}

void* decode_thread(void* arg) {
    DoubleBuffer* double_buffer = (DoubleBuffer*)arg;

    plm_t* mpeg;
    mpeg = plm_create_with_filename("/cd/video.mpeg");
    if (!mpeg) {
        printf("Error opening video file\n");
        pthread_exit(NULL);
    }

    plm_frame_t* frame;
    uint64_t start_time, end_time;

    while (1) {
        frame = plm_decode_video(mpeg);
        if (!frame) {
            // End of the video stream
            break;
        }

        start_time = timer_us_gettime64();
        printf("Decoding frame %d\n", num_frames);

        uint16_t* frame_data = (uint16_t*)malloc(FRAME_WIDTH * FRAME_HEIGHT * 2);
        memcpy(frame_data, frame->y.data, FRAME_WIDTH * FRAME_HEIGHT * 2);

        if (!add_frame_to_double_buffer(double_buffer, frame_data)) {
            free(frame_data);
            break;
        }

        // Increment the num_frames variable after each successful decoding
        pthread_mutex_lock(&double_buffer->done_mutex);
        num_frames++;
        pthread_mutex_unlock(&double_buffer->done_mutex);

        if (num_frames > BUFFER_SIZE) {
            uint16_t* old_frame = get_frame_from_buffer(&double_buffer->buffer1);
            free(old_frame);
        }

        end_time = timer_us_gettime64();
        printf("Decoding time for frame %d: %" PRIu64 " us\n", num_frames, end_time - start_time);
    }

    pthread_mutex_lock(&double_buffer->done_mutex);
    double_buffer->decoding_done = true;
    pthread_cond_signal(&double_buffer->done_cond);
    pthread_mutex_unlock(&double_buffer->done_mutex);

    printf("Decoding thread finished\n");
    printf("Total frames decoded: %d\n", num_frames);

    plm_destroy(mpeg);

    pthread_exit(NULL);
    return NULL;
}



void* render_thread(void* arg) {
    DoubleBuffer* double_buffer = (DoubleBuffer*)arg;
    int current_frame = 0;
    uint64_t start_time, end_time;
    uint64_t frame_interval = 1000000 / FRAME_RATE;

    printf("Render thread started\n");

    printf("Render thread waiting for decoding to finish...\n");
    pthread_mutex_lock(&double_buffer->done_mutex);
    while (!double_buffer->decoding_done) {
        pthread_cond_wait(&double_buffer->done_cond, &double_buffer->done_mutex);
    }
    pthread_mutex_unlock(&double_buffer->done_mutex);
    printf("Render thread detected decoding is done!\n");

    while (!is_buffer_empty(&double_buffer->buffer1) || !is_buffer_empty(&double_buffer->buffer2)) {
        start_time = timer_us_gettime64();

        if (is_buffer_empty(&double_buffer->buffer1) && is_buffer_empty(&double_buffer->buffer2)) {
            timer_spin_sleep(1000); // Sleep for 1 millisecond
            continue;
        }

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);

        // Clear the background to black before rendering the video frame
        // ...
       


        pvr_list_finish();
        pvr_list_begin(PVR_LIST_TR_POLY);

        uint16_t* frame_data = get_frame_from_double_buffer(double_buffer);
        if (frame_data) {
            render_video_frame(frame_data);
            free(frame_data);
        }

        pvr_list_finish();
        pvr_scene_finish();

        end_time = timer_us_gettime64();
        printf("Rendering frame %d (Time: %" PRIu64 " us)\n", current_frame, end_time - start_time);

        uint64_t elapsed_time = end_time - start_time;
        if (elapsed_time < frame_interval) {
            timer_spin_sleep(frame_interval - elapsed_time);
        }

        current_frame++;
    }

    printf("Render thread finished\n");
    printf("Total frames rendered: %d\n", current_frame);

    pthread_exit(NULL);
    return NULL;
}

void* maple_event_thread(void* arg) {
    CircularBuffer* buffer = (CircularBuffer*)arg;
    int* decoding_done_ptr = (int*)arg; // Cast arg to int* to access the decoding_done flag
    int done = 0;
    while (!done) {
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
        if (st->buttons & CONT_START) {
            done = 1;
            pthread_cond_signal(&buffer->not_empty);
        }
        MAPLE_FOREACH_END()
    }

    // Set the decoding_done flag using the pointer
    pthread_mutex_lock(&buffer->mutex);
    *decoding_done_ptr = 1;
    pthread_mutex_unlock(&buffer->mutex);

    pthread_exit(NULL);
    return NULL;
}







