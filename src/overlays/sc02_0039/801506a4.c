/* Mark a contacted item and apply the action selected by its object type. */
#include "actor_pickup.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern u16 D_80180D5C[];
extern u16 D_80180D64[];
extern u16 D_80180D6C[];
#define BFM_PICKUP_TABLE_49 D_80180D5C
#define BFM_PICKUP_TABLE_50 D_80180D64
#define BFM_PICKUP_TABLE_51 D_80180D6C
#define READ_PICKUP_OR_RETURN(table, dest) ((dest) = (table)[*(s16 *)((u8 *)ent + 0x70)])
#else
static int pickup_value(const Obj8014A2E4 *ent, unsigned table, u16 *value)
{
    const u8 *bytes = (const u8 *)ent + 0x70;
    u16 bits = (u16)((u16)bytes[0] | (u16)bytes[1] << 8);
    s16 index = (s16)((bits & 0x8000u) ? (s32)bits - 65536 : (s32)bits);
    if (!bfm_pickup_table_value(table, index, value)) {
        bfm_pickup_table_refused(table, index);
        return 0;
    }
    return 1;
}
#define READ_PICKUP_OR_RETURN(table, dest) \
    do { \
        if (!pickup_value(ent, table, &(dest))) { \
            return; \
        } \
    } while (0)
#endif

void func_801506A4(void *self, Obj8014A2E4 *ent)
{
    u16 value;
    ent->flags |= 1;
    switch (ent->active) {
    case 49:
        func_80147324(1106);
        READ_PICKUP_OR_RETURN(BFM_PICKUP_TABLE_49, value);
        func_8014ADA8(self, value);
        READ_PICKUP_OR_RETURN(BFM_PICKUP_TABLE_49, value);
        func_8014ACE8(self, 3, value);
        break;
    case 50:
        if (D_80078EB4) {
            func_80147324(1106);
            READ_PICKUP_OR_RETURN(BFM_PICKUP_TABLE_50, value);
            func_8014BB24(self, value, 1);
            READ_PICKUP_OR_RETURN(BFM_PICKUP_TABLE_50, value);
            func_8014ACE8(self, 2, value);
        }
        break;
    case 51:
        func_80147324(1106);
        READ_PICKUP_OR_RETURN(BFM_PICKUP_TABLE_51, value);
        func_80150820(self, value);
        break;
    case 197:
    case 635:
        func_80147324(1106);
        func_8015086C(self);
        break;
    }
}
