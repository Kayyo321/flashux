#include "test.h"
#include "core/partition_internal.h"
#include <unistd.h>
#include <string.h>
static uint32_t get32(const unsigned char *in)
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}
static void exercise(uint32_t sector_size)
{
    char path[] = "/tmp/flashux-partition-XXXXXX";
    int fd = mkstemp(path);
    unsigned char header[4096];
    unsigned char backup[4096];
    unsigned char array[16384];
    flashux_partition_table_t table = {0};
    CHECK(fd >= 0);
    CHECK(unlink(path) == 0);
    CHECK(ftruncate(fd, 16 * 1024 * 1024) == 0);
    table.scheme = FLASHUX_PARTITION_SCHEME_GPT;
    table.sector_size = sector_size;
    table.total_sectors = 16 * 1024 * 1024 / sector_size;
    table.entry_count = 1;
    table.entries[0].partition_number = 1;
    table.entries[0].start_lba = 1024 * 1024 / sector_size;
    table.entries[0].sector_count = 1024;
    table.entries[0].fs_type = FLASHUX_FS_FAT32;
    table.entries[0].is_bootable = true;
    CHECK(flashux_partition_write_fd(fd, &table) == FLASHUX_SUCCESS);
    CHECK(pread(fd, header, sector_size, sector_size) == (ssize_t)sector_size);
    CHECK(memcmp(header, "EFI PART", 8) == 0);
    uint32_t crc = get32(header + 16);
    memset(header + 16, 0, 4);
    CHECK(flashux_crc32(header, 92) == crc);
    CHECK(pread(fd, array, sizeof(array), 2 * sector_size) == (ssize_t)sizeof(array));
    CHECK(get32(header + 88) == flashux_crc32(array, sizeof(array)));
    static const unsigned char esp[] = {0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11};
    CHECK(memcmp(array, esp, sizeof(esp)) == 0);
    CHECK((array[23] & 0xf0) == 0x40 && (array[24] & 0xc0) == 0x80);
    CHECK((header[63] & 0xf0) == 0x40 && (header[64] & 0xc0) == 0x80);
    CHECK(pread(fd, backup, sector_size, (off_t)((table.total_sectors - 1) * sector_size)) ==
          (ssize_t)sector_size);
    CHECK(memcmp(backup + 56, header + 56, 16) == 0);
    CHECK(get32(backup + 88) == get32(header + 88));
    table.entry_count = 2;
    table.entries[1] = table.entries[0];
    table.entries[1].partition_number = 2;
    CHECK(flashux_partition_write_fd(fd, &table) == FLASHUX_ERR_INVALID_ARG);
    CHECK(pread(fd, backup, sector_size, sector_size) == (ssize_t)sector_size);
    memset(backup + 16, 0, 4);
    CHECK(memcmp(backup, header, sector_size) == 0);
    table.entry_count = 1;
    table.scheme = FLASHUX_PARTITION_SCHEME_MBR;
    CHECK(flashux_partition_write_fd(fd, &table) == FLASHUX_SUCCESS);
    CHECK(pread(fd, header, sector_size, 0) == (ssize_t)sector_size);
    CHECK(header[510] == 0x55 && header[511] == 0xaa);
    CHECK(header[446] == 0x80 && header[450] == 0x0c);
    CHECK(get32(header + 454) == table.entries[0].start_lba);
    CHECK(close(fd) == 0);
}
int main(void)
{
    CHECK(flashux_crc32((const unsigned char *)"123456789", 9) == UINT32_C(0xcbf43926));
    exercise(512);
    exercise(4096);
    return 0;
}
