#include "flashux/checksum.h"
#include "common/file_identity.h"
#include <openssl/evp.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

struct hash_job {
    int fd;
    off_t size;
    const EVP_MD *algorithm;
    char hex[129];
    flashux_error_t result;
};

static void *hash_worker(void *data)
{
    struct hash_job *job = data;
    EVP_MD_CTX *digest = EVP_MD_CTX_new();
    unsigned char buffer[65536];
    unsigned char output[EVP_MAX_MD_SIZE];
    unsigned length = 0;
    off_t offset = 0;
    job->result = FLASHUX_ERR_IO;
    if (digest == NULL) {
        job->result = FLASHUX_ERR_NO_MEMORY;
        return NULL;
    }
    if (EVP_DigestInit_ex(digest, job->algorithm, NULL) != 1) {
        goto cleanup;
    }
    while (offset < job->size) {
        off_t left = job->size - offset;
        size_t count = left < (off_t)sizeof(buffer) ? (size_t)left : sizeof(buffer);
        ssize_t got = pread(job->fd, buffer, count, offset);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got <= 0 || EVP_DigestUpdate(digest, buffer, (size_t)got) != 1) {
            goto cleanup;
        }
        offset += got;
    }
    if (EVP_DigestFinal_ex(digest, output, &length) != 1 || length > 64) {
        goto cleanup;
    }
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < length; ++i) {
        job->hex[2 * i] = digits[output[i] >> 4];
        job->hex[2 * i + 1] = digits[output[i] & 15];
    }
    job->hex[2 * length] = '\0';
    job->result = FLASHUX_SUCCESS;
cleanup:
    EVP_MD_CTX_free(digest);
    digest = NULL;
    return NULL;
}

flashux_error_t flashux_checksum_file(flashux_context_t *ctx, const char *file_path,
                                     uint32_t checksum_flags,
                                     flashux_checksum_result_t *result_out)
{
    struct hash_job jobs[4] = {0};
    pthread_t workers[4];
    bool started[4] = {false};
    struct stat before;
    struct stat after;
    flashux_error_t result = FLASHUX_SUCCESS;
    if (result_out != NULL) {
        memset(result_out, 0, sizeof(*result_out));
    }
    if (ctx == NULL || file_path == NULL || result_out == NULL || checksum_flags == 0 ||
        (checksum_flags & ~UINT32_C(15)) != 0) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    int fd = open(file_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return FLASHUX_ERR_NOT_FOUND;
    }
    if (fstat(fd, &before) != 0 || !S_ISREG(before.st_mode) || before.st_size < 0) {
        (void)close(fd);
        return FLASHUX_ERR_INVALID_ARG;
    }
    const EVP_MD *algorithms[] = {EVP_md5(), EVP_sha1(), EVP_sha256(), EVP_sha512()};
    for (unsigned i = 0; i < 4; ++i) {
        if ((checksum_flags & (UINT32_C(1) << i)) != 0) {
            jobs[i].fd = fd;
            jobs[i].size = before.st_size;
            jobs[i].algorithm = algorithms[i];
            if (pthread_create(&workers[i], NULL, hash_worker, &jobs[i]) != 0) {
                result = FLASHUX_ERR_UNKNOWN;
                break;
            }
            started[i] = true;
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (started[i]) {
            (void)pthread_join(workers[i], NULL);
            if (jobs[i].result != FLASHUX_SUCCESS) {
                result = jobs[i].result;
            }
        }
    }
    if (fstat(fd, &after) != 0 || !flashux_file_unchanged(&before, &after)) {
        result = FLASHUX_ERR_IO;
    }
    (void)close(fd);
    if (result == FLASHUX_SUCCESS) {
        memcpy(result_out->md5, jobs[0].hex, sizeof(result_out->md5));
        memcpy(result_out->sha1, jobs[1].hex, sizeof(result_out->sha1));
        memcpy(result_out->sha256, jobs[2].hex, sizeof(result_out->sha256));
        memcpy(result_out->sha512, jobs[3].hex, sizeof(result_out->sha512));
    } else {
        flashux_log_error(ctx, "Checksum failed: %s", flashux_strerror(result));
    }
    return result;
}
