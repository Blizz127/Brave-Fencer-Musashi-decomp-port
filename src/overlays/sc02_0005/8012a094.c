#include "tower_postfire_camera.h"

#if defined(__mips__) && !defined(__LP64__)
#define COPY_WORD(dst, src) (*(u32 *)(dst) = *(u32 *)(src))
#define COPY_POSITION(dst, src) (*(PostfireCameraRawPosition *)(dst) = *(PostfireCameraRawPosition *)(src))
#define READ_HALF(p) (*(u16 *)(p))
#define WRITE_HALF(p, v) (*(u16 *)(p) = (v))
#else
#include <string.h>
static u16 read_half(const u8 *p)
{
    u16 value;
    memcpy(&value, p, sizeof value);
    return value;
}
static void write_half(u8 *p, u16 value)
{
    memcpy(p, &value, sizeof value);
}
#define COPY_WORD(dst, src) memcpy((dst), (src), sizeof(u32))
#define COPY_POSITION(dst, src) memcpy((dst), (src), sizeof(PostfireCameraRawPosition))
#define READ_HALF(p) read_half(p)
#define WRITE_HALF(p, v) write_half((p), (v))
#endif

s32 func_8012A094(u8 *camera)
{
    u16 x, y, z;
    COPY_WORD(camera + 8, camera + 0xc);
    COPY_WORD(camera + 0x10, camera + 0x14);
    COPY_POSITION(camera + 0x18, camera + 0x20);
    x = READ_HALF(camera + 0x2e);
    y = READ_HALF(camera + 0x30);
    z = READ_HALF(camera + 0x32);
    WRITE_HALF(camera + 0x28, x);
    WRITE_HALF(camera + 0x2a, y);
    WRITE_HALF(camera + 0x2c, z);
    return x;
}
