/* Change the Bincho Field's glow and rotate its three direction vectors. */
#include "forest_field.h"

typedef struct {
    u32 unused, flags;
} FieldModel;

typedef struct {
    u16 active;
    u8 pad02[0x16];
    u16 scaleX, scaleY;
} FieldGlow;

typedef struct {
    u8 pad00[0x20];
    FieldModel *model;
    u8 pad24[0xa8];
    FieldGlow *glow;
    u8 padD0[0x2c];
    u16 angleX, angleY;
} BinchoField;

extern void func_80048FBC(ForestMatrix *, ForestVector *, ForestVector *);

void func_8017C294(void *owner, s16 mode)
{
    BinchoField *field = owner;
    ForestVector vector;
    ForestMatrix matrix;
    FieldGlow *glow = field->glow;
    FieldModel *model = field->model;
    u32 flags;
    u16 scale;

    if (mode) {
        flags = 0x40000000;
        scale = 0x1000;
    } else {
        flags = 0x50000000;
        scale = 0x1333;
    }
    model->flags = (0x40000000u | (model->flags & ~0x30000000u)) | flags;
    if (field->glow->active)
        glow->scaleX = glow->scaleY = scale;

    vector.x = field->angleX += 0x80;
    vector.y = field->angleY += 0x100;
    vector.z = 0;
    func_80049A1C(&vector, &matrix);
    vector.x = vector.y = 0;
    vector.z = 0x100;
    func_80048FBC(&matrix, &vector, &vector);
    D_801F7380->direction.x = D_801F7390->direction.x = D_801F73A0->direction.x = vector.x;
    D_801F7380->direction.y = D_801F7390->direction.y = D_801F73A0->direction.y = vector.y;
    D_801F7380->direction.z = D_801F7390->direction.z = D_801F73A0->direction.z = vector.z;
}
