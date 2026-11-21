/* Fresh source from own retail instructions and qualified declaration prefix.
 * Precondition: obj designates a valid aligned Obj80172588 object.
 * No incoming native allocation or whole-program backing is established here.
 */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x1F4];
    u32 word1F4;
} Obj80172588;

typedef char check_byte_width[(sizeof(u8) == 1) ? 1 : -1];
typedef char check_word_width[(sizeof(u32) == 4) ? 1 : -1];
typedef char check_object_size[(sizeof(Obj80172588) == 0x1F8) ? 1 : -1];

void func_80172588(Obj80172588 *obj)
{
    obj->word1F4 = 0;
}
