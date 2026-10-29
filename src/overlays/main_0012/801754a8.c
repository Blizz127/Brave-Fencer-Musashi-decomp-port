/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame dispatch for the state block D_8011F7A8: byte 9 is set to 1 unless the game is running
 * normally (neither D_800B9A13 nor bit 7 of D_80126B9C set) or the counter func_80029504() lies in
 * [10, 10 + 0x4B0); then the handler D_80181B6C[block.mode] is called on the block. */
#include "psx_types.h"

/* State block: mode byte at 0, flag byte at 9. */
typedef struct {
    u8 mode;
    u8 pad[8];
    u8 flag;
} State;

extern State D_8011F7A8;
extern u8 D_800B9A13;
extern u32 D_80126B9C;
extern u32 D_80181B6C[]; /* guest handler table: u32 elements, cast at use */
extern u32 func_80029504(void);

void func_801754A8(void)
{
    State *s = &D_8011F7A8;
    u32 t = func_80029504();
    u8 flag = 0;

    if ((D_800B9A13 == 0 && (D_80126B9C & 0x80) == 0) || t - 10 >= 0x4B0) {
        flag = 1;
    }
    s->flag = flag;
    ((void (*)(State *))D_80181B6C[s->mode])(s);
}
