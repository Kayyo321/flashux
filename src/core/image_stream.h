#ifndef FLASHUX_IMAGE_STREAM_H
#define FLASHUX_IMAGE_STREAM_H
#include "flashux/flasher.h"
typedef struct flashux_image_stream flashux_image_stream_t;
flashux_error_t flashux_image_stream_open(int fd, flashux_image_stream_t **stream);
flashux_error_t flashux_image_stream_read(flashux_image_stream_t *stream, unsigned char *buffer,
                                        size_t capacity, size_t *count);
void flashux_image_stream_close(flashux_image_stream_t **stream);
flashux_error_t flashux_image_decoded_size(int fd, uint64_t maximum, uint64_t *size,
                                         bool (*cancel)(void *), void *cancel_data,
                                         flashux_progress_cb progress_cb, void *progress_data);
#endif
