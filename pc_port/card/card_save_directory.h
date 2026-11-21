#ifndef MUSASHI_CARD_SAVE_DIRECTORY_H
#define MUSASHI_CARD_SAVE_DIRECTORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Select the launcher's save directory and create missing directories.
 * Return 1 when the destination exists as a directory. On failure, path
 * contains the selected destination if it fit, or an empty string. */
int musashi_card_save_directory(char *path, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
