#include "psx_types.h"

typedef struct {
    u8 reserved[0x15];
    u8 state;
    u8 reserved16[0x12];
    u32 ticks;
} LakeReturnTimer;

extern void func_8002D4C8(u16 command, u16 argument);
extern void func_8001BFD0(void);
extern void func_800D1E28(void);

int func_8017E970(LakeReturnTimer *timer)
{
    if (--timer->ticks == (u32)-1) {
        func_8002D4C8(28, 0);
        func_8001BFD0();
        func_8002D4C8(29, 0);
        func_800D1E28();
        ++timer->state;
    }

    return 0;
}
