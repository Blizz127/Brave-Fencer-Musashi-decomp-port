/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 0 for a fixed set of 26 ids and nonzero for any other id. */
#include "psx_types.h"

s32 func_8001923C(s32 arg0) {
    if (arg0 == 0xB3) {
        return 0;
    }
    if (arg0 == 0xB4) {
        return 0;
    }
    if (arg0 == 0x12E) {
        return 0;
    }
    if (arg0 == 0x12F) {
        return 0;
    }
    if (arg0 == 0x150) {
        return 0;
    }
    if (arg0 == 0x152) {
        return 0;
    }
    if (arg0 == 0x16E) {
        return 0;
    }
    if (arg0 == 0x16F) {
        return 0;
    }
    if (arg0 == 0xB9) {
        return 0;
    }
    if (arg0 == 0xA1) {
        return 0;
    }
    if (arg0 == 0xA2) {
        return 0;
    }
    if (arg0 == 0xA3) {
        return 0;
    }
    if (arg0 == 0x10B) {
        return 0;
    }
    if (arg0 == 0x10C) {
        return 0;
    }
    if (arg0 == 0x10D) {
        return 0;
    }
    if (arg0 == 0x10F) {
        return 0;
    }
    if (arg0 == 0x110) {
        return 0;
    }
    if (arg0 == 0x111) {
        return 0;
    }
    if (arg0 == 0x112) {
        return 0;
    }
    if (arg0 == 0x113) {
        return 0;
    }
    if (arg0 == 0x114) {
        return 0;
    }
    if (arg0 == 0xAA) {
        return 0;
    }
    if (arg0 == 0x115) {
        return 0;
    }
    if (arg0 == 0x116) {
        return 0;
    }
    if (arg0 != 0x117) {
        return arg0 != 0x118;
    }
    return 0;
}
