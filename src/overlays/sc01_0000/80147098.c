/* Own retail leaf; retain the qualified root API.
 * Input must denote a valid Obj80147098 with the qualified flag220 field.
 * Incoming native backing and parent compatibility are unproved.
 * Source-only and unselected; no runtime execution or admission.
 */
#include "psx_types.h"
typedef struct {
    u8 pad000[0x220];
    u8 flag220;
} Obj80147098;
typedef char BfmByteFieldLayout[
    (sizeof(u8) == 1 && sizeof(Obj80147098) == 0x221) ? 1 : -1];
void func_80147098(Obj80147098 *obj)
{
    obj->flag220 = 0;
}
