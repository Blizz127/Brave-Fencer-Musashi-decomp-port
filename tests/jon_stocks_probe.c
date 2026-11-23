#include "jon_stocks.h"
#include <stddef.h>
#include <stdlib.h>

_Static_assert(offsetof(JonStocksActor, state) == 2, "actor state");
_Static_assert(offsetof(JonStocksActor, branch) == 52, "actor story branch");

unsigned short D_801F1480, D_801F1482;
unsigned char D_801F10C0[1], D_801F10E0[1], D_801F10F8[1];
unsigned char D_801F1118[1], D_801F1138[1], D_801F1158[1];

typedef int (*ProbeService)(unsigned int, unsigned int, unsigned int);
static ProbeService service;
static JonStocksActor *subject;

void jon_probe_run(JonStocksActor *actor, ProbeService observe)
{
    subject = actor;
    service = observe;
    func_801EF558(actor);
}

int func_80029504(void) { return service(0, 0, 0); }
void func_8002D4C8(unsigned short command, unsigned short argument) { service(1, command, argument); }
void func_80029514(int progress) { service(3, (unsigned int)progress, 0); }
unsigned int func_80029124(unsigned int flag, unsigned int set) { return (unsigned int)service(4, flag, set); }
int func_80029178(unsigned int flag) { return service(5, flag, 0); }
int func_8018766C(int first, int second) { return service(6, (unsigned int)first, (unsigned int)second); }
int func_800D0F4C(int item) { return service(7, (unsigned int)item, 0); }

void func_80178CBC(JonStocksActor *actor, unsigned char *script)
{
    unsigned int identity;
    if (actor != subject) abort();
    if (script == D_801F10C0) identity = 0;
    else if (script == D_801F10E0) identity = 1;
    else if (script == D_801F10F8) identity = 2;
    else if (script == D_801F1118) identity = 3;
    else if (script == D_801F1138) identity = 4;
    else if (script == D_801F1158) identity = 5;
    else abort();
    service(2, identity, 0);
}
