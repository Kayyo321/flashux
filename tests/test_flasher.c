#include "test.h"
#include "core/progress.h"
#include "helper/privileged_ops.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
struct observer {
    flashux_context_t *ctx;
    bool cancel;
};
static void cancel_progress(const flashux_progress_t *progress, void *data)
{
    struct observer *observer = data;
    if (progress->stage == FLASHUX_STAGE_WRITING) {
        observer->cancel = true;
        if (observer->ctx != NULL) { CHECK(flashux_flash_cancel(observer->ctx) == FLASHUX_SUCCESS); }
    }
}
static bool cancel_requested(void *data)
{
    return ((struct observer *)data)->cancel;
}
int main(void)
{
    flashux_progress_t metrics = {0};
    CHECK(flashux_progress_update(&metrics, 100, 1000, 1000000000, 0, 0) == FLASHUX_SUCCESS);
    CHECK(metrics.percent_complete == 10.0 && metrics.eta_seconds == 9);
    CHECK(metrics.current_speed_bps == 100.0 && metrics.average_speed_bps == 100.0);
    CHECK(flashux_progress_update(&metrics, 300, 1000, 2000000000, 100, 1000000000)
          == FLASHUX_SUCCESS);
    CHECK(metrics.current_speed_bps == 125.0 && metrics.eta_seconds == 6);
    CHECK(flashux_progress_update(&metrics, 1001, 1000, 0, 0, 0) == FLASHUX_ERR_INVALID_ARG);
    metrics.current_speed_bps = 0;
    CHECK(flashux_progress_update(&metrics, 0, 0, 0, 0, 0) == FLASHUX_SUCCESS);
    CHECK(isfinite(metrics.average_speed_bps) && metrics.eta_seconds == 0);
    char image_path[] = "/tmp/flashux-source-XXXXXX";
    char target_path[] = "/tmp/flashux-target-XXXXXX";
    int image = mkstemp(image_path);
    int target = mkstemp(target_path);
    CHECK(image >= 0 && target >= 0);
    const off_t length = 3 * 1024 * 1024 + 37;
    CHECK(ftruncate(image, length) == 0 && ftruncate(target, length + 8192) == 0);
    unsigned char mark = 0xad;
    CHECK(pwrite(image, &mark, 1, length - 1) == 1);
    CHECK(pwrite(target, &mark, 1, length + 8191) == 1);
    struct observer observer = {NULL, false};
    CHECK(flashux_raw_copy(image, target, 4096, NULL, NULL, cancel_requested, &observer)
          == FLASHUX_SUCCESS);
    unsigned char got = 0;
    CHECK(pread(target, &got, 1, length - 1) == 1 && got == mark);
    CHECK(pread(target, &got, 1, length) == 1 && got == 0);
    CHECK(pread(target, &got, 1, length + 8191) == 1 && got == mark);
    observer.cancel = true;
    CHECK(flashux_raw_copy(image, target, 512, NULL, NULL, cancel_requested, &observer)
          == FLASHUX_ERR_CANCELLED);
    CHECK(flashux_raw_copy(image, image, 512, NULL, NULL, NULL, NULL)
          == FLASHUX_ERR_INVALID_ARG);
    CHECK(flashux_context_new(&observer.ctx) == FLASHUX_SUCCESS);
    flashux_flash_options_t options = {0};
    options.dry_run = true;
    (void)snprintf(options.source_image_path, sizeof(options.source_image_path), "%s", image_path);
    (void)snprintf(options.target_device_path, sizeof(options.target_device_path), "%s", target_path);
    CHECK(flashux_flash_validate_options(observer.ctx, &options, NULL, 0) == FLASHUX_SUCCESS);
    observer.cancel = false;
    CHECK(flashux_flash_start_async(observer.ctx, &options, cancel_progress, &observer)
          == FLASHUX_SUCCESS);
    CHECK(flashux_flash_wait(observer.ctx) == FLASHUX_ERR_CANCELLED);
    CHECK(!flashux_flash_is_running(observer.ctx));
    CHECK(flashux_flash_start_async(observer.ctx, &options, NULL, NULL) == FLASHUX_SUCCESS);
    CHECK(flashux_flash_wait(observer.ctx) == FLASHUX_SUCCESS);
    options.mode = FLASHUX_MODE_ISO_EXTRACT;
    CHECK(flashux_flash_validate_options(observer.ctx, &options, NULL, 0) == FLASHUX_ERR_UNSUPPORTED);
    CHECK(pread(target, &got, 1, length + 8191) == 1 && got == mark);
    flashux_context_free(&observer.ctx);
    CHECK(close(image) == 0 && close(target) == 0);
    CHECK(unlink(image_path) == 0 && unlink(target_path) == 0);
    return 0;
}
