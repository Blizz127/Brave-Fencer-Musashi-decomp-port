#include "action22_contact.h"

void func_8015FCC8(Action21Actor *base)
{
    TwinpeakPollActor *actor = (TwinpeakPollActor *)base;

    if (D_800AE6B0 & 0x80ffffffu) {
        func_8016007C(base);
        D_80189EAC[base->current_action](base);
        func_80147078(base, 0);
        func_80159B70(base);
    } else {
        actor->contact_flag_197 = 0;
        actor->contact_flag_be = 0;
        actor->contact_flag_bf = 0;
        actor->contact = (unsigned char)actor->selected_contact;
        func_8014BFF4(base, 1);
        func_80146CA0(base);
    }
}
