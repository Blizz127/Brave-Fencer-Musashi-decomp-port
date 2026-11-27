/* SPDX-License-Identifier: MIT */
#ifndef BFM_CHAPTER_RESTART_H
#define BFM_CHAPTER_RESTART_H

#include "bedroom.h"

/* No saved data or Memory Box is available. Build the original restart
 * record in memory, preserving the player's settings and elapsed playtime. */
int bfm_chapter_restart_record(const MusashiDiscMedia *disc, const BfmGameplayRun *run,
                               BfmSaveRecord *out);
/* Mount and render the first field of the castle wake before replacing the
 * live banks. A failure leaves the outgoing run intact. */
BfmBedroom *bfm_chapter_restart_open(const MusashiDiscMedia *disc, BfmGameplayRun *run);

#endif
