#include "steamwood_valve.h"

#if defined(__mips__) && !defined(__LP64__)
#define VALVE_SCRIPT(address) ((const unsigned int *)(address))
#else
extern const unsigned int *steamwood_script_words(unsigned int address);
#define VALVE_SCRIPT(address) steamwood_script_words(address)
#endif

void func_80171928(struct ValvePlayer *player)
{
    player->action = VALVE_SCRIPT(player->script_address)[player->cursor]
                   | (player->action & 0x7f000000);
    func_80171A1C(player);
}
