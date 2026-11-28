#include "psx_types.h"

typedef struct {
    u8 reserved[0x15];
    u8 state;
    u8 reserved16[0x12];
    u32 ticks;
} LakeReturnJob;

typedef int (*LakeReturnStep)(LakeReturnJob *job);

extern LakeReturnStep D_80185954[];

int func_8017E934(LakeReturnJob *job)
{
    return D_80185954[job->state](job);
}
