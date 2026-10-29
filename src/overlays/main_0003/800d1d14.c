/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"

typedef struct { s16 value; s16 padding; } Entry;
extern Entry D_800D3630[];
extern u8 D_800D3618[], D_800D3620[];
extern u32 D_800D3624[]; /* guest addresses as words (native-lane layout rule); cast at the use */
extern void func_8001534C(s16, void *, int, s16, int, int);
void func_800D1D14(s32 index, s32 mode) {
 if (mode == 0) func_8001534C(*(s16 *)(index * 4 + (u32)D_800D3630), D_800D3618, 0, 0, 0, 0);
 else func_8001534C(*(s16 *)(index * 4 + (u32)D_800D3630), (void *)D_800D3624[index], 0x50, (s16)D_800D3620[index], 0, 0);
}
