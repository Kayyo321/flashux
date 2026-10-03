#include "test.h"
#include "flashux/checksum.h"
#include <string.h>
#include <unistd.h>
int main(void)
{
    char path[] = "/tmp/flashux-checksum-XXXXXX";
    int fd = mkstemp(path);
    flashux_context_t *ctx = NULL;
    flashux_checksum_result_t hashes;
    CHECK(fd >= 0);
    CHECK(write(fd, "abc", 3) == 3);
    CHECK(close(fd) == 0);
    CHECK(flashux_context_new(&ctx) == FLASHUX_SUCCESS);
    CHECK(flashux_checksum_file(ctx, path, 15, &hashes) == FLASHUX_SUCCESS);
    CHECK(strcmp(hashes.md5, "900150983cd24fb0d6963f7d28e17f72") == 0);
    CHECK(strcmp(hashes.sha1, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0);
    CHECK(strcmp(hashes.sha256,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);
    CHECK(strcmp(hashes.sha512,
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2"
        "192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f") == 0);
    CHECK(flashux_checksum_file(ctx, path, 16, &hashes) == FLASHUX_ERR_INVALID_ARG);
    CHECK(hashes.md5[0] == '\0');
    CHECK(unlink(path) == 0);
    CHECK(flashux_checksum_file(ctx, path, 1, &hashes) == FLASHUX_ERR_NOT_FOUND);
    flashux_context_free(&ctx);
    return 0;
}
