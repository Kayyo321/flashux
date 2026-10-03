#ifndef FLASHUX_DEVICE_INTERNAL_H
#define FLASHUX_DEVICE_INTERNAL_H
#include "flashux/device.h"
flashux_error_t flashux_device_geometry_from_sysfs(const char *path, flashux_device_t *device);
flashux_error_t flashux_device_capture_sequence(const char *path, uint64_t *sequence);
flashux_error_t flashux_device_topology_contains(const char *sysroot, unsigned mounted_major,
                                                unsigned mounted_minor, unsigned target_major,
                                                unsigned target_minor, bool *protected_out);
#endif
