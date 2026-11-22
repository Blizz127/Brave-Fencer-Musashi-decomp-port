/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards the low byte of its argument to func_80029124 with selector 0xE. */
#include "event_flags.h"

void func_80028E44(u32 a) {
    func_80029124(0xE, a & 0xFF);
}
