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
};

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
    if (!cs_storage_commit(profile->storage, slot, expected, replacement) ||
        cs_storage_snapshot(profile->storage, slot, profile->readback) != CS_MEDIA_OK ||
        memcmp(profile->readback, replacement, sizeof profile->image) != 0)
        return 0;
    memcpy(profile->image, profile->readback, sizeof profile->image);
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

void bfm_inn_profile_menu(BfmInnProfile *profile, BfmSaveMenu *menu)
{
    bfm_save_menu_read_image(menu, profile->image, sizeof profile->image);
}

const char *bfm_inn_profile_save(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record,
                                 const MusashiDiscMedia *disc)
{
    if (!profile) return "This card is read-only.";
    return bfm_inn_card_save(profile->image, file, record, checked_commit, profile, disc);
}

const char *bfm_chapter_profile_save(BfmInnProfile *profile, unsigned file,
                                     const BfmSaveRecord *record, const MusashiDiscMedia *disc)
{
    if (!profile) return "This card is read-only.";
    return bfm_chapter_card_save(profile->image, file, record, checked_commit, profile, disc);
}
