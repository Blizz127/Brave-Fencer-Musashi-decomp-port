/* Owner re-derivation from retail assembly (clean input set). */
/* Records a hit: entry idx of five 28-byte-stride field views gets a flag (1), the 3 u16
 * components copied from *p, and the raw word value. */
#include "psx_types.h"

typedef struct { u16 v; u8 pad[26]; } U16Row; /* 28 bytes, field at offset 0 */
typedef struct { u32 v; u8 pad[24]; } U32Row; /* 28 bytes, field at offset 0 */

typedef struct { u16 x, y, z; } Vec3s;

extern U32Row D_801BE920[];
extern U16Row D_801BE924[];
extern U16Row D_801BE926[];
extern U16Row D_801BE928[];
extern U32Row D_801BE92C[];

void func_8013B6A0(s32 idx, Vec3s *p, s32 value)
{
    D_801BE920[idx].v = 1;
    D_801BE924[idx].v = p->x;
    D_801BE926[idx].v = p->y;
    D_801BE928[idx].v = p->z;
    D_801BE92C[idx].v = value;
}
