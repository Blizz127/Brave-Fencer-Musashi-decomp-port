#include "message_defaults.h"

/* The established slot type holds these fields in opaque byte ranges.
 * All halfword and word offsets below are naturally aligned. */
#if defined(__GNUC__) && __GNUC__ >= 3
typedef unsigned short SlotHalfword __attribute__((__may_alias__));
typedef unsigned int SlotWord __attribute__((__may_alias__));
#else
typedef unsigned short SlotHalfword;
typedef unsigned int SlotWord;
#endif

static __inline__ void write_halfword(unsigned char *field, unsigned short value)
{
    *(SlotHalfword *)field = value;
}

void func_80137840(TutorialHandleSlot *slot)
{
    unsigned char *bytes = (unsigned char *)slot;

    bytes[0x0c] = 1;
    bytes[0x23] = 1;
    slot->active_state = 1;
    write_halfword(bytes + 0x2a, 0x1000);
    write_halfword(bytes + 0x28, 0x1000);
    write_halfword(bytes + 0x2c, 0x24);
    write_halfword(bytes + 0x2e, 3);
    slot->state = 0;
    slot->reserved06 = 0;
    write_halfword(bytes + 0x1c, 0);
    write_halfword(bytes + 0x10, 0);
    write_halfword(bytes + 0x14, 0);
    write_halfword(bytes + 0x12, 0);
    write_halfword(bytes + 0x16, 0);
    write_halfword(bytes + 0x30, 0);
    write_halfword(bytes + 0x32, 0);
    write_halfword(bytes + 0x34, 0);
    write_halfword(bytes + 0x36, 0);
    bytes[0x22] = 0;
    bytes[0x20] = 0;
    *(SlotWord *)(bytes + 0x40) = 0;
    ((signed char *)bytes)[0x1f] = -1;
    write_halfword(bytes + 0x44, 0);
    write_halfword(bytes + 0x46, 0);
    write_halfword(bytes + 0x48, 0);
    write_halfword(bytes + 0x4a, 0);

    func_80139634(slot);
    bytes[0x0d] = 0;
    func_80139DC8();
}
