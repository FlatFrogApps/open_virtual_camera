/*
* This file is originally from pyvirtualcam and was modified on 2024-03-01
*/


#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct video_queue;
typedef struct video_queue video_queue_t;

enum queue_state {
	SHARED_QUEUE_STATE_INVALID,
	SHARED_QUEUE_STATE_STARTING,
	SHARED_QUEUE_STATE_READY,
	SHARED_QUEUE_STATE_STOPPING,
};

extern video_queue_t *video_queue_create(uint32_t cx, uint32_t cy,
					 uint64_t interval);
extern void video_queue_close(video_queue_t *vq);
extern void video_queue_write(video_queue_t *vq, uint8_t **data,
			      uint32_t *linesize, uint64_t timestamp);

#ifdef __cplusplus
}
#endif
