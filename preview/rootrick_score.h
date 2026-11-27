/* SPDX-License-Identifier: MIT */
#ifndef BFM_ROOTRICK_SCORE_H
#define BFM_ROOTRICK_SCORE_H

#include <string.h>

/* The challenge, race and its two outcomes use the same instrument layout. */
static inline int bfm_rootrick_score_member(const char *archive, unsigned member)
{
    return !strcmp(archive, "SC03.CD") && (member == 50 || (member >= 37 && member <= 39));
}

#endif
