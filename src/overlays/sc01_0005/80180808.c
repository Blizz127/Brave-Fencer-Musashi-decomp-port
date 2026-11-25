#include "saved_room_actor.h"

extern unsigned char D_80126B58[];
extern short D_80078E92;

extern unsigned int func_8012C1B8(void);
extern void func_8001C2C4(unsigned int);
extern int func_80029504(void);
extern void func_8014BB24(void *, int, int);
extern void func_8014B944(void *, int, int);
extern void func_8014B034(int);
extern void func_80029344(void);
extern void func_8014BD24(void *, int);

unsigned int func_80180808(struct FileActor *actor)
{
    void *scene = D_80126B58;
    unsigned int result;

    result = func_8012C1B8();
    actor->work = result;
    if (result) {
        int phase;

        func_8001C2C4(result);
        phase = func_80029504();
        if (phase < 5 || phase == 660) {
            if (phase < 5) {
                actor->subtype = 1;
            } else {
                int remaining;

                actor->subtype = 2;
                remaining = 2760 - D_80078E92;
                func_8014BB24(scene, 999, 1);
                func_8014B944(scene, 0x01000000, 1);
                func_8014B034(remaining);
                func_80029344();
                func_8014BD24(scene, 999);
            }
            actor->field10A = 96;
            actor->field106 = 255;
            actor->field108 = 255;
            func_8017FD50(actor);
        }
        result = actor->state + 1;
        actor->state = result;
    }
    return result;
}
