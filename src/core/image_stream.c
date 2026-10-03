#include "image_stream.h"
#include "progress.h"
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
struct flashux_image_stream {
    struct archive *archive;
    int fd;
    off_t offset;
    unsigned char input[65536];
    bool finished;
};
static la_ssize_t read_source(struct archive *archive, void *data, const void **buffer)
{
    flashux_image_stream_t *stream = data;
    ssize_t count;
    do { count = pread(stream->fd, stream->input, sizeof(stream->input), stream->offset); }
    while (count < 0 && errno == EINTR);
    if (count < 0) { archive_set_error(archive, errno, "%s", "Unable to read image descriptor"); return -1; }
    stream->offset += count;
    *buffer = stream->input;
    return (la_ssize_t)count;
}
void flashux_image_stream_close(flashux_image_stream_t **stream)
{
    if (stream != NULL && *stream != NULL) {
        if ((*stream)->archive != NULL) {
            (void)archive_read_free((*stream)->archive);
            (*stream)->archive = NULL;
        }
        free(*stream);
        *stream = NULL;
    }
}
flashux_error_t flashux_image_stream_open(int fd, flashux_image_stream_t **stream_out)
{
    if (fd < 0 || stream_out == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *stream_out = NULL;
    flashux_image_stream_t *stream = calloc(1, sizeof(*stream));
    if (stream == NULL) { return FLASHUX_ERR_NO_MEMORY; }
    stream->fd = fd;
    stream->archive = archive_read_new();
    if (stream->archive == NULL) {
        flashux_image_stream_close(&stream);
        return FLASHUX_ERR_NO_MEMORY;
    }
    struct archive *reader = stream->archive;
    if (archive_read_support_filter_gzip(reader) != ARCHIVE_OK ||
        archive_read_support_filter_xz(reader) != ARCHIVE_OK ||
        archive_read_support_filter_bzip2(reader) != ARCHIVE_OK ||
        archive_read_support_filter_zstd(reader) != ARCHIVE_OK ||
        archive_read_support_format_raw(reader) != ARCHIVE_OK ||
        archive_read_support_format_zip_streamable(reader) != ARCHIVE_OK) {
        flashux_image_stream_close(&stream);
        return FLASHUX_ERR_UNSUPPORTED;
    }
    struct archive_entry *entry;
    if (archive_read_open(reader, stream, NULL, read_source, NULL) != ARCHIVE_OK ||
        archive_read_next_header(reader, &entry) != ARCHIVE_OK ||
        archive_entry_filetype(entry) != AE_IFREG || archive_entry_is_encrypted(entry) != 0) {
        flashux_image_stream_close(&stream);
        return FLASHUX_ERR_CORRUPT_IMAGE;
    }
    *stream_out = stream;
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_image_stream_read(flashux_image_stream_t *stream, unsigned char *buffer,
                                        size_t capacity, size_t *count)
{
    if (stream == NULL || buffer == NULL || count == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *count = 0;
    if (stream->finished) { return FLASHUX_SUCCESS; }
    while (*count < capacity) {
        la_ssize_t got = archive_read_data(stream->archive, buffer + *count, capacity - *count);
        if (got < 0) { return FLASHUX_ERR_CORRUPT_IMAGE; }
        if (got == 0) {
            struct archive_entry *next;
            if (archive_read_next_header(stream->archive, &next) != ARCHIVE_EOF) {
                return FLASHUX_ERR_CORRUPT_IMAGE; /* Exactly one regular image per archive. */
            }
            stream->finished = true;
            break;
        }
        *count += (size_t)got;
    }
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_image_decoded_size(int fd, uint64_t maximum, uint64_t *size,
                                         bool (*cancel)(void *), void *cancel_data,
                                         flashux_progress_cb progress_cb, void *progress_data)
{
    flashux_image_stream_t *stream = NULL;
    unsigned char buffer[65536];
    uint64_t total = 0;
    uint64_t began = flashux_monotonic_nanos();
    uint64_t last_report = began;
    flashux_progress_t progress = {.stage = FLASHUX_STAGE_VERIFYING_LOCK};
    (void)snprintf(progress.stage_description, sizeof(progress.stage_description),
                   "Checking decoded image capacity");
    if (size == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *size = 0;
    flashux_error_t result = flashux_image_stream_open(fd, &stream);
    while (result == FLASHUX_SUCCESS) {
        if (cancel != NULL && cancel(cancel_data)) { result = FLASHUX_ERR_CANCELLED; break; }
        size_t count = 0;
        result = flashux_image_stream_read(stream, buffer, sizeof(buffer), &count);
        if (result != FLASHUX_SUCCESS || count == 0) { break; }
        if ((uint64_t)count > maximum - total) { result = FLASHUX_ERR_INVALID_ARG; break; }
        total += count;
        uint64_t now = flashux_monotonic_nanos();
        if (progress_cb != NULL && now - last_report >= UINT64_C(250000000)) {
            (void)flashux_progress_update(&progress, total, maximum, now - began,
                                          progress.bytes_processed, progress.elapsed_nanos);
            progress_cb(&progress, progress_data);
            last_report = now;
        }
    }
    flashux_image_stream_close(&stream);
    if (result == FLASHUX_SUCCESS && total == 0) { result = FLASHUX_ERR_CORRUPT_IMAGE; }
    if (result == FLASHUX_SUCCESS) { *size = total; }
    return result;
}
