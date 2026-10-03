#include "cli_args.h"
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

static bool copy_arg(char *out, size_t size, const char *arg)
{
    if (arg == NULL || strlen(arg) >= size) {
        return false;
    }
    memcpy(out, arg, strlen(arg) + 1);
    return true;
}
static bool number(const char *text, uint64_t *out)
{
    char *end;
    if (text == NULL || text[0] < '0' || text[0] > '9') {
        return false;
    }
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    uint64_t factor = 1;
    if (*end != '\0') {
        if (end[1] != '\0') {
            return false;
        }
        switch (*end) {
        case 'K':
        case 'k':
            factor = 1024;
            break;
        case 'M':
        case 'm':
            factor = 1024 * 1024;
            break;
        case 'G':
        case 'g':
            factor = UINT64_C(1024) * 1024 * 1024;
            break;
        default:
            return false;
        }
    }
    if (errno != 0 || value > UINT64_MAX / factor) {
        return false;
    }
    *out = (uint64_t)value * factor;
    return true;
}
int flashux_cli_parse_args(int argc, char **argv, flashux_cli_options_t *opts)
{
    enum {
        OPT_MODE = 256,
        OPT_SCHEME,
        OPT_TARGET,
        OPT_FS,
        OPT_CLUSTER,
        OPT_BAD,
        OPT_PERSIST,
        OPT_FIXED,
        OPT_DRY,
        OPT_TPM,
        OPT_ACCOUNT,
        OPT_LOCAL,
        OPT_REGIONAL,
        OPT_TELEMETRY,
        OPT_BITLOCKER
    };
    static const struct option options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"image", required_argument, NULL, 'i'},
        {"device", required_argument, NULL, 'd'},
        {"label", required_argument, NULL, 'l'},
        {"batch", no_argument, NULL, 'b'},
        {"interactive", no_argument, NULL, 't'},
        {"list-devices", no_argument, NULL, 'L'},
        {"yes", no_argument, NULL, 'y'},
        {"mode", required_argument, NULL, OPT_MODE},
        {"partition-scheme", required_argument, NULL, OPT_SCHEME},
        {"target-system", required_argument, NULL, OPT_TARGET},
        {"filesystem", required_argument, NULL, OPT_FS},
        {"cluster-size", required_argument, NULL, OPT_CLUSTER},
        {"bad-blocks", required_argument, NULL, OPT_BAD},
        {"persistence-size", required_argument, NULL, OPT_PERSIST},
        {"include-fixed", no_argument, NULL, OPT_FIXED},
        {"dry-run", no_argument, NULL, OPT_DRY},
        {"bypass-tpm", no_argument, NULL, OPT_TPM},
        {"bypass-online-account", no_argument, NULL, OPT_ACCOUNT},
        {"local-account", required_argument, NULL, OPT_LOCAL},
        {"match-regional-options", no_argument, NULL, OPT_REGIONAL},
        {"disable-telemetry", no_argument, NULL, OPT_TELEMETRY},
        {"disable-bitlocker", no_argument, NULL, OPT_BITLOCKER},
        {NULL, 0, NULL, 0}};
    if (argc < 1 || argv == NULL || opts == NULL) {
        return -1;
    }
    memset(opts, 0, sizeof(*opts));
    opts->fs_type = FLASHUX_FS_FAT32;
    opts->part_scheme = FLASHUX_PARTITION_SCHEME_GPT;
    opts->target_sys = FLASHUX_TARGET_UEFI_NON_CSM;
    opts->interactive = argc == 1;
    opterr = 0;
#if defined(__APPLE__)
    optreset = 1;
    optind = 1;
#else
    optind = 0;
#endif
    int option;
    while ((option = getopt_long(argc, argv, "hvi:d:l:btLy", options, NULL)) != -1) {
        uint64_t value;
        switch (option) {
        case 'h':
            opts->help = true;
            break;
        case 'v':
            opts->version = true;
            break;
        case 'i':
            if (!copy_arg(opts->image_path, sizeof(opts->image_path), optarg)) {
                return -1;
            }
            break;
        case 'd':
            if (!copy_arg(opts->device_path, sizeof(opts->device_path), optarg)) {
                return -1;
            }
            break;
        case 'l':
            if (!copy_arg(opts->volume_label, sizeof(opts->volume_label), optarg)) {
                return -1;
            }
            break;
        case 'b':
            opts->batch_mode = true;
            break;
        case 't':
            opts->interactive = true;
            break;
        case 'L':
            opts->list_devices = true;
            break;
        case 'y':
            opts->yes = true;
            break;
        case OPT_FIXED:
            opts->include_fixed = true;
            break;
        case OPT_DRY:
            opts->dry_run = true;
            break;
        case OPT_MODE:
            if (strcmp(optarg, "dd") == 0) {
                opts->mode = FLASHUX_MODE_DD_RAW;
            } else if (strcmp(optarg, "iso") == 0) {
                opts->mode = FLASHUX_MODE_ISO_EXTRACT;
            } else if (strcmp(optarg, "wtg") == 0) {
                opts->mode = FLASHUX_MODE_WINDOWS_TO_GO;
            } else if (strcmp(optarg, "freedos") == 0) {
                opts->mode = FLASHUX_MODE_FREEDOS;
            } else {
                return -1;
            }
            break;
        case OPT_SCHEME:
            if (strcmp(optarg, "mbr") == 0) {
                opts->part_scheme = FLASHUX_PARTITION_SCHEME_MBR;
            } else if (strcmp(optarg, "gpt") == 0) {
                opts->part_scheme = FLASHUX_PARTITION_SCHEME_GPT;
            } else if (strcmp(optarg, "hybrid") == 0) {
                opts->part_scheme = FLASHUX_PARTITION_SCHEME_HYBRID;
            } else {
                return -1;
            }
            break;
        case OPT_TARGET:
            if (strcmp(optarg, "bios") == 0) {
                opts->target_sys = FLASHUX_TARGET_BIOS_CSM;
            } else if (strcmp(optarg, "uefi") == 0) {
                opts->target_sys = FLASHUX_TARGET_UEFI_NON_CSM;
            } else if (strcmp(optarg, "uefi-ntfs") == 0) {
                opts->target_sys = FLASHUX_TARGET_UEFI_NTFS;
            } else {
                return -1;
            }
            break;
        case OPT_FS:
            if (strcmp(optarg, "fat16") == 0) {
                opts->fs_type = FLASHUX_FS_FAT16;
            } else if (strcmp(optarg, "fat32") == 0) {
                opts->fs_type = FLASHUX_FS_FAT32;
            } else if (strcmp(optarg, "ntfs") == 0) {
                opts->fs_type = FLASHUX_FS_NTFS;
            } else if (strcmp(optarg, "exfat") == 0) {
                opts->fs_type = FLASHUX_FS_EXFAT;
            } else if (strcmp(optarg, "ext4") == 0) {
                opts->fs_type = FLASHUX_FS_EXT4;
            } else if (strcmp(optarg, "udf") == 0) {
                opts->fs_type = FLASHUX_FS_UDF;
            } else {
                return -1;
            }
            break;
        case OPT_CLUSTER:
            if (!number(optarg, &value) || value > UINT32_MAX) {
                return -1;
            }
            opts->cluster_size = (uint32_t)value;
            break;
        case OPT_BAD:
            if (!number(optarg, &value) || (value != 0 && value != 1 && value != 2 && value != 4)) {
                return -1;
            }
            opts->bad_blocks_passes = (uint32_t)value;
            break;
        case OPT_PERSIST:
            if (!number(optarg, &opts->persistence_size_bytes)) {
                return -1;
            }
            break;
        case OPT_TPM:
            opts->win_custom.bypass_tpm_secureboot_ram = true;
            break;
        case OPT_ACCOUNT:
            opts->win_custom.bypass_online_account = true;
            break;
        case OPT_LOCAL:
            if (!copy_arg(opts->win_custom.local_username, sizeof(opts->win_custom.local_username),
                          optarg) ||
                optarg[0] == '\0') {
                return -1;
            }
            opts->win_custom.create_local_account = true;
            break;
        case OPT_REGIONAL:
            opts->win_custom.match_regional_options = true;
            break;
        case OPT_TELEMETRY:
            opts->win_custom.disable_telemetry = true;
            break;
        case OPT_BITLOCKER:
            opts->win_custom.disable_bitlocker = true;
            break;
        default:
            return -1;
        }
    }
    if (optind != argc || (opts->interactive && opts->batch_mode)) {
        return -1;
    }
    if (!opts->help && !opts->version && !opts->list_devices && !opts->interactive &&
        (opts->image_path[0] == '\0' || opts->device_path[0] == '\0')) {
        return -1;
    }
    if (!opts->interactive) {
        opts->batch_mode = true;
    }
    return 0;
}
void flashux_cli_to_flash(const flashux_cli_options_t *opts, flashux_flash_options_t *flash)
{
    memset(flash, 0, sizeof(*flash));
    memcpy(flash->target_device_path, opts->device_path, sizeof(flash->target_device_path));
    memcpy(flash->source_image_path, opts->image_path, sizeof(flash->source_image_path));
    memcpy(flash->volume_label, opts->volume_label, sizeof(flash->volume_label));
    flash->mode = opts->mode;
    flash->part_scheme = opts->part_scheme;
    flash->target_sys = opts->target_sys;
    flash->fs_type = opts->fs_type;
    flash->quick_format = true;
    flash->cluster_size = opts->cluster_size;
    flash->bad_blocks_passes = opts->bad_blocks_passes;
    flash->persistence_size_bytes = opts->persistence_size_bytes;
    flash->win_custom = opts->win_custom;
    flash->dry_run = opts->dry_run;
}
