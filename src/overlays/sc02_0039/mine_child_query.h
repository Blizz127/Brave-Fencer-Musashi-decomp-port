#ifndef BFM_MINE_CHILD_QUERY_H
#define BFM_MINE_CHILD_QUERY_H

#include "mine_child_model.h"
#include "../main_0012/scene_object_probes.h"

/* The record is followed by the signed offsets used on both axes. Its
   storage must cover any count supplied by a segment callback. */
typedef struct {
    MineChildModelRecord record;
#if defined(__mips__) && !defined(__LP64__)
    /* The original target compiler uses this spelling for a trailing array. */
    s16 samples[0];
#else
    s16 samples[];
#endif
} MineChildQueryShape;

#if !defined(__mips__) || defined(__LP64__)
s32 bfm_mine_child_query_segment(MineChildModelActor *actor, SceneProbeVector *start,
                                SceneProbeVector *end);
#endif

#endif
