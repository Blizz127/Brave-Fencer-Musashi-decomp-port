#include "mine_stat_event.h"
#include <assert.h>
#include <string.h>

void func_80173EF8(u8 *);
s16 D_80126938, D_80126B3A;
u32 D_80078ED8, D_80078EA4;
static u8 actor[0x230], flag;
static unsigned n, award, flag_id, progression;
static char calls[40];
static int selector, default_arg;
static int amounts[4];

static void record(char name) { assert(n + 1 < sizeof calls); calls[n++] = name; calls[n] = 0; }
void func_80147060(void *p) { assert(p == actor && D_80126938 == 1); record('L'); }
void func_80029124(s32 f,s32 bit) { assert(bit == 1); flag_id = f; record('F'); }
u8 func_80029178(s32 f) { assert(f == 0x11d); record('?'); return flag; }
void func_80029514(s32 v) { progression = v; record('P'); }
void func_8014BCEC(void *p,s32 v) { assert(p == actor && v == 25); record('+'); }
void func_8014BDC8(void) { record('A'); }
void func_8014BB0C(void) { record('B'); }
void func_8014B2A8(void) { record('C'); D_80078ED8=1; D_80078EA4=2; }
static void stat(unsigned i,int v,char name)
{
    assert(i == award && v == amounts[i]);
    ++award;
    record(name);
}
s32 func_8002A088(s32 v) { stat(0,v,'b'); return 0; }
void func_8002A2D4(s32 v) { stat(1,v,'m'); }
void func_8002A544(s32 v) { stat(2,v,'l'); }
void func_8002A7B4(s32 v) { stat(3,v,'f'); }
s32 func_800D0EC4(void) { record('G'); D_80126B3A = -307; return 54; }
void func_800D0F0C(s32 x,s32 y) { assert(x == 54); default_arg = y; record('D'); }
void func_800D128C(s32 x,s32 y) { assert(x == 0x70 && !y); record('S'); }
void func_80145EE8(s32 v) { assert(!v); record('E'); }
void func_80171A1C(void *p) { assert(p == actor); record('Z'); }

int main(void)
{
    static const int points[][4] = {
        {100,32,32,24}, {160,40,64,48}, {240,48,96,72}, {320,64,128,96}, {400,80,160,128}
    };
    static const unsigned limit[] = {200,600,900,1200};
    D_80126938 = 12;
    func_80173EF8(actor);
    assert(!n && D_80126938 == 12);
    actor[0xb9] = 0x80;
    for (selector = 0x78; selector <= 0x7c; ++selector) {
        for (flag = 0; flag <= 1; ++flag) {
            unsigned i = (unsigned)selector - 0x78;
            D_80126B3A = (s16)selector;
            D_80078ED8 = D_80078EA4 = 0;
            progression = 0;
            n = award = 0;
            memcpy(amounts,points[i],sizeof amounts);
            func_80173EF8(actor);
            assert(!strcmp(calls,i < 4 ? "LFP+ABCbmlfEZ" : flag ? "L?bmlfABCFEZ" : "L?+bmlfABCFEZ"));
            assert(flag_id == 0x8a + i && award == 4 && progression == (i < 4 ? limit[i] : 0));
            assert(D_80078ED8 == 0x00a00000 && D_80078EA4 == D_80078ED8);
        }
    }
    n = 0; D_80126B3A = 0; func_80173EF8(actor); assert(!strcmp(calls,"LEZ"));
    n = 0; D_80126B3A = 0x70; func_80173EF8(actor); assert(!strcmp(calls,"LSEZ"));
    n = 0; D_80126B3A = -12; func_80173EF8(actor);
    assert(!strcmp(calls,"LGDEZ") && default_arg == -307);
    return 0;
}
