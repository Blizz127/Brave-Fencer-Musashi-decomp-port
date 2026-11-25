#ifndef BFM_ROOM_DESCRIPTOR_H
#define BFM_ROOM_DESCRIPTOR_H

#include "psx_types.h"

/* The room entry copied by MAIN and selected by SC01 is nine PS1 words.
 * Address fields hold guest addresses, including the two scene callbacks. */
typedef struct {
    u16 room;
    u8 parameter;
    u8 world;
    u32 resource;
    u32 room_value;
    u32 scene_data;
    u16 x, y, z, facing;
    u32 activate;
    u32 reserved1c;
    u32 finish;
} RoomTransferDescriptor;

extern RoomTransferDescriptor D_80078E50;
/* The original compiler addresses the room ID as its own halfword symbol in
 * the two classifiers. It is the first field of the same backing object. */
#if defined(__mips__) && !defined(__LP64__)
extern u16 room_descriptor_id __asm__("D_80078E50");
#define ROOM_DESCRIPTOR_ID room_descriptor_id
#else
#define ROOM_DESCRIPTOR_ID (D_80078E50.room)
#endif

extern RoomTransferDescriptor D_80079204;
void func_800296F8(RoomTransferDescriptor *source);
void func_800298BC(RoomTransferDescriptor *destination);

#endif
