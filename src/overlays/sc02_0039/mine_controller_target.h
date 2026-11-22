#ifndef BFM_MINE_CONTROLLER_TARGET_H
#define BFM_MINE_CONTROLLER_TARGET_H

#include "mine_guest_abi.h"
#include "psx_types.h"

void func_80165E90(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
/* Host observations of the game calls. Each point has four signed halfwords. */
const u8 *bfm_mine_controller_target_view(u32 address, unsigned size);
void bfm_mine_controller_first_points(s16 *from, s16 *to);
void bfm_mine_controller_second_points(s16 *from, s16 *to);
void bfm_mine_controller_transform_points(u8 *controller, s16 *from, s16 *to);
void bfm_mine_controller_first_draw(u8 *controller, s16 *from, s16 *to);

/* Returns zero at the first unmapped target. Earlier service calls persist. */
int bfm_mine_controller_update(void);
#endif

#endif
