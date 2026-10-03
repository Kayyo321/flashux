#include "test.h"
#include "core/image_stream.h"
#include "helper/privileged_ops.h"
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static const unsigned char payload[] = "decoded image data with a final unaligned sector";

static void archive_image(int fd, unsigned filter, bool multiple)
{
    CHECK(ftruncate(fd, 0) == 0 && lseek(fd, 0, SEEK_SET) == 0);
    struct archive *writer = archive_write_new();
    CHECK(writer != NULL);
    if (filter == 4) {
        CHECK(archive_write_set_format_zip(writer) == ARCHIVE_OK);
    } else {
        CHECK(archive_write_set_format_raw(writer) == ARCHIVE_OK);
        int result = filter == 0   ? archive_write_add_filter_gzip(writer)
                     : filter == 1 ? archive_write_add_filter_xz(writer)
                     : filter == 2 ? archive_write_add_filter_bzip2(writer)
                                   : archive_write_add_filter_zstd(writer);
        CHECK(result == ARCHIVE_OK);
    }
    CHECK(archive_write_set_bytes_per_block(writer, 0) == ARCHIVE_OK);
    CHECK(archive_write_open_fd(writer, fd) == ARCHIVE_OK);
    for (unsigned i = 0; i < (multiple ? 2U : 1U); ++i) {
        struct archive_entry *entry = archive_entry_new();
        CHECK(entry != NULL);
        archive_entry_set_pathname(entry, i == 0 ? "image.img" : "extra.img");
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0600);
        archive_entry_set_size(entry, (la_int64_t)sizeof(payload));
        CHECK(archive_write_header(writer, entry) == ARCHIVE_OK);
        CHECK(archive_write_data(writer, payload, sizeof(payload)) == (la_ssize_t)sizeof(payload));
        archive_entry_free(entry);
        entry = NULL;
    }
    CHECK(archive_write_close(writer) == ARCHIVE_OK);
    CHECK(archive_write_free(writer) == ARCHIVE_OK);
    writer = NULL;
}
int main(void)
{
    char image_path[] = "/tmp/flashux-archive-XXXXXX";
    char target_path[] = "/tmp/flashux-decoded-XXXXXX";
    int source = mkstemp(image_path);
    int target = mkstemp(target_path);
    CHECK(source >= 0 && target >= 0 && ftruncate(target, 8192) == 0);
    for (unsigned filter = 0; filter < 5; ++filter) {
        archive_image(source, filter, false);
        uint64_t size = 0;
        CHECK(flashux_image_decoded_size(source, 8192, &size, NULL, NULL, NULL, NULL) ==
                  FLASHUX_SUCCESS &&
              size == sizeof(payload));
        CHECK(flashux_raw_copy(source, target, 4096, NULL, NULL, NULL, NULL) == FLASHUX_SUCCESS);
        unsigned char decoded[sizeof(payload)] = {0};
        CHECK(pread(target, decoded, sizeof(decoded), 0) == (ssize_t)sizeof(decoded));
        CHECK(memcmp(decoded, payload, sizeof(payload)) == 0);
        CHECK(flashux_image_decoded_size(source, sizeof(payload) - 1, &size, NULL, NULL, NULL,
                                         NULL) == FLASHUX_ERR_INVALID_ARG &&
              size == 0);
        CHECK(ftruncate(target, 32) == 0);
        unsigned char sentinel = 0xad;
        CHECK(pwrite(target, &sentinel, 1, 0) == 1);
        CHECK(flashux_raw_copy(source, target, 512, NULL, NULL, NULL, NULL) ==
              FLASHUX_ERR_INVALID_ARG);
        CHECK(pread(target, decoded, 1, 0) == 1 && decoded[0] == sentinel);
        CHECK(ftruncate(target, 8192) == 0);
        flashux_context_t *ctx = NULL;
        CHECK(flashux_context_new(&ctx) == FLASHUX_SUCCESS);
        flashux_flash_options_t options = {.dry_run = true};
        (void)snprintf(options.source_image_path, sizeof(options.source_image_path), "%s",
                       image_path);
        (void)snprintf(options.target_device_path, sizeof(options.target_device_path), "%s",
                       target_path);
        CHECK(flashux_flash_start_async(ctx, &options, NULL, NULL) == FLASHUX_SUCCESS);
        CHECK(flashux_flash_wait(ctx) == FLASHUX_SUCCESS);
        CHECK(pread(target, decoded, 1, 0) == 1 && decoded[0] == sentinel);
        flashux_context_free(&ctx);
    }
    archive_image(source, 4, true);
    CHECK(flashux_raw_copy(source, target, 512, NULL, NULL, NULL, NULL) ==
          FLASHUX_ERR_CORRUPT_IMAGE);
    unsigned char mark;
    CHECK(pread(target, &mark, 1, 0) == 1 && mark == 0xad);
    archive_image(source, 1, false);
    CHECK(ftruncate(source, 15) == 0);
    CHECK(flashux_raw_copy(source, target, 512, NULL, NULL, NULL, NULL) ==
          FLASHUX_ERR_CORRUPT_IMAGE);
    CHECK(pread(target, &mark, 1, 0) == 1 && mark == 0xad);
    CHECK(close(source) == 0 && close(target) == 0);
    CHECK(unlink(image_path) == 0 && unlink(target_path) == 0);
    return 0;
}
