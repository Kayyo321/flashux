#include "cli_runtime.h"
#include "flashux/device.h"
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
static size_t utf8_sequence(const unsigned char *text)
{
    unsigned char lead = text[0];
    size_t size = lead >= 0xc2 && lead <= 0xdf ? 2 :
        lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if (size == 0) { return 0; }
    uint32_t codepoint = lead & (size == 2 ? 0x1fU : size == 3 ? 0x0fU : 0x07U);
    for (size_t i = 1; i < size; ++i) {
        if (text[i] < 0x80 || text[i] > 0xbf) { return 0; }
        codepoint = (codepoint << 6) | (text[i] & 0x3fU);
    }
    if ((size == 3 && codepoint < 0x800) || (size == 4 && codepoint < 0x10000) ||
        (codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint > 0x10ffff) { return 0; }
    return size;
}
void flashux_cli_json_string(const char *text)
{
    (void)putchar('"');
    const unsigned char *p = (const unsigned char *)(text == NULL ? "" : text);
    while (*p != 0) {
        if (*p == '"' || *p == '\\') {
            (void)putchar('\\');
            (void)putchar(*p++);
        } else if (*p < 32) { (void)printf("\\u%04x", (unsigned)*p++); }
        else if (*p < 128) { (void)putchar(*p++); }
        else {
            size_t length = utf8_sequence(p);
            if (length == 0) { (void)fputs("\\ufffd", stdout); ++p; }
            else {
                for (size_t i = 0; i < length; ++i) { (void)putchar(*p++); }
            }
        }
    }
    (void)putchar('"');
}
static void result_record(flashux_error_t result)
{
    (void)printf("{\"event\":\"result\",\"code\":%d,\"message\":", (int)result);
    flashux_cli_json_string(flashux_strerror(result));
    (void)puts("}");
}
int flashux_cli_list_devices(flashux_context_t *ctx, bool include_fixed)
{
    flashux_device_t *list = NULL;
    flashux_error_t result = flashux_device_list_scan(ctx, include_fixed, &list);
    if (result == FLASHUX_SUCCESS) {
        for (flashux_device_t *device = list; device != NULL; device = device->next) {
            (void)printf("{\"event\":\"device\",\"path\":");
            flashux_cli_json_string(device->node_path);
            (void)printf(",\"model\":");
            flashux_cli_json_string(device->model);
            (void)printf(",\"size_bytes\":%" PRIu64 ",\"safe\":%s}\n", device->size_bytes,
                flashux_device_is_safe_target(ctx, device, NULL, 0) ? "true" : "false");
        }
    }
    flashux_device_list_free(&list);
    result_record(result);
    return result == FLASHUX_SUCCESS ? 0 : 1;
}
static void batch_progress(const flashux_progress_t *progress, void *data)
{
    (void)data;
    (void)printf("{\"event\":\"progress\",\"stage\":%d,\"bytes\":%" PRIu64
                 ",\"total_bytes\":%" PRIu64 ",\"percent\":%.3f,\"speed_bps\":%.3f,"
                 "\"eta_seconds\":%u,\"message\":", (int)progress->stage,
                 progress->bytes_processed, progress->total_bytes, progress->percent_complete,
                 progress->current_speed_bps, progress->eta_seconds);
    flashux_cli_json_string(progress->stage_description);
    (void)puts("}");
    (void)fflush(stdout);
}
int flashux_cli_run_batch(flashux_context_t *ctx, const flashux_cli_options_t *opts)
{
    flashux_flash_options_t options;
    if (!opts->dry_run && !opts->yes) {
        flashux_log_error(ctx, "%s", "Batch writes require --yes after reviewing the target");
        result_record(FLASHUX_ERR_PERMISSION_DENIED);
        return 1;
    }
    flashux_cli_to_flash(opts, &options);
    flashux_error_t result = flashux_flash_start_async(ctx, &options, batch_progress, NULL);
    if (result == FLASHUX_SUCCESS) {
        while (flashux_flash_is_running(ctx)) {
            if (flashux_cli_interrupted) { (void)flashux_flash_cancel(ctx); }
            struct timespec delay = {0, 50000000};
            (void)nanosleep(&delay, NULL);
        }
        result = flashux_flash_wait(ctx);
    }
    result_record(result);
    return result == FLASHUX_SUCCESS ? 0 : result == FLASHUX_ERR_CANCELLED ? 130 : 1;
}
