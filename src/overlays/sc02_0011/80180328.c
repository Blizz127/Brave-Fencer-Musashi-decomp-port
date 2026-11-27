/* Finish an elemental scroll acquisition when the actor sequence signals completion. */
#include "scroll_actor.h"

extern short D_80126B3A;
extern int D_8011F9C8;
extern void func_80029124(int flag, int enabled);
extern void func_80029514(int progress);
extern void func_80147060(ScrollActor *actor);
extern void func_80147324(int field_signal);
extern void func_80174650(ScrollActor *actor);

void func_80180328(ScrollActor *actor)
{
    if (actor->signals & 0x4000) {
        func_80147324(1093);
    } else if (actor->signals & 0x8000) {
        func_80147060(actor);
        switch ((short)(D_80126B3A - 2)) {
        case 0:
            func_80029124(22, 1);
            func_80029514(133);
            break;
        case 1:
            func_80029124(23, 1);
            break;
        case 2:
            func_80029124(24, 1);
            break;
        case 3:
            func_80029124(25, 1);
            break;
        case 4:
            func_80029124(26, 1);
            break;
        }
        D_8011F9C8 = 1;
        func_80174650(actor);
        func_80171A1C(actor);
    }
}
