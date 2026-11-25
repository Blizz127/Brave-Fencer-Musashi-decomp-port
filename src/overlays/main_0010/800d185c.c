/* Owner reconstruction of the 36-byte room descriptor copy. */
#include "room_transfer.h"

void func_800D185C(u8 *source)
{
    RoomTransferQueue *queue;
    RoomTransferDescriptor *descriptor = (RoomTransferDescriptor *)source;

    queue = &D_800AE6A8;
    D_80078E50 = *descriptor;
    queue->parameter = descriptor->parameter;
    queue->world = descriptor->world;
    queue->pending = (u32)(SceneAddressInteger)descriptor | (queue->pending & 0x01000000u);
}
