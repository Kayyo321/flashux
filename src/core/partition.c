#include "partition_internal.h"
#include "flashux/device.h"
#include "flashux_config.h"
#include <openssl/rand.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#if FLASHUX_LINUX
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

static void put32(unsigned char *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        out[i] = (unsigned char)(value >> (8 * i));
    }
}
static void put64(unsigned char *out, uint64_t value)
{
    for (unsigned i = 0; i < 8; ++i) {
        out[i] = (unsigned char)(value >> (8 * i));
    }
}
uint32_t flashux_crc32(const unsigned char *data, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1) != 0 ? UINT32_C(0xedb88320) : 0);
        }
    }
    return ~crc;
}
static flashux_error_t write_at(int fd, const unsigned char *data, size_t size,
                                uint64_t offset)
{
    while (size > 0) {
        ssize_t count = pwrite(fd, data, size, (off_t)offset);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return FLASHUX_ERR_IO;
        }
        data += (size_t)count;
        size -= (size_t)count;
        offset += (uint64_t)count;
    }
    return FLASHUX_SUCCESS;
}
static bool parse_guid(const char *text, unsigned char *out)
{
    unsigned char bytes[16];
    size_t index = 0;
    if (strlen(text) != 36) {
        return false;
    }
    for (size_t i = 0; i < 36;) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (text[i++] != '-') {
                return false;
            }
            continue;
        }
        unsigned value = 0;
        for (unsigned j = 0; j < 2; ++j) {
            unsigned char c = (unsigned char)text[i++];
            unsigned digit;
            if (c >= '0' && c <= '9') {
                digit = (unsigned)(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                digit = (unsigned)(c - 'a') + 10;
            } else if (c >= 'A' && c <= 'F') {
                digit = (unsigned)(c - 'A') + 10;
            } else {
                return false;
            }
            value = value * 16 + digit;
        }
        bytes[index++] = (unsigned char)value;
    }
    static const unsigned order[] = {3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
    for (unsigned i = 0; i < 16; ++i) {
        out[i] = bytes[order[i]];
    }
    return true;
}
static uint8_t mbr_type(flashux_fs_type_t type)
{
    switch (type) {
    case FLASHUX_FS_FAT16: return 0x0e;
    case FLASHUX_FS_FAT32: return 0x0c;
    case FLASHUX_FS_EXT4: return 0x83;
    default: return 0x07;
    }
}
static void mbr_entry(unsigned char *entry, uint8_t type, uint64_t start,
                       uint64_t count, bool bootable)
{
    entry[0] = bootable ? 0x80 : 0;
    entry[1] = entry[5] = 0xfe;
    entry[2] = entry[3] = entry[6] = entry[7] = 0xff;
    entry[4] = type;
    put32(entry + 8, (uint32_t)start);
    put32(entry + 12, count > UINT32_MAX ? UINT32_MAX : (uint32_t)count);
}
static void gpt_header(unsigned char *out, uint64_t current, uint64_t backup,
                       uint64_t first, uint64_t last, uint64_t array_lba,
                       const unsigned char *guid, uint32_t array_crc)
{
    memcpy(out, "EFI PART", 8);
    put32(out + 8, 0x00010000);
    put32(out + 12, 92);
    put64(out + 24, current);
    put64(out + 32, backup);
    put64(out + 40, first);
    put64(out + 48, last);
    memcpy(out + 56, guid, 16);
    put64(out + 72, array_lba);
    put32(out + 80, 128);
    put32(out + 84, 128);
    put32(out + 88, array_crc);
    put32(out + 16, flashux_crc32(out, 92));
}

flashux_error_t flashux_partition_write_fd(int fd, const flashux_partition_table_t *table)
{
    unsigned char mbr[4096] = {0};
    unsigned char primary[4096] = {0};
    unsigned char backup[4096] = {0};
    unsigned char entries[16384] = {0};
    unsigned char disk_guid[16];
    struct stat st;
    if (fd < 0 || table == NULL || table->entry_count == 0 || table->entry_count > 16 ||
        (table->sector_size != 512 && table->sector_size != 4096) ||
        table->scheme < FLASHUX_PARTITION_SCHEME_MBR ||
        table->scheme > FLASHUX_PARTITION_SCHEME_HYBRID ||
        table->total_sectors > (uint64_t)INT64_MAX / table->sector_size ||
        fstat(fd, &st) != 0) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    uint64_t array_sectors = sizeof(entries) / table->sector_size;
    uint64_t first = table->scheme == FLASHUX_PARTITION_SCHEME_MBR ? 1 : 2 + array_sectors;
    if (table->total_sectors <= 2 * array_sectors + 3) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    uint64_t last = table->scheme == FLASHUX_PARTITION_SCHEME_MBR ?
        table->total_sectors - 1 : table->total_sectors - array_sectors - 2;
    if (S_ISREG(st.st_mode) && ((uint64_t)st.st_size <
        table->total_sectors * table->sector_size)) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < table->entry_count; ++i) {
        const flashux_partition_entry_t *part = &table->entries[i];
        if (part->partition_number == 0 || part->partition_number > 16 ||
            part->start_lba < first || part->sector_count == 0 || part->start_lba > last ||
            part->sector_count - 1 > last - part->start_lba ||
            part->start_lba % (1048576 / table->sector_size) != 0 ||
            memchr(part->label, '\0', sizeof(part->label)) == NULL ||
            memchr(part->type_guid, '\0', sizeof(part->type_guid)) == NULL ||
            part->fs_type < FLASHUX_FS_FAT16 || part->fs_type > FLASHUX_FS_UDF) {
            return FLASHUX_ERR_INVALID_ARG;
        }
        if ((table->scheme == FLASHUX_PARTITION_SCHEME_MBR ||
             table->scheme == FLASHUX_PARTITION_SCHEME_HYBRID) &&
            (part->start_lba > UINT32_MAX || part->sector_count > UINT32_MAX ||
             part->sector_count - 1 > UINT32_MAX - part->start_lba)) {
            return FLASHUX_ERR_INVALID_ARG;
        }
        for (size_t j = 0; j < i; ++j) {
            const flashux_partition_entry_t *other = &table->entries[j];
            if (part->partition_number == other->partition_number ||
                (part->start_lba < other->start_lba + other->sector_count &&
                 other->start_lba < part->start_lba + part->sector_count)) {
                return FLASHUX_ERR_INVALID_ARG;
            }
        }
        if (table->scheme == FLASHUX_PARTITION_SCHEME_MBR) {
            if (part->partition_number > 4) {
                return FLASHUX_ERR_INVALID_ARG;
            }
            mbr_entry(mbr + 446 + 16 * (part->partition_number - 1),
                      mbr_type(part->fs_type), part->start_lba, part->sector_count,
                      part->is_bootable);
        } else {
            unsigned char *entry = entries + 128 * (part->partition_number - 1);
            const char *guid = part->type_guid[0] != '\0' ? part->type_guid :
                (part->is_bootable ? "c12a7328-f81f-11d2-ba4b-00a0c93ec93b" :
                 part->fs_type == FLASHUX_FS_EXT4 ? "0fc63daf-8483-4772-8e79-3d69d8477de4" :
                 "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7");
            if (!parse_guid(guid, entry) || RAND_bytes(entry + 16, 16) != 1) {
                return FLASHUX_ERR_INVALID_ARG;
            }
            entry[22] = (unsigned char)((entry[22] & 15) | 0x40);
            entry[24] = (unsigned char)((entry[24] & 63) | 0x80);
            put64(entry + 32, part->start_lba);
            put64(entry + 40, part->start_lba + part->sector_count - 1);
            for (size_t j = 0; part->label[j] != '\0' && j < 36; ++j) {
                if ((unsigned char)part->label[j] > 127) {
                    return FLASHUX_ERR_INVALID_ARG;
                }
                entry[56 + j * 2] = (unsigned char)part->label[j];
            }
            if (table->scheme == FLASHUX_PARTITION_SCHEME_HYBRID) {
                if (i >= 3) {
                    return FLASHUX_ERR_INVALID_ARG;
                }
                mbr_entry(mbr + 462 + 16 * i, mbr_type(part->fs_type), part->start_lba,
                          part->sector_count, part->is_bootable);
            }
        }
    }
    mbr[510] = 0x55;
    mbr[511] = 0xaa;
    flashux_error_t result;
    if (table->scheme != FLASHUX_PARTITION_SCHEME_MBR) {
        mbr_entry(mbr + 446, 0xee, 1, table->total_sectors - 1, false);
        if (RAND_bytes(disk_guid, 16) != 1) {
            return FLASHUX_ERR_UNKNOWN;
        }
        uint32_t crc = flashux_crc32(entries, sizeof(entries));
        gpt_header(primary, 1, table->total_sectors - 1, first, last, 2, disk_guid, crc);
        gpt_header(backup, table->total_sectors - 1, 1, first, last,
                   table->total_sectors - 1 - array_sectors, disk_guid, crc);
        result = write_at(fd, entries, sizeof(entries), 2 * table->sector_size);
        if (result != FLASHUX_SUCCESS) { return result; }
        result = write_at(fd, entries, sizeof(entries),
                          (table->total_sectors - 1 - array_sectors) * table->sector_size);
        if (result != FLASHUX_SUCCESS) { return result; }
        result = write_at(fd, primary, table->sector_size, table->sector_size);
        if (result != FLASHUX_SUCCESS) { return result; }
        result = write_at(fd, backup, table->sector_size,
                          (table->total_sectors - 1) * table->sector_size);
        if (result != FLASHUX_SUCCESS) { return result; }
    }
    result = write_at(fd, mbr, table->sector_size, 0);
    return result == FLASHUX_SUCCESS && fsync(fd) != 0 ? FLASHUX_ERR_IO : result;
}

flashux_error_t flashux_partition_table_create(flashux_context_t *ctx,
                                              const flashux_partition_table_t *table)
{
    flashux_device_t device = {0};
    int fd = -1;
    if (ctx == NULL || table == NULL ||
        memchr(table->device_path, '\0', sizeof(table->device_path)) == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    memcpy(device.node_path, table->device_path, sizeof(device.node_path));
    flashux_error_t result = flashux_device_lock(ctx, &device, &fd);
    if (result == FLASHUX_SUCCESS) {
        result = flashux_partition_write_fd(fd, table);
#if FLASHUX_LINUX
        if (result == FLASHUX_SUCCESS && ioctl(fd, BLKRRPART) != 0) {
            result = FLASHUX_ERR_IO;
        }
#endif
    }
    flashux_device_unlock(&fd);
    if (result != FLASHUX_SUCCESS) {
        flashux_log_error(ctx, "Partition table failed: %s", flashux_strerror(result));
    }
    return result;
}
