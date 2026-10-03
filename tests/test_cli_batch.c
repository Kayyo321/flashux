#include "test.h"
#include "cli/cli_runtime.h"
#include <unistd.h>
#include <string.h>
volatile sig_atomic_t flashux_cli_interrupted = 0;
int main(void)
{
    FILE *output = tmpfile();
    CHECK(output != NULL);
    int saved = dup(STDOUT_FILENO);
    CHECK(saved >= 0 && fflush(stdout) == 0 && dup2(fileno(output), STDOUT_FILENO) >= 0);
    flashux_cli_json_string("\"\\\nGreek: \xce\xa9 invalid: \xc0\xaf truncated: \xf0\x9f");
    CHECK(fflush(stdout) == 0 && fseek(output, 0, SEEK_SET) == 0);
    char line[1024];
    CHECK(fgets(line, sizeof(line), output) != NULL);
    CHECK(strcmp(line, "\"\\\"\\\\\\u000aGreek: \xce\xa9 invalid: \\ufffd\\ufffd truncated: "
                       "\\ufffd\\ufffd\"") == 0);
    CHECK(ftruncate(fileno(output), 0) == 0 && fseek(output, 0, SEEK_SET) == 0);
    char source_path[] = "/tmp/flashux-batch-source-XXXXXX";
    char target_path[] = "/tmp/flashux-batch-target-XXXXXX";
    int source = mkstemp(source_path);
    int target = mkstemp(target_path);
    CHECK(source >= 0 && target >= 0 && ftruncate(source, 65573) == 0 &&
          ftruncate(target, 131072) == 0);
    unsigned char mark = 0xad;
    CHECK(pwrite(target, &mark, 1, 0) == 1 && pwrite(target, &mark, 1, 131071) == 1);
    flashux_context_t *ctx = NULL;
    CHECK(flashux_context_new(&ctx) == FLASHUX_SUCCESS);
    flashux_cli_options_t options = {.batch_mode = true, .dry_run = true};
    (void)snprintf(options.image_path, sizeof(options.image_path), "%s", source_path);
    (void)snprintf(options.device_path, sizeof(options.device_path), "%s", target_path);
    CHECK(flashux_cli_run_batch(ctx, &options) == 0);
    CHECK(fflush(stdout) == 0 && fseek(output, 0, SEEK_SET) == 0);
    bool progress = false;
    bool result = false;
    while (fgets(line, sizeof(line), output) != NULL) {
        CHECK(line[0] == '{' && strlen(line) > 2 && line[strlen(line) - 2] == '}');
        if (strstr(line, "\"event\":\"progress\"") != NULL) {
            progress = true;
        }
        if (strstr(line, "\"event\":\"result\",\"code\":0") != NULL) {
            result = true;
        }
    }
    CHECK(progress && result);
    unsigned char got = 0;
    CHECK(pread(target, &got, 1, 0) == 1 && got == mark);
    CHECK(pread(target, &got, 1, 131071) == 1 && got == mark);
    options.dry_run = false;
    CHECK(flashux_cli_run_batch(ctx, &options) == 1);
    CHECK(fflush(stdout) == 0 && dup2(saved, STDOUT_FILENO) >= 0);
    CHECK(close(saved) == 0 && fclose(output) == 0);
    flashux_context_free(&ctx);
    CHECK(close(source) == 0 && close(target) == 0);
    CHECK(unlink(source_path) == 0 && unlink(target_path) == 0);
    return 0;
}
