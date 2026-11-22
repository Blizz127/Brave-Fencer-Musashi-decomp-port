#include "game_track_rows.h"

s32 func_80030730(void)
{
    TrackDefinition *definition;
    const TrackPair *pairs;
    TrackRow *rows;
    TrackInfo info;
    s32 channel_offset;
    s32 count, i, handle;
#ifdef BFM_NATIVE_TRACK_ROWS
    if (bfm_track_rows_failed())
        return 0;
#endif
    if (!(s16)func_80042374(0))
        return 0;
    D_8006AEF4 &= 0xfc;
    func_80037D74();
#ifdef BFM_NATIVE_TRACK_ROWS
    if (!bfm_track_slot_valid(D_800A4638.slot) ||
        !bfm_track_definition_valid(D_800A4638.track))
        return bfm_reject_track_rows();
#endif
    channel_offset = D_800A4638.slot * (s32)sizeof(TrackSlot);
    D_800760F0 = 0;
    definition = &D_80064D44[D_800A4638.track];
    D_800760F4 = definition->count;
    if (D_800760F4 == 0) {
        ((TrackSlot *)((u8 *)&D_800A4638.slots + channel_offset))->empty = 0;
        D_800A4638.phase = 0;
        return 1;
    }
#ifndef BFM_NATIVE_TRACK_ROWS
    pairs = (const TrackPair *)definition->pair_address;
#endif
    if (D_80065504[D_800A4638.slot] < D_800760F4)
        D_800760F4 = D_80065504[D_800A4638.slot];
#ifdef BFM_NATIVE_TRACK_ROWS
    if (!bfm_track_chunk(definition->pair_address, D_8006A970[D_800A4638.slot],
                         0, D_800760F4, &pairs, &rows))
        return bfm_reject_track_rows();
#else
    rows = (TrackRow *)D_8006A970[D_800A4638.slot];
#endif
    if (D_800760F4 <= 10) {
#ifdef BFM_NATIVE_TRACK_ROWS
        if (!bfm_track_owners_valid(D_800A4638.track, definition->kind, definition->target))
            return bfm_reject_track_rows();
#endif
        if (definition->kind) {
            ((TrackSlot *)((u8 *)&D_800A4638.slots + channel_offset))->reserved_a = definition->value;
            D_800A4638.slots[D_800A4638.slot].other_type = definition->kind;
            D_800A4638.slots[D_800A4638.slot].other_track = definition->target;
            D_800C5328[definition->target][1] = D_800A4638.slot;
        }
        count = D_800760F4;
        D_800760F4 = 0;
        D_800C5328[D_800A4638.track][0] = D_800A4638.slot;
    } else {
        count = 10;
        D_800760F4 -= 10;
    }
    for (i = 0; i < count; i++, pairs++, rows++) {
        if (!func_8003F144(D_800A4638.handle, pairs->first, pairs->second, &info)) {
            handle = func_8003F380(D_800A4638.handle, info.key);
            if (handle >= 0) {
                rows->active = 1;
                rows->scale = info.scale << 8;
                rows->handle = handle;
                rows->first = info.first;
                rows->second = info.second;
            }
        }
    }
    if (!D_800760F4) {
#ifdef BFM_NATIVE_TRACK_ROWS
        if (!bfm_track_slot_valid(D_800A4638.slot))
            return bfm_reject_track_rows();
#endif
        D_800A4638.slots[D_800A4638.slot].empty = 0;
        D_800A4638.phase = 0;
        return 1;
    }
    D_800760F0 += 10;
    D_800A4638.phase++;
    return 0;
}
