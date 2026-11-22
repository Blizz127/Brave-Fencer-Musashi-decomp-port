/* Tick an intact Bincho Field. The caller supplies an active type-128 slot. */
#include "forest_field.h"

typedef struct {
    u16 fractionX, x, fractionY, y, fractionZ, z;
} FieldPosition;

typedef struct {
    u8 pad00[0x12];
    u16 yaw;
} FieldModel;

typedef struct {
    u16 type, phase;
    FieldPosition position;
    u8 pad10[0xc];
    s32 timer;
    FieldModel *model;
    u8 pad24[0x3a];
    u16 hit;
    u8 pad60[0xa6];
    s16 proximityCount;
    u8 pad108[2];
    s16 hintSeen;
} BinchoField;

extern u16 D_800B99DA;
extern BinchoField D_801202A0[96];
extern FieldPosition D_80126B5C;
extern u16 D_80126B62;
extern s16 D_800B9A08;
extern u8 D_80190050[], D_801900B4[];
extern s32 func_80013328(FieldPosition *, FieldPosition *);
extern void func_801746A4(void);
extern s32 func_8012BD14(BinchoField *);

ForestFieldPhase func_8017C750;

void func_8017C750(void *actor_slot)
{
    BinchoField *field = actor_slot;
    FieldModel *model = field->model;
    BinchoField *slot;
    s32 distance, level;
    s8 first;
    s16 i;

    if (!(D_800B99DA & 15)) {
        for (slot = D_801202A0; slot < D_801202A0 + 96; slot++) {
            if (slot->type == 0x128) {
                first = slot == field;
                break;
            }
        }
        distance = func_80013328(&field->position, &D_80126B5C);
        if (distance < 0x800)
            level = 0x44 - ((distance >> 5) & 0x38);
        else
            level = 4;
        if (first || D_80115214 < level)
            D_80115214 = level;
    }

    model->yaw += 0x20;
    func_8017C294(field, func_8005C49C() & 15);
    if (field->hit) {
        for (i = 0; (u16)i < 8; i++) {
            if (field->hit == D_80190050[i]) {
                func_801746A4();
                field->timer = 32;
                field->phase++;
            }
        }
        field->hit = 0;
    } else if (!field->hintSeen) {
        for (i = 5; i < 40; i++) {
            if (func_800291B4(i + 59))
                break;
        }
        if (i != 40) {
            field->hintSeen = 1;
        } else if (func_8012BD14(field) < 104 * 104) {
            if (D_800B9A08 == 0x3028) {
                if ((u16)(field->position.y - D_80126B62 + 4) < 4)
                    field->proximityCount++;
                if (field->proximityCount < 2)
                    return;
            }
            func_80178B18(field, D_801900B4);
            field->hintSeen = 1;
            field->phase = 10;
        } else {
            field->proximityCount = 0;
        }
    }
}
