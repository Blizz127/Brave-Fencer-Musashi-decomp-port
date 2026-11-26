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

BfmInnProfile *bfm_inn_profile_create(const char *seed, const char *directory, const char **error)
{
    BfmSaveMenu menu;
    *error = "Specify a readable source card with --card and a new directory with --save-copy.";
    if (!seed || !directory || !*directory) return NULL;
    FILE *source = fopen(seed, "rb");
    if (!source) return NULL;
    BfmInnProfile *profile = calloc(1, sizeof *profile);
    int valid_size = profile && fread(profile->image, 1, sizeof profile->image, source) == sizeof profile->image &&
                     fgetc(source) == EOF && !ferror(source);
    fclose(source);
    *error = "Could not read a complete 128 KiB source memory card.";
    if (!valid_size) goto failed;
    bfm_save_menu_read_image(&menu, profile->image, sizeof profile->image);
    *error = "The source must have a readable Musashi file with an occupied slot.";
    unsigned valid = 0;
    for (unsigned i = 0; i < 4; ++i) valid += menu.rows[i].state == BFM_SAVE_VALID;
    if (menu.issue || !valid) goto failed;
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

const char *bfm_inn_profile_save(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record)
{
    if (!profile) return "This card is read-only.";
    return bfm_inn_card_save(profile->image, file, record, checked_commit, profile);
}
