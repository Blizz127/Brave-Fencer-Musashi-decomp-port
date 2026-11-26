/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_PROFILE_H
#define BFM_INN_PROFILE_H
#include "save_menu.h"

typedef struct BfmInnProfile BfmInnProfile;
BfmInnProfile *bfm_inn_profile_create(const char *seed, const char *directory, const char **error);
void bfm_inn_profile_close(BfmInnProfile *profile);
void bfm_inn_profile_menu(BfmInnProfile *profile, BfmSaveMenu *menu);
const char *bfm_inn_profile_save(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record,
                                 const MusashiDiscMedia *disc);

#endif
