#ifndef MUSASHI_CARD_SAVE_DIRECTORY_H
#define MUSASHI_CARD_SAVE_DIRECTORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { CARD_DIRECTORY_UNAVAILABLE, CARD_DIRECTORY_READY, CARD_DIRECTORY_EXISTING };

/* Resolve the startup directory before attaching a writable slot. EXISTING
 * means a previously observed image must survive until the session acquires
 * its lock. Unavailable leaves the candidate path if it fit. */
int musashi_card_save_directory(char *path, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
