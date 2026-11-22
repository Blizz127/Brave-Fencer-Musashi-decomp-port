/* Set the actor's template address, health, flags and optional model. */
#include "steam_knight_resource.h"

extern u8 D_80078EAE;
extern void func_8012CAE4(u8 *actor);
extern void func_8001C214(void *model, s32 resource);

#if defined(__mips__) && !defined(__LP64__)
#define READ_HALF(ptr) (*(u16 *)(ptr))
#define READ_SIGNED_HALF(ptr) (*(s16 *)(ptr))
#define READ_WORD(ptr) (*(u32 *)(ptr))
#define READ_SIGNED_WORD(ptr) (*(s32 *)(ptr))
#define WRITE_HALF(ptr, value) (*(u16 *)(ptr) = (value))
#define WRITE_WORD(ptr, value) (*(u32 *)(ptr) = (value))
#define WRITE_ADDRESS(ptr, value) (*(void **)(ptr) = (value))
#else
/* Actor records store 32-bit addresses even when host pointers are wider. */
extern u32 bfm_actor_template_address(const void *pointer);

static u16 read_half(const u8 *p)
{
    return p[0] | (u16)(p[1] << 8);
}

static u32 read_word(const u8 *p)
{
    return read_half(p) | ((u32)read_half(p + 2) << 16);
}

static void write_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void write_word(u8 *p, u32 value)
{
    write_half(p, (u16)value);
    write_half(p + 2, (u16)(value >> 16));
}

static void write_address(u8 *p, const void *value)
{
    write_word(p, value ? bfm_actor_template_address(value) : 0);
}

#define READ_HALF(ptr) read_half(ptr)
#define READ_SIGNED_HALF(ptr) ((s16)read_half(ptr))
#define READ_WORD(ptr) read_word(ptr)
#define READ_SIGNED_WORD(ptr) ((s32)read_word(ptr))
#define WRITE_HALF(ptr, value) write_half((ptr), (value))
#define WRITE_WORD(ptr, value) write_word((ptr), (value))
#define WRITE_ADDRESS(ptr, value) write_address((ptr), (value))
#endif

s32 func_8012C354(u8 *actor, u8 *record)
{
    void *model;
    s32 health;
    u32 flags;

    WRITE_ADDRESS(actor + 0x78, record);
    if (D_80078EAE == 0) {
        health = (READ_SIGNED_HALF(record) * 24) / 16;
    } else {
        health = READ_HALF(record);
    }
    WRITE_HALF(actor + 0x76, health);
    WRITE_HALF(actor + 0x5C, READ_HALF(record + 2));
    if (READ_WORD(record + 4) != 0) {
        model = func_8001CFDC(0x26, 0x85);
        WRITE_ADDRESS(actor + 0x20, model);
        if (model == 0) {
            func_8012CAE4(actor);
            return 0;
        }
        func_8001C214(model, READ_SIGNED_WORD(record + 4));
    }
    flags = READ_WORD(record + 8);
    if (flags != 0) {
        WRITE_WORD(actor + 0x58, flags | 0x60000000);
    }
    return 1;
}
