#include "tomb_opening.h"

/* Start the key event when the room script is free and Musashi is in range. */
void func_8017EC30(CemeteryActor *key)
{
    if (!func_8016F1AC() && func_8012BD14(key) <= 1024) {
        func_8017D884();
        key->phase = 2;
    }
}
