/* Owner reconstruction of the pending and queued room request driver. */
#include "room_transfer.h"

extern u32 D_800AE6B0;

void func_800D1754(void)
{
    RoomTransferQueue *queue;
    u32 request;

    queue = &D_800AE6A8;
    if (func_800D17B8(D_800AE6B0) == 0) {
        request = queue->queued;
        if (request != 0) {
            queue->queued = 0;
            D_800AE6B0 = request;
            func_800D17B8(request);
        }
    }
}
