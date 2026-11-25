#include <assert.h>
#include <stdint.h>
#include "room_transfer.h"

RoomTransferDescriptor test_source;
RoomTransferDescriptor D_80078E50;
RoomTransferQueue D_800AE6A8;
u8 D_80159698[1];

/* The test linker binds the reset word to the descriptor resource field. */
extern s32 D_80114EA8;
extern u32 D_800AE6B4;
extern u8 D_800AE6BC;
static unsigned int notifications;

void func_80011A3C(void)
{
    ++notifications;
    assert(D_80114EA8 == 0);
    assert(D_800AE6BC == 0);
    assert(D_800AE6B4 == 29);
    assert(D_80078E50.resource == 29);
}

int main(void)
{
    uintptr_t address = (uintptr_t)&test_source;
    assert((address & 0x80FFFFFFu) == address);
    test_source.resource = 29;
    D_800AE6BC = 17;
    assert(func_800D17B8((u32)address) == 1);
    assert(notifications == 1);
    return 0;
}
