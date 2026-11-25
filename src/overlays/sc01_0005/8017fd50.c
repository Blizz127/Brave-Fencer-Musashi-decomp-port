#include "primitive_ordering_table.h"
#include "saved_room_actor.h"

typedef union {
    u32 word;
    struct {
        u32 address : 24;
        u32 count : 8;
    } link;
} PacketTag;

typedef struct {
    PacketTag tag;
    u32 command;
} DrawModePacket;

typedef struct {
    u8 r, g, b, command;
    s16 x, y;
} FadeVertex;

typedef struct {
    PacketTag tag;
    FadeVertex vertex[4];
} FadeQuad;

extern PrimitiveOrderingBank D_800AE7BC[];

static inline PacketTag *fade_bucket(const volatile u16 *selector)
{
    u16 bank = *selector;
    return (PacketTag *)(unsigned long)D_800AE7BC[bank].table;
}

static inline void prepend_packet(PacketTag *tag, const volatile u16 *selector)
{
    tag->link.address = fade_bucket(selector)->link.address;
    fade_bucket(selector)->link.address = (unsigned long)tag;
}

void func_8017FD50(struct FileActor *actor)
{
    u8 *cursor = func_80010A08(2 * sizeof(DrawModePacket) + sizeof(FadeQuad));
    DrawModePacket *mode = (DrawModePacket *)cursor;
    FadeQuad *quad;
    DrawModePacket *blend;

    /* Prepending the three packets puts the blend mode first in GPU draw order. */
    mode->tag.link.count = 1;
    mode->command = 0xe1000000u | 0x15u;
    prepend_packet(&mode->tag, &D_800B9A02);
    cursor += sizeof(*mode);

    quad = (FadeQuad *)cursor;
    quad->tag.word = 8u << 24;
    quad->vertex[0].command = 0x3a; /* Semi-transparent Gouraud quad. */
    quad->vertex[0].r = quad->vertex[1].r =
        quad->vertex[0].g = quad->vertex[1].g =
        quad->vertex[0].b = quad->vertex[1].b = actor->field108;
    quad->vertex[2].r = quad->vertex[3].r =
        quad->vertex[2].g = quad->vertex[3].g =
        quad->vertex[2].b = quad->vertex[3].b = actor->field106;
    quad->vertex[0].x = quad->vertex[2].x = -160;
    quad->vertex[1].x = quad->vertex[3].x = 160;
    quad->vertex[0].y = quad->vertex[1].y = -120;
    quad->vertex[2].y = quad->vertex[3].y = 120;
    blend = (DrawModePacket *)(quad + 1);
    prepend_packet(&quad->tag, &D_800B9A02);
    cursor += sizeof(*quad);

    ((PacketTag *)cursor)->link.count = 1;
    blend->command = 0xe1000000u | 0x240u;
    prepend_packet(&blend->tag, &D_800B9A02);
}
