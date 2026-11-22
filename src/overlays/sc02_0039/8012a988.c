#include "mine_animation_frames.h"

extern void func_8012AAAC(Obj8014A2E4 *actor);

enum {
    tick_flags = 0x72,
    frame_list = 0x90,
    frame_index = 0x94,
    frame_delay = 0x98,
    tick_state = 0xc1
};

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define signed_delay(value) ((s16)(value))
#define read_half(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define read_word(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define write_half(p, off, value) (read_half(p, off) = (value))
#define write_word(p, off, value) (read_word(p, off) = (value))
#else
static s16 signed_delay(u16 value)
{
    return (s16)((s32)(value ^ 0x8000u) - 0x8000);
}

static u16 read_half(const Obj8014A2E4 *actor, unsigned offset)
{
    const u8 *p = (const u8 *)actor + offset;
    return (u16)(p[0] | (u16)p[1] << 8);
}

static u32 read_word(const Obj8014A2E4 *actor, unsigned offset)
{
    return (u32)read_half(actor, offset) | (u32)read_half(actor, offset + 2) << 16;
}

static void write_half(Obj8014A2E4 *actor, unsigned offset, u16 value)
{
    u8 *p = (u8 *)actor + offset;
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void write_word(Obj8014A2E4 *actor, unsigned offset, u32 value)
{
    write_half(actor, offset, (u16)value);
    write_half(actor, offset + 2, (u16)(value >> 16));
}
#endif

void func_8012A988(Obj8014A2E4 *actor)
{
    s16 delay;
    u16 flags;
    u32 index;
    s32 last;
    s16 operation;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    u32 frames;
    if (!actor)
        return;
#endif

    write_half(actor, tick_flags, read_half(actor, tick_flags) & 0x9fff);
    if (!read_half(actor, 0) || ((u8 *)actor)[tick_state] == 0x11)
        return;
    delay = signed_delay(read_half(actor, frame_delay));
    if (!delay)
        return;
    delay = signed_delay((u16)(delay - 1));
    write_half(actor, frame_delay, (u16)delay);
    if (delay > 0 || !read_word(actor, frame_list))
        return;

    flags = read_half(actor, tick_flags);
    if (flags & 0x400) {
        index = read_word(actor, frame_index) - 1u;
        write_word(actor, frame_index, index);
        if (index & 0x80000000u) {
            write_word(actor, frame_index, 1);
            write_half(actor, tick_flags, read_half(actor, tick_flags) & 0xfbff);
        }
    } else if (flags & 0x200) {
        index = read_word(actor, frame_index) - 1u;
        write_word(actor, frame_index, index);
        if (index & 0x80000000u) {
            last = func_8012ACE0(actor);
#if BFM_MINE_DIRECT_GUEST_ADDRESS
            operation = *(s16 *)(read_word(actor, frame_list) + (u32)last * 8u + 4u);
#else
            frames = read_word(actor, frame_list);
            if (last < 0 || !frames || (frames & 1u) || frames > 0xfffffffau ||
                (u32)last > (0xfffffffau - frames) / 8u ||
                !bfm_animation_frame_operation(frames + (u32)last * 8u + 4u, &operation))
                operation = 0;
#endif
            if (operation != -1) {
                write_word(actor, frame_index, 0);
                write_half(actor, frame_delay, 0);
                return;
            }
            write_word(actor, frame_index, (u32)last - 1u);
        }
    } else {
        write_word(actor, frame_index, read_word(actor, frame_index) + 1u);
    }
    func_8012AAAC(actor);
}
