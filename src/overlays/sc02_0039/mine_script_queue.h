#ifndef MINE_SCRIPT_QUEUE_H
#define MINE_SCRIPT_QUEUE_H

#include "psx_types.h"

extern u16 D_801A07D8[512];
extern s16 D_801A0BDC;
extern const void *D_801A0BFC;

/* Command supplies its complete sequence of one through five words. The
 * authentic write index is initialized to zero and remains in [0, 511].
 * On a host the live payload reference may point directly to native storage. */
void func_80179B74(const u16 *command);
void func_8017A094(const void *payload);

#endif
