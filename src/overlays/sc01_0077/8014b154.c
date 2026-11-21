/* Fresh own-retail derivation with the qualified standalone object declaration.
 * Requires a valid aligned Obj8014B154; incoming native backing is unproved.
 */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x44];
    u32 flags44;
} Obj8014B154;

typedef char check_byte_width[(sizeof(u8) == 1) ? 1 : -1];
typedef char check_word_width[(sizeof(u32) == 4) ? 1 : -1];
typedef char check_object_size[(sizeof(Obj8014B154) == 0x48) ? 1 : -1];

u32 func_8014B154(Obj8014B154 *obj)
{
    return obj->flags44 & 0x80U;
}
