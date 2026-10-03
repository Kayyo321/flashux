#include "device_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
static flashux_error_t read_dev(const char *path, unsigned *major_out, unsigned *minor_out)
{
    char filename[4096];
    char text[128];
    char extra;
    int count = snprintf(filename, sizeof(filename), "%s/dev", path);
    if (count < 0 || (size_t)count >= sizeof(filename)) { return FLASHUX_ERR_IO; }
    FILE *file = fopen(filename, "r");
    if (file == NULL) { return errno == ENOENT ? FLASHUX_ERR_NOT_FOUND : FLASHUX_ERR_IO; }
    bool ok = fgets(text, sizeof(text), file) != NULL && !ferror(file);
    (void)fclose(file);
    if (!ok || text[0] < '0' || text[0] > '9' || strchr(text, '-') != NULL ||
        strchr(text, '+') != NULL || sscanf(text, "%u:%u %c", major_out, minor_out, &extra) != 2) {
        return FLASHUX_ERR_IO;
    }
    return FLASHUX_SUCCESS;
}
static flashux_error_t walk_topology(const char *link, unsigned target_major,
                                     unsigned target_minor, unsigned depth, bool *found)
{
    char path[4096];
    char parent[4096];
    char slaves[4096];
    unsigned node_major;
    unsigned node_minor;
    if (depth >= 16 || realpath(link, path) == NULL ||
        read_dev(path, &node_major, &node_minor) != FLASHUX_SUCCESS) { return FLASHUX_ERR_IO; }
    if (node_major == target_major && node_minor == target_minor) { *found = true; return FLASHUX_SUCCESS; }
    memcpy(parent, path, strlen(path) + 1);
    char *slash = strrchr(parent, '/');
    if (slash == NULL) { return FLASHUX_ERR_IO; }
    *slash = '\0';
    flashux_error_t result = read_dev(parent, &node_major, &node_minor);
    if (result == FLASHUX_SUCCESS && node_major == target_major && node_minor == target_minor) {
        *found = true;
        return FLASHUX_SUCCESS;
    }
    if (result != FLASHUX_SUCCESS && result != FLASHUX_ERR_NOT_FOUND) { return result; }
    int length = snprintf(slaves, sizeof(slaves), "%s/slaves", path);
    if (length < 0 || (size_t)length >= sizeof(slaves)) { return FLASHUX_ERR_IO; }
    DIR *directory = opendir(slaves);
    if (directory == NULL) { return errno == ENOENT ? FLASHUX_SUCCESS : FLASHUX_ERR_IO; }
    struct dirent *entry;
    result = FLASHUX_SUCCESS;
    for (;;) {
        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) { result = FLASHUX_ERR_IO; }
            break;
        }
        if (entry->d_name[0] == '.') { continue; }
        char child[4096];
        length = snprintf(child, sizeof(child), "%s/%s", slaves, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(child)) { result = FLASHUX_ERR_IO; break; }
        result = walk_topology(child, target_major, target_minor, depth + 1, found);
        if (result != FLASHUX_SUCCESS || *found) { break; }
    }
    (void)closedir(directory);
    return result;
}
flashux_error_t flashux_device_topology_contains(const char *sysroot, unsigned mounted_major,
                                                unsigned mounted_minor, unsigned target_major,
                                                unsigned target_minor, bool *protected_out)
{
    char path[4096];
    bool found = false;
    if (sysroot == NULL || protected_out == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *protected_out = true; /* Failure always retains protection. */
    int length = snprintf(path, sizeof(path), "%s/dev/block/%u:%u", sysroot,
                           mounted_major, mounted_minor);
    if (length < 0 || (size_t)length >= sizeof(path)) { return FLASHUX_ERR_IO; }
    flashux_error_t result = walk_topology(path, target_major, target_minor, 0, &found);
    if (result == FLASHUX_SUCCESS) { *protected_out = found; }
    return result;
}
