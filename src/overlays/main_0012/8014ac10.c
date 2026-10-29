/* Owner re-derivation from retail assembly (clean input set). */
/* Clears every live pool entry (D_8011D030, 30 x 0x58) whose id equals `id`, including the 0x38/0x84-byte
 * block its +0x20 pointer refers to. */
#include "psx_types.h"

typedef struct {
    u16 kind;
} Blk8014AC10;

typedef struct {
    u16 id;
    u8 pad02[0x1E];
    u32 blk; /* 0x20: guest address of Blk8014AC10 */
    u8 pad24[0x34];
} Ent8014AC10; /* 0x58 bytes */

extern u8 D_8011D030[];
extern void func_80016714(void *p, s32 n);

void func_8014AC10(u32 id)
{
    Ent8014AC10 *e;

    for (e = ((Ent8014AC10 *)&D_8011D030); e < &((Ent8014AC10 *)&D_8011D030)[30]; e++) {
        if (e->id == id) {
            if (e->blk != 0) {
                func_80016714((void *)e->blk, ((Blk8014AC10 *)e->blk)->kind == 1 ? 0x84 : 0x38);
            }
            func_80016714(e, 0x58);
        }
    }
}
