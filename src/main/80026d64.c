#include "model_draw.h"

/* Headers and primitive addresses refer to the little-endian guest layout. */
typedef union {
    u32 word;
    u16 count;
} PrimitiveGroup;

typedef struct {
    u32 vertices;
    u32 vertex_count;
    u32 normals;
    u32 normal_count;
    u32 groups;
    u32 primitive_count;
} ModelPart;

extern u32 D_800A5E60;
typedef u32 PrimitiveWriter(PrimitiveGroup *group, u32 vertices, u32 normals,
                            u32 cursor, u32 count, s32 mode, u32 ordering_table);
extern PrimitiveWriter func_80027058, func_800273F4, func_80027200, func_800275BC;
extern PrimitiveWriter func_800277DC, func_800279AC;

static __inline__ u32 group_count(PrimitiveGroup *group)
{
    return *(u16 *)group;
}

void func_80026D64(DrawArg *arg, OtRec *ot, s32 mode)
{
    ModelPart *part = (ModelPart *)(unsigned long)arg->unk8;
    PrimitiveGroup *group = (PrimitiveGroup *)(unsigned long)part->groups;
    u32 remaining = part->primitive_count;
    u32 vertices = part->vertices;
    u32 normals = part->normals;

    while (remaining) {
        u32 word = group->word;
        u32 flags = (word >> 16) & 4;
        switch ((word >> 24) & 0xfd) {
        case 0x20:
            if (flags) {
                D_800A5E60 = func_800273F4(
                    group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
                remaining -= group_count(group);
                group += group_count(group) * 6;
            } else {
                D_800A5E60 = func_80027058(
                    group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
                remaining -= group_count(group);
                group += group_count(group) * 4;
            }
            break;
        case 0x28:
            if (flags) {
                D_800A5E60 = func_800275BC(
                    group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
                remaining -= group_count(group);
                group += group_count(group) * 8;
            } else {
                D_800A5E60 = func_80027200(
                    group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
                remaining -= group_count(group);
                group += group_count(group) * 5;
            }
            break;
        case 0x24:
            remaining -= group_count(group);
            group += group_count(group) * 6;
            break;
        case 0x30:
            remaining -= group_count(group);
            group += group_count(group) * 5;
            break;
        case 0x38:
            remaining -= group_count(group);
            group += group_count(group) * 6;
            break;
        case 0x34:
            remaining -= group_count(group);
            group += group_count(group) * 7;
            break;
        case 0x3c:
            remaining -= group_count(group);
            group += group_count(group) * 9;
            break;
        case 0x25:
            D_800A5E60 = func_800277DC(
                group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
            remaining -= group_count(group);
            group += group_count(group) * 7;
            break;
        case 0x2d:
            D_800A5E60 = func_800279AC(
                group, vertices, normals, D_800A5E60, group_count(group), mode, ot->unk4);
            remaining -= group_count(group);
            group += group_count(group) * 8;
            break;
        case 0x2c:
            remaining -= group_count(group);
            group += group_count(group) * 8;
            break;
        }
    }
}
