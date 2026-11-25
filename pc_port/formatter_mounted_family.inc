/* SPDX-License-Identifier: MIT */
/* Fields in the actual family owners. Observe their identities and live
 * snapshots around one instruction; neither owner has a generation counter. */
struct bfm_formatter_family_snapshot {
    const OpeningPitchRun *run;
    union { OpeningPitchRun opening; Sc01RoomFrame room; } state;
};

static inline bool bfm_formatter_same_opening(const OpeningPitchRun *a, const OpeningPitchRun *b)
{
    return a->image == b->image && a->clock == b->clock &&
        a->binding.advance == b->binding.advance && a->binding.userdata == b->binding.userdata &&
        a->pc == b->pc && a->npc == b->npc && a->branch == b->branch && a->delay == b->delay &&
        a->hi == b->hi && a->lo == b->lo && !memcmp(a->regs, b->regs, sizeof a->regs) &&
        !memcmp(a->channel_state, b->channel_state, sizeof a->channel_state) &&
        !memcmp(a->mixer_state, b->mixer_state, sizeof a->mixer_state) &&
        !memcmp(a->caller, b->caller, sizeof a->caller) && !memcmp(a->leaf, b->leaf, sizeof a->leaf);
}

static inline bool bfm_formatter_same_room(const Sc01RoomFrame *a, const Sc01RoomFrame *b)
{
    return a->memory == b->memory && a->next == b->next && a->npc == b->npc &&
        a->branch == b->branch && a->sp == b->sp && a->ra == b->ra &&
        a->caller_sp == b->caller_sp && a->callee_return == b->callee_return &&
        a->callee_sp == b->callee_sp && a->return_jump == b->return_jump &&
        a->merge.kind == b->merge.kind && a->merge.reg == b->merge.reg &&
        a->merge.value == b->merge.value && a->merge.prior == b->merge.prior;
}
