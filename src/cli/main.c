#include "cli_runtime.h"
#include <stdio.h>
#include <locale.h>
volatile sig_atomic_t flashux_cli_interrupted = 0;
static void interrupt_cli(int signal_number)
{
    (void)signal_number;
    flashux_cli_interrupted = 1;
}
static void usage(void)
{
    (void)puts("Flashux 0.1.0 - native Linux boot media foundation\n"
        "Usage: flashux-cli --image IMAGE --device TARGET [--batch] [--yes | --dry-run]\n"
        "       flashux-cli --list-devices [--include-fixed]\n"
        "       flashux-cli --interactive [--image IMAGE] [--device TARGET]\n"
        "Options: --mode dd|iso|wtg|freedos  --partition-scheme mbr|gpt|hybrid\n"
        "         --target-system bios|uefi|uefi-ntfs\n"
        "         --filesystem fat16|fat32|ntfs|exfat|ext4|udf --cluster-size BYTES\n"
        "         --label LABEL --bad-blocks 0|1|2|4 --persistence-size BYTES\n"
        "         --bypass-tpm --bypass-online-account --local-account USER\n"
        "         --match-regional-options --disable-telemetry --disable-bitlocker\n"
        "         --help --version\n"
        "Raw jobs copy the image layout; partition/format selections apply to later ISO modes.\n"
        "This foundation rejects ISO extraction, compressed images and setup customizations.\n"
        "Dry runs read the image and leave the target untouched; regular-file targets are allowed.");
}
int main(int argc, char **argv)
{
    flashux_cli_options_t options;
    flashux_context_t *ctx = NULL;
    struct sigaction action = {0};
    (void)setlocale(LC_ALL, "");
    (void)setlocale(LC_NUMERIC, "C");
    if (flashux_cli_parse_args(argc, argv, &options) != 0) {
        flashux_log_error(NULL, "%s", "Invalid arguments; use --help");
        return 2;
    }
    if (options.help) { usage(); return 0; }
    if (options.version) { (void)puts("flashux 0.1.0 (GPL-3.0)"); return 0; }
    action.sa_handler = interrupt_cli;
    (void)sigemptyset(&action.sa_mask);
    (void)sigaction(SIGINT, &action, NULL);
    (void)sigaction(SIGTERM, &action, NULL);
    if (flashux_context_new(&ctx) != FLASHUX_SUCCESS) {
        flashux_log_error(NULL, "%s", "Unable to allocate context");
        return 1;
    }
    int result = options.list_devices ? flashux_cli_list_devices(ctx, options.include_fixed) :
        options.interactive ? flashux_cli_run_interactive(ctx, &options) :
        flashux_cli_run_batch(ctx, &options);
    flashux_context_free(&ctx);
    return result;
}
