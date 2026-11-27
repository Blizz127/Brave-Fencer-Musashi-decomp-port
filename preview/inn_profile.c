/* SPDX-License-Identifier: MIT */
#include "inn_profile.h"
#include "inn_card.h"
#include "card_host_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

struct BfmInnProfile {
    CsStorage *storage;
    uint8_t image[CS_IMAGE_BYTES];
    uint8_t readback[CS_IMAGE_BYTES];
    uint8_t attempted[CS_IMAGE_BYTES];
    int verification_pending;
};

typedef const char *(*SaveFile)(const uint8_t *, unsigned, const BfmSaveRecord *,
                                BfmInnCardCommit, void *, const MusashiDiscMedia *);

enum CardState { CARD_PRIOR_IMAGE, CARD_ATTEMPTED_IMAGE, CARD_UNCONFIRMED };

/* Only our two known images can resolve an interrupted verification. Leave a
 * third image untouched, even if it contains a perfectly readable save. */
static enum CardState pending_state(BfmInnProfile *profile)
{
    if (!profile->verification_pending) return CARD_PRIOR_IMAGE;
    if (cs_storage_snapshot(profile->storage, 0, profile->readback) != CS_MEDIA_OK)
        return CARD_UNCONFIRMED;
    if (memcmp(profile->readback, profile->attempted, sizeof profile->image) == 0)
        return CARD_ATTEMPTED_IMAGE;
    if (memcmp(profile->readback, profile->image, sizeof profile->image) != 0)
        return CARD_UNCONFIRMED;
    profile->verification_pending = 0;
    return CARD_PRIOR_IMAGE;
}

static void accept_readback(BfmInnProfile *profile)
{
    memcpy(profile->image, profile->readback, sizeof profile->image);
    profile->verification_pending = 0;
}

static int new_directory(const char *path)
{
#ifdef _WIN32
    return _mkdir(path) == 0;
#else
    return mkdir(path, 0700) == 0;
#endif
}

void bfm_inn_profile_close(BfmInnProfile *profile)
{
    if (profile) {
        if (profile->storage) cs_storage_destroy(profile->storage);
        free(profile);
    }
}

static int checked_commit(void *context, unsigned slot, const uint8_t *expected,
                          const uint8_t *replacement)
{
    BfmInnProfile *profile = context;
    if (!cs_storage_commit(profile->storage, slot, expected, replacement))
        return 0;
    if (cs_storage_snapshot(profile->storage, slot, profile->readback) != CS_MEDIA_OK ||
        memcmp(profile->readback, replacement, sizeof profile->image) != 0) {
        memcpy(profile->attempted, replacement, sizeof profile->attempted);
        profile->verification_pending = 1;
        return 0;
    }
    accept_readback(profile);
    return 1;
}

static const char *initial_image(CardImage image, const char *seed)
{
    if (!seed) {
        if (card_format(image) != CARD_OK) return "Could not format the new memory card.";
        return NULL;
    }

    FILE *source = fopen(seed, "rb");
    if (!source) return "Could not open the source memory card.";
    int complete = fread(image.bytes, 1, image.length, source) == image.length &&
                   fgetc(source) == EOF && !ferror(source);
    fclose(source);
    if (!complete) return "Could not read a complete 128 KiB source memory card.";

    BfmSaveMenu menu;
    bfm_save_menu_read_image(&menu, image.bytes, image.length);
    if (!menu.issue) {
        if (card_is_unallocated(image)) return NULL;
        for (unsigned i = 0; i < 4; ++i)
            if (menu.rows[i].state == BFM_SAVE_VALID) return NULL;
    }
    return "Use an empty formatted card or a readable Musashi file with an occupied slot.";
}

BfmInnProfile *bfm_inn_profile_create(const char *seed, const char *directory, const char **error)
{
    *error = "--save-copy requires a new directory under an existing parent.";
    if (!directory || !*directory) return NULL;
    *error = "Not enough memory to prepare the save card.";
    BfmInnProfile *profile = calloc(1, sizeof *profile);
    if (!profile) return NULL;
    *error = initial_image((CardImage){profile->image, sizeof profile->image}, seed);
    if (*error) goto failed;
    *error = "Could not select the save-copy directory.";
    profile->storage = cs_storage_create();
    if (!profile->storage || bfm_plat_memcard_set_dir(directory) != BFM_PLAT_OK) goto failed;
    *error = "--save-copy requires a new directory under an existing parent.";
    if (!new_directory(directory)) goto failed;
    *error = "Could not create and verify a new memory card in the save-copy directory.";
    if (cs_storage_attach(profile->storage, 0) != BFM_PLAT_OK ||
        !checked_commit(profile, 0, NULL, profile->image)) goto failed;
    *error = NULL;
    return profile;
failed:
    bfm_inn_profile_close(profile);
    return NULL;
}

const BfmSaveRecord *bfm_inn_profile_menu(BfmInnProfile *profile, BfmSaveMenu *menu)
{
    enum CardState state = pending_state(profile);
    if (state == CARD_UNCONFIRMED) {
        *menu = (BfmSaveMenu){ .issue = "Card unverified. Reopen the menu." };
        return NULL;
    }
    if (state == CARD_ATTEMPTED_IMAGE) accept_readback(profile);
    bfm_save_menu_read_image(menu, profile->image, sizeof profile->image);
    if (state == CARD_ATTEMPTED_IMAGE && !menu->issue && menu->page == BFM_SAVE_PICK &&
        menu->selected < 4 && menu->rows[menu->selected].state == BFM_SAVE_VALID)
        return &menu->records[menu->selected];
    return NULL;
}

static int same_save(void *context, unsigned slot, const uint8_t *expected,
                     const uint8_t *replacement)
{
    (void)slot;
    (void)expected;
    return memcmp(context, replacement, CS_IMAGE_BYTES) == 0;
}

static const char *save_record(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record,
                               const MusashiDiscMedia *disc, SaveFile write_file)
{
    if (!profile) return "This card is read-only.";
    enum CardState state = pending_state(profile);
    if (state == CARD_UNCONFIRMED) return "Card unverified. Retry Save.";
    if (state == CARD_ATTEMPTED_IMAGE) {
        /* Re-encode against the old card to recognize an exact retry, including
         * the selected file and record. This callback only compares bytes. */
        const char *different = write_file(profile->image, file, record, same_save,
                                           profile->readback, disc);
        accept_readback(profile);
        if (!different) return NULL;
    }
    return write_file(profile->image, file, record, checked_commit, profile, disc);
}

const char *bfm_inn_profile_save(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record,
                                 const MusashiDiscMedia *disc)
{
    return save_record(profile, file, record, disc, bfm_inn_card_save);
}

const char *bfm_chapter_profile_save(BfmInnProfile *profile, unsigned file,
                                     const BfmSaveRecord *record, const MusashiDiscMedia *disc)
{
    return save_record(profile, file, record, disc, bfm_chapter_card_save);
}
