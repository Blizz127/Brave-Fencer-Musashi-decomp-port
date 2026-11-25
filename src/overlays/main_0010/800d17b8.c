/* Owner reconstruction of the tagged room descriptor selection. */
#include "room_transfer.h"

#if !defined(__mips__) || !defined(__GNUC__) || __GNUC__ > 2
#include <stdatomic.h>
#endif

extern u32 D_800AE6B4;
extern u8 D_800AE6BC;
extern s32 D_80114EA8;
extern u8 D_80159698[];
extern void func_80011A3C(void);

s32 func_800D17B8(u32 request)
{
    RoomTransferDescriptor *descriptor;

    descriptor = (RoomTransferDescriptor *)(SceneAddressInteger)(request & 0x80FFFFFFu);
    if (descriptor != 0) {
        func_800D185C((u8 *)descriptor);
        if (request & 0x01000000u) {
            D_800AE6B4 = (u32)(SceneAddressInteger)D_80159698;
            D_800AE6BC = 0;
        } else {
            u32 resource = descriptor->resource;
#if !defined(__mips__) || !defined(__GNUC__) || __GNUC__ > 2
            /* The host linker can bind the reset word inside the source descriptor. */
            atomic_signal_fence(memory_order_seq_cst);
#endif

            D_800AE6BC = 0;
            D_80114EA8 = 0;
            D_800AE6B4 = resource;
        }
        func_80011A3C();
        return 1;
    }
    return 0;
}
