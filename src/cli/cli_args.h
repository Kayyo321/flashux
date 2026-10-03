#ifndef FLASHUX_CLI_ARGS_H
#define FLASHUX_CLI_ARGS_H

#include "flashux/flasher.h"

/**
 * @brief Parsed command-line flags and runtime options.
 */
typedef struct {
    bool interactive;          /* curses TUI mode */
    bool batch_mode;           /* Non-interactive JSON output */
    bool list_devices;         /* Enumerate devices and exit */
    bool include_fixed;        /* Include non-removable drives */
    char image_path[512];      /* Image path */
    char device_path[64];      /* Target device */
    char volume_label[128];    /* Label */
    flashux_flash_mode_t mode; /* Flash mode */
    flashux_partition_scheme_t part_scheme;
    flashux_target_system_t target_sys;
    flashux_fs_type_t fs_type;
    uint32_t bad_blocks_passes; /* Surface scan passes */
    bool yes;
    bool help;
    bool version;
    uint32_t cluster_size;
    uint64_t persistence_size_bytes;
    flashux_windows_customizations_t win_custom;
    bool dry_run; /* Test mode */
} flashux_cli_options_t;

/**
 * @brief Parse command-line arguments using getopt_long.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector.
 * @param[out] opts Struct populated with parsed CLI flags.
 * @return 0 on success, negative integer or exit code on parsing failure.
 */
int flashux_cli_parse_args(int argc, char **argv, flashux_cli_options_t *opts);

/**
 * @brief Run interactive ncurses terminal dashboard.
 *
 * @param[in] ctx Flashux context.
 * @param[in] opts Runtime options.
 * @return 0 on normal exit, non-zero on error.
 */
int flashux_cli_run_interactive(flashux_context_t *ctx, const flashux_cli_options_t *opts);

/**
 * @brief Run non-interactive scriptable batch mode emitting structured JSON.
 *
 * @param[in] ctx Flashux context.
 * @param[in] opts Runtime options.
 * @return 0 on successful flash, non-zero on error.
 */
int flashux_cli_run_batch(flashux_context_t *ctx, const flashux_cli_options_t *opts);

void flashux_cli_to_flash(const flashux_cli_options_t *opts, flashux_flash_options_t *flash);
#endif /* FLASHUX_CLI_ARGS_H */
