typedef unsigned char RoomByte;
typedef unsigned int RoomAddress;

typedef struct RoomPacketLink {
    union {
        RoomAddress offset;
        struct {
            RoomByte index[3];
            RoomByte primitive_code;
        } fields;
    } reference;
    RoomAddress attributes[2];
} RoomPacketLink;

typedef struct RoomPacketGroup {
    RoomAddress attributes[3];
    unsigned int link_count;
    RoomAddress links;
} RoomPacketGroup;

/* Resolve the scene's relative group and textured polygon references once. */
void func_8013C0F8(RoomByte *image)
{
    RoomAddress data_base;
    RoomAddress packet_base;
    unsigned int group_index;
    unsigned int link_index;
    RoomPacketLink *link;
    RoomPacketGroup *group;

    data_base = (RoomAddress)(image + 20);
    if (!image[4]) {
        packet_base = data_base + *(RoomAddress *)(image + 12);
        *(RoomAddress *)(image + 12) = packet_base;
        *(RoomAddress *)(image + 16) += data_base;
        group = (RoomPacketGroup *)data_base;
        for (group_index = 0; group_index < *(unsigned int *)(image + 8);
             group_index++, group++) {
            group->links += data_base;
            link = (RoomPacketLink *)group->links;
            for (link_index = 0; link_index < group->link_count;
                 link_index++, link++) {
                switch (link->reference.fields.primitive_code) {
                /* Opaque textured triangles and quads, flat or Gouraud. */
                case 0x24:
                case 0x2c:
                case 0x34:
                case 0x3c:
                    link->reference.offset = (link->reference.offset & 0x00ffffff) + packet_base;
                    break;
                /* Semi-transparent variants carry the same relative reference. */
                case 0x26:
                case 0x2e:
                case 0x36:
                case 0x3e:
                    link->reference.offset = (link->reference.offset & 0x00ffffff) + packet_base;
                    break;
                }
            }
        }
        image[4] = 1;
    }
}
