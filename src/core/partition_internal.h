#ifndef FLASHUX_PARTITION_INTERNAL_H
#define FLASHUX_PARTITION_INTERNAL_H
#include "flashux/partition.h"
flashux_error_t flashux_partition_write_fd(int fd, const flashux_partition_table_t *table);
uint32_t flashux_crc32(const unsigned char *data, size_t size);
#endif
