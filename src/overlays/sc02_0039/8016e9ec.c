/* Open a status frame using the category's mutable room text. */
#include "mine_frame_slots.h"

extern u8 *D_80182498[];
extern u8 D_801824B0[];
extern u16 D_801824B4[];
extern s32 func_800D2CA8(s32 value, s32 shift);
extern void func_800D2D10(s32 packed, s32 digits, u8 *output, s32 flags);
extern s16 func_80024054(u8 *text, s16 *glyphs);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define MINE_REG(name) __asm__(name)
#define MINE_KEEP_INDEX(index, text, insertion) \
    __asm__("" : "=r"(index) : "0"(index), "r"(text), "r"(insertion))
#define MINE_TEXT_AT(text, insertion) ((u8 *)((insertion) + (u32)(text)))
#define MINE_STORE16(where, value) (*(s16 *)(where) = (s16)(value))
#else
#define MINE_REG(name)
#define MINE_KEEP_INDEX(index, text, insertion) ((void)0)
#define MINE_TEXT_AT(text, insertion) ((text) + (insertion))
static __inline__ void mine_store16(u8 *where, u16 value)
{
    where[0] = (u8)value;
    where[1] = (u8)(value >> 8);
}
#define MINE_STORE16(where, value) mine_store16((where), (u16)(value))
#endif

void func_8016E9EC(MineFrameSlot *slot)
{
    s32 index;
    s32 insertion;
    register u8 *text MINE_REG("$18");
    register u8 kind MINE_REG("$20");

    index = slot->active - 1;
    insertion = D_801824B0[index];
    text = D_80182498[index];
    MINE_KEEP_INDEX(index, text, insertion);
    kind = (u8)index;

    func_800D2D10(func_800D2CA8(slot->reserved[1], 28), 1,
                  MINE_TEXT_AT(text, insertion), 0);
    func_80024054(text, (s16 *)(slot->reserved + 6));
    slot->reserved[1] = kind;
    MINE_STORE16(slot->reserved + 2, -215);
    MINE_STORE16(slot->reserved + 4, D_801824B4[index]);
    slot->reserved[0] = 0;
    ++slot->handler;
    func_8016EC0C(slot, 128);
}
