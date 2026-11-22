/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards its low byte to func_80029124 with selector 0xD. */
#include "event_flags.h"

void func_80028E00(s32 arg0) {
    func_80029124(0xD, arg0 & 0xFF);
}
