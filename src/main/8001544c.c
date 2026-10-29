/* func_8001544C: first free (active == 0) 16-byte entry of the D_800B93D8 table, or 0. */
#include "psx_types.h"

typedef struct Entry {
    s16 active;
    u8 rest[14];
} Entry;

extern Entry D_800B93D8[], D_800B97D8[];

Entry *func_8001544C(void) {
    Entry *e;
    volatile int unused;
    for (e = D_800B93D8; e < D_800B97D8; e++) {
        if (e->active == 0) {
            return e;
        }
    }
    return 0;
}
