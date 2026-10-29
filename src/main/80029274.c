/* Owner re-derivation: the two retail block-copy loops copy 0x98 bytes. */
#include "psx_types.h"
typedef struct { s32 words[38]; } Block;
extern Block D_80072C84, D_80078E78;
void func_80029274(void) {
 Block temporary = D_80072C84;
 D_80078E78 = temporary;
}
