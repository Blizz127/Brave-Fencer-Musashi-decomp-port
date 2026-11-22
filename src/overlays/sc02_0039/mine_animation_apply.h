#ifndef BFM_MINE_ANIMATION_APPLY_H
#define BFM_MINE_ANIMATION_APPLY_H
#include "mine_animation_frames.h"

void func_8012AAAC(Obj8014A2E4 *actor);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
enum {
    BFM_ANIMATION_BAD_ACTOR = 1,
    BFM_ANIMATION_COMMAND_LIMIT,
    BFM_ANIMATION_BAD_FRAME,
    BFM_ANIMATION_BAD_TARGET
};

/* The host supplies the work limit and maps one aligned game word.
 * Mappings must remain stable throughout the call, including any aliases. */
u32 bfm_animation_command_budget(const Obj8014A2E4 *actor);
u8 *bfm_animation_word_view(u32 address, int writable);
void bfm_animation_command_refused(Obj8014A2E4 *actor, unsigned reason, u32 address);
#endif
#endif
