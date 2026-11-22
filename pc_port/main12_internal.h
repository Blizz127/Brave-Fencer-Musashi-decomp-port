#ifndef MUSASHI_MAIN12_INTERNAL_H
#define MUSASHI_MAIN12_INTERNAL_H

#include "musashi_main12.h"

/* Allocated only for a successfully verified disc mount, outside guest RAM. */
struct MusashiMain12Owner {
    MusashiGuestIrqImage *image;
    MusashiBootMemory *memory;
    const uint8_t *transfer; /* First 2048-byte payload in the immutable raw disc. */
    int (*matches)(const MusashiMain12Owner *owner, const uint8_t *live,
                   int installed, const uint8_t mailbox[16]);
    const uint8_t *exe_text; /* First sector payload of the mounted executable text. */
    int (*resident_matches)(const MusashiMain12Owner *owner, uint32_t start, uint32_t size);
    uint8_t mailbox[16];
    uint8_t directory_row[40];
    uint8_t directory_valid;
    uint64_t directory_fd_token;
    uint8_t menu_word_changed;
    int installed;
    struct {
        const MusashiCdDevice *files;
        const MusashiBiosEvents *events;
        uint64_t descriptor, allocation[8];
        unsigned phase;
    } metadata_read;
};

#endif
