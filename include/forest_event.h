#ifndef MUSASHI_FOREST_EVENT_H
#define MUSASHI_FOREST_EVENT_H

#include "scene_position_slot.h"

/* Requests store three four-byte game values even on a 64-bit host. */

typedef struct {
    s16 type, zero, amount, phase;
    u32 actor, first, second;
} ForestActionEventRecord;

typedef ScenePositionSlot ForestEventSlot;

ForestEventSlot *func_80146750(ForestActionEventRecord *request);
ForestEventSlot *func_8014680C(ForestActionEventRecord *request);

/* A request may overlap a slot. The next field must see the preceding store. */
#if defined(__mips__) && !defined(__LP64__)
#define BFM_STORE_EVENT_FIELD(destination, record, field) ((destination) = (record)->field)
#else
#include <stddef.h>
#include <string.h>
#define BFM_STORE_EVENT_FIELD(destination, record, field) \
    memmove(&(destination), (const unsigned char *)(record) + \
            offsetof(ForestActionEventRecord, field), sizeof(destination))
#endif

#endif
