/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_PROFILE_H
#define BFM_INN_PROFILE_H
#include "save_menu.h"

typedef struct BfmInnProfile BfmInnProfile;
BfmInnProfile *bfm_inn_profile_create(const char *seed, const char *directory, const char **error);
void bfm_inn_profile_close(BfmInnProfile *profile);
/* Reopening the picker can verify a previously unconfirmed write. Only then
 * return its record in menu, so the active playthrough can use that checkpoint. */
const BfmSaveRecord *bfm_inn_profile_menu(BfmInnProfile *profile, BfmSaveMenu *menu);
const char *bfm_inn_profile_save(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record,
                                 const MusashiDiscMedia *disc);

const char *bfm_chapter_profile_save(BfmInnProfile *profile, unsigned file,
                                     const BfmSaveRecord *record, const MusashiDiscMedia *disc);

#endif
