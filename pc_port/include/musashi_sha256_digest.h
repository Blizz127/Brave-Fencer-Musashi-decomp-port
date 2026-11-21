#ifndef MUSASHI_SHA256_DIGEST_H
#define MUSASHI_SHA256_DIGEST_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include "../platform/bfm_plat_sha256.h"

static inline int musashi_sha256_digest(const void *data, size_t size, uint8_t digest[32]) {
    BfmPlatSha256 sha;
    bfm_plat_sha256_init(&sha);
    bfm_plat_sha256_update(&sha, data, size);
    bfm_plat_sha256_final(&sha, digest);
    return 1;
}
#else
#include <openssl/evp.h>

static inline int musashi_sha256_digest(const void *data, size_t size, uint8_t digest[32]) {
    unsigned int length = 0;
    return EVP_Digest(data, size, digest, &length, EVP_sha256(), NULL) == 1 && length == 32;
}
#endif

#endif
