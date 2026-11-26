/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_CARD_H
#define BFM_INN_CARD_H

#include "bedroom_record.h"
#include "card_image.h"

/* Write an empty in-game File 1..4 in an existing US Musashi container.
 * Operates in memory. An error leaves the image unchanged. */
const char *bfm_inn_card_encode(CardImage image, unsigned file, const BfmSaveRecord *record);

/* Called for port 0 while its writable session is exclusively held. Compare
 * the full current image to expected and replace it atomically on equality.
 * Return nonzero only on success. Both images are CARD_IMAGE_SIZE bytes. */
typedef int (*BfmInnCardCommit)(void *context, unsigned port, const uint8_t *expected,
                                const uint8_t *replacement);

/* The expected image was captured when the Save picker was opened. The
 * caller retains it. Return NULL after a successful commit or an error for
 * the player. The mounted US disc supplies the header for a formatted empty
 * card. Read-only sessions do not supply a commit function. */
const char *bfm_inn_card_save(const uint8_t *expected, unsigned file,
                              const BfmSaveRecord *record, BfmInnCardCommit commit,
                              void *context, const MusashiDiscMedia *disc);

/* Chapter completion saves the earned story-two record to an empty file. */
const char *bfm_chapter_card_save(const uint8_t *expected, unsigned file,
                                 const BfmSaveRecord *record, BfmInnCardCommit commit,
                                 void *context, const MusashiDiscMedia *disc);

#endif
