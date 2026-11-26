/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_RISE_H
#define BFM_TOWER_RISE_H

/* Room-controller updates since the central figure first saw four credits. */
enum {
    BFM_TOWER_CAMERA_CUT = 33,
    BFM_TOWER_POOL_DRAINS = BFM_TOWER_CAMERA_CUT + 48,
    BFM_TOWER_RISE_START = BFM_TOWER_POOL_DRAINS + 16,
    BFM_TOWER_RISE_END = BFM_TOWER_RISE_START + 276,
    BFM_TOWER_RELEASE = BFM_TOWER_RISE_END + 54
};

static inline int bfm_tower_body_height(unsigned age)
{
    if (age <= BFM_TOWER_RISE_START) return 2207;
    if (age >= BFM_TOWER_RISE_END) return 0;
    return 2207 - 8*(int)(age - BFM_TOWER_RISE_START);
}

#endif
