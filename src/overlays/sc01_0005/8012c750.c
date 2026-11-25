/* Owner W13 recovery of the SC01 placement slot search. */
#include "saved_room_actor.h"

/* D_801202A0 names the first of 96 actors following the pool sentinel. */
extern struct FileActor D_80120194[];
extern struct FileActor D_801202A0[];

struct FileActor *func_8012C750(struct FileActorPlacement *record)
{
    struct FileActor *slot;
    struct FileActor *sentinel;
    struct FileActor *cursor = 0;
    struct FileActor *result;

    if (record->flags & 0x0800) {
        sentinel = D_80120194;
        slot = sentinel + 96;
        for (; slot != sentinel; --slot) {
            if (slot->type == 0) {
                goto found;
            }
        }
        slot = 0;
    } else {
        sentinel = D_80120194;
        /* This entry to the forward loop preserves the original compiler layout. */
        slot = 0;
        if (slot == 0) {
            goto scan;
        }
    hit:
        slot = cursor;
        goto found;
    scan:
        slot = sentinel + 97;
        for (cursor = D_801202A0; cursor != slot; ++cursor) {
            if (cursor->type == 0) {
                goto hit;
            }
        }
        slot = 0;
    }
found:
    if (slot == 0) {
        result = 0;
    } else {
        record->flags |= 0x8000;
        result = func_8012C890(record, slot, 0);
    }
    return result;
}
