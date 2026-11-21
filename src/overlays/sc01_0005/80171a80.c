/* Own retail leaf; retain the qualified root API.
 * Input must denote a valid aligned Obj with the qualified signed word field.
 * Incoming native backing and parent compatibility are unproved.
 * Source-only and unselected; no runtime execution or admission.
 */
#include "psx_types.h"
typedef struct {
    u8 pad00[0x200];
    s32 f200;
} Obj;
typedef char BfmWordFieldLayout[
    (sizeof(u8) == 1 && sizeof(s32) == 4 && sizeof(Obj) == 0x204) ? 1 : -1];
void func_80171A80(Obj *o, s32 v)
{
    o->f200 = v;
}
