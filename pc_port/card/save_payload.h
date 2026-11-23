#ifndef MUSASHI_SAVE_PAYLOAD_H
#define MUSASHI_SAVE_PAYLOAD_H

#include <stdint.h>

/* One 8 KiB game save container holds four menu files after its metadata. */
enum {
    MUSASHI_SAVE_FILES = 4,
    MUSASHI_PAYLOAD_FIRST = 1152,
    MUSASHI_PAYLOAD_BYTES = 768,
    MUSASHI_PAYLOAD_DEST = 0x80075cc0u
};

static inline uint32_t musashi_save_payload_offset(unsigned file)
{
    return file >= 1u && file <= MUSASHI_SAVE_FILES
        ? MUSASHI_PAYLOAD_FIRST + (file - 1u) * MUSASHI_PAYLOAD_BYTES : 0u;
}
#endif
