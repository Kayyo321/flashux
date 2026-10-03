#ifndef FLASHUX_CHECKSUM_H
#define FLASHUX_CHECKSUM_H

#include "flashux/core.h"

/**
 * @brief Bitflags for requested hash algorithms.
 */
typedef enum {
    FLASHUX_CHECKSUM_MD5 = (1 << 0),
    FLASHUX_CHECKSUM_SHA1 = (1 << 1),
    FLASHUX_CHECKSUM_SHA256 = (1 << 2),
    FLASHUX_CHECKSUM_SHA512 = (1 << 3)
} flashux_checksum_type_t;

/**
 * @brief Checksum calculation results in hex string representations.
 */
typedef struct {
    char md5[33];
    char sha1[41];
    char sha256[65];
    char sha512[129];
} flashux_checksum_result_t;

/**
 * @brief Calculate cryptographic hashes for a disk image file.
 *
 * @param[in] ctx Flashux context.
 * @param[in] file_path Absolute path to image file.
 * @param[in] checksum_flags Bitwise combination of flashux_checksum_type_t.
 * @param[out] result_out Struct receiving hex string hashes.
 * @return FLASHUX_SUCCESS on success, negative error code on failure.
 */
flashux_error_t flashux_checksum_file(flashux_context_t *ctx, const char *file_path,
                                      uint32_t checksum_flags,
                                      flashux_checksum_result_t *result_out);

#endif /* FLASHUX_CHECKSUM_H */
