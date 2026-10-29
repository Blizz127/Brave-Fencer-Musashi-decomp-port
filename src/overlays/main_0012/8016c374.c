/* func_8016C374 [8016C374,8016C470) - owner re-derivation from retail assembly (clean input set).
 * Decays two 3-component u16 vectors by 10 (clamped at 0), writes both as bytes to c and returns the
 * OR of the second vector's components. */
#include "psx_types.h"

/* callers read v0: yes (second caller branches on it); returns s16. a0 is unused (callers pass it through). */
s16 func_8016C374(void *unused, u16 *a, u16 *b, u8 *c) {
    a[0] -= 10;
    if ((s16)a[0] < 0) {
        a[0] = 0;
    }
    a[1] -= 10;
    if ((s16)a[1] < 0) {
        a[1] = 0;
    }
    a[2] -= 10;
    if ((s16)a[2] < 0) {
        a[2] = 0;
    }
    b[0] -= 10;
    if ((s16)b[0] < 0) {
        b[0] = 0;
    }
    b[1] -= 10;
    if ((s16)b[1] < 0) {
        b[1] = 0;
    }
    b[2] -= 10;
    if ((s16)b[2] < 0) {
        b[2] = 0;
    }
    c[0] = a[0];
    c[1] = a[1];
    c[2] = a[2];
    c[4] = b[0];
    c[5] = b[1];
    c[6] = b[2];
    return b[0] | b[1] | b[2];
}
