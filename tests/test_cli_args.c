#include "test.h"
#include "cli/cli_args.h"
#include <string.h>
int main(void)
{
    flashux_cli_options_t opts;
    char program[] = "flashux";
    char image[] = "--image";
    char source[] = "/tmp/image";
    char device[] = "--device";
    char target[] = "/tmp/target";
    char dry[] = "--dry-run";
    char *valid[] = {program, image, source, device, target, dry, NULL};
    CHECK(flashux_cli_parse_args(6, valid, &opts) == 0);
    CHECK(opts.dry_run && opts.batch_mode && !opts.interactive);
    flashux_flash_options_t flash;
    flashux_cli_to_flash(&opts, &flash);
    CHECK(strcmp(flash.source_image_path, source) == 0 && flash.quick_format);
    char batch[] = "--batch";
    char interactive[] = "--interactive";
    char *conflict[] = {program, batch, interactive, NULL};
    CHECK(flashux_cli_parse_args(3, conflict, &opts) != 0);
    char cluster[] = "--cluster-size";
    char huge[] = "18446744073709551616";
    char *overflow[] = {program, cluster, huge, NULL};
    CHECK(flashux_cli_parse_args(3, overflow, &opts) != 0);
    char help[] = "--help";
    char *help_args[] = {program, help, NULL};
    CHECK(flashux_cli_parse_args(2, help_args, &opts) == 0 && opts.help);
    char bad[] = "--bad-blocks";
    char three[] = "3";
    char *bad_args[] = {program, help, bad, three, NULL};
    CHECK(flashux_cli_parse_args(4, bad_args, &opts) != 0);
    char path[600];
    memset(path, 'a', sizeof(path));
    path[sizeof(path) - 1] = '\0';
    char *long_args[] = {program, image, path, device, target, NULL};
    CHECK(flashux_cli_parse_args(5, long_args, &opts) != 0);
    char persistence[] = "--persistence-size";
    char size[] = "2G";
    char *size_args[] = {program, help, persistence, size, NULL};
    CHECK(flashux_cli_parse_args(4, size_args, &opts) == 0);
    CHECK(opts.persistence_size_bytes == UINT64_C(2147483648));
    return 0;
}
