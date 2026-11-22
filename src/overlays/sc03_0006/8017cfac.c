#include "forest_field.h"

typedef struct {
    u8 index;
} RescueRecord;

typedef struct {
    u16 kind;
    u16 phase;
    u8 pad04[0xd8];
    const RescueRecord *record;
} CrystalActor;

extern void func_800291A0(u32, u32);
extern void func_8002955C(void);
extern u32 func_8002953C(void);
extern void func_8002AC00(u32);

ForestFieldPhase func_8017CFAC;

void func_8017CFAC(void *slot)
{
    const RescueRecord *record = ((CrystalActor *)slot)->record;

    if (func_80178970(slot)) {
        func_80178D18(slot);
        func_800291A0(0x3a + record->index, 1);
        func_8002955C();
        if (func_8002953C() >= 35) {
            func_8002AC00(0x25);
        }
        ((CrystalActor *)slot)->phase++;
    }
}
