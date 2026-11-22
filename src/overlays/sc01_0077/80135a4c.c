/* Test the selected forest model and translate a resolved contact to the endpoint. */
#include "forest_floor.h"
#ifdef __mips__
#define CONTACT_MODEL(address, is_box) ((void *)(address))
typedef FloorPoint ContactPoint;
#else
#define CONTACT_MODEL(address, is_box) bfm_forest_contact_model((address), (is_box))
typedef volatile FloorPoint ContactPoint;
#endif

s32 func_80135A4C(FloorContactRecord *record, s32 packed, FloorPoint *start, FloorPoint *end)
{
    /* Reserve the original local work area above the outgoing arguments. */
    u8 workspace[32];
    void *model;
    void *matrix;
    s32 rotate, kind;
    s16 accumulated;
    s16 attempt;
    s16 stationary;
    s32 result;
    ContactPoint *far_point, *projected;
    (void)workspace;
    kind = func_80135480(record, packed, start, end);
    switch (kind) {
#if defined(__GNUC__) && __GNUC__ >= 4
    /* The classifier returns one of these five model kinds. */
    default: __builtin_unreachable();
#endif
    case 0:
        return 0;
    case 1:
        matrix = &record->rotation;
        model = CONTACT_MODEL(((u32)packed & 0x0fffffffu) | 0x80000000u, packed < 0);
        rotate = 0;
        break;
    case 2:
        matrix = &record->rotation;
        model = CONTACT_MODEL(((u32)packed & 0x0fffffffu) | 0x80000000u, packed < 0);
        rotate = 1;
        break;
    case 3:
        matrix = &record->rotation;
        model = &D_801D9524;
        rotate = 0;
        break;
    case 4:
        matrix = &D_801D9504;
        model = &D_801D9524;
        rotate = 1;
        break;
    }
    accumulated = 0;
    if (packed < 0) {
        if (func_80135EB0(model, -32768)) goto hit;
        model = CONTACT_MODEL(((FloorBox *)model)->next, 1);
        while (model) {
            if (func_80135EB0(model, -32768)) {
hit:
                func_80136A94(rotate, record, end, matrix);
                return 1;
            }
            model = CONTACT_MODEL(((FloorBox *)model)->next, 1);
        }
    } else {
        attempt = 0;
        D_801870AC->w = -32767;
        D_801870B0->w = 32767;
        D_801D9500 = 0;
        D_801D94FC = 0;
        stationary = (s16)start->x == (s16)end->x &&
                     (s16)start->y == (s16)end->y &&
                     (s16)start->z == (s16)end->z;
        while ((result = func_80133AB0(0, (s16)D_801870AC->x,
                                     (s16)D_801870AC->z, model)) != 0) {
            accumulated |= result;
            if (stationary) break;
            /* Moving endpoints may be adjusted up to six times. */
            if (attempt++ >= 5) return 0;
        }
        if (accumulated == 0 && D_801D94FC == 0) return 0;
        if (D_801870AC->w >= -3019)
            D_801152A8 = D_801152B0;
        far_point = D_801870B0;
        projected = D_801870B4;
        projected->x = far_point->x;
        projected->y = far_point->y;
        projected->z = far_point->z;
        goto hit;
    }
    return 0;
}
