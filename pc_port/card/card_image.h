#ifndef CARD_IMAGE_H
#define CARD_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CARD_IMAGE_SIZE 131072u
#define CARD_BLOCK_SIZE 8192u
#define CARD_MAX_FILES 15u
#define CARD_MAX_NAME 20u

typedef struct {
    uint8_t *bytes;
    size_t length;
} CardImage;

typedef struct {
    char name[CARD_MAX_NAME + 1u];
    uint32_t size;
    uint32_t first_sector;
} CardEntry;

typedef enum {
    CARD_OK,
    CARD_ARGUMENT,
    CARD_INVALID,
    CARD_NOT_FOUND,
    CARD_EXISTS,
    CARD_NO_SPACE,
    CARD_RANGE
} CardResult;

/* Callers serialize operations on an image and supply nonoverlapping buffers.
   File operations validate the current image. Only format initializes it. */
CardResult card_validate(CardImage image);
/* True only for an entire, exactly-sized, all-zero unformatted image. */
int card_is_blank(CardImage image);
/* Inspect only the logical header's two identifier bytes. Check the image
   length and replacement table before resolving sector zero. A completely
   blank image has no identifier. Other header bytes and directory contents
   do not affect this query. On failure leave *recognized unchanged. */
CardResult card_header_signature(CardImage image, int *recognized);
CardResult card_format(CardImage image);
CardResult card_list(CardImage image, CardEntry entries[CARD_MAX_FILES], size_t *count);
CardResult card_open(CardImage image, const char *name, CardEntry *entry);
/* blocks is the fixed allocation size, from 1 through CARD_MAX_FILES. */
CardResult card_create(CardImage image, const char *name, size_t blocks);
/* Reads through EOF succeed and report the number of available bytes. */
CardResult card_read(CardImage image, const char *name, size_t offset,
                     void *data, size_t length, size_t *actual);
/* Writes are exact and atomic w.r.t. validation and capacity errors. */
CardResult card_write(CardImage image, const char *name, size_t offset,
                      const void *data, size_t length);
CardResult card_delete(CardImage image, const char *name);

#ifdef __cplusplus
}
#endif
#endif
