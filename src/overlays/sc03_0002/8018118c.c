#include "macho_gate.h"

MachoFieldCallback func_8018118C(MachoInteraction *interaction)
{
    if (interaction->actor->phase != 2)
        return 0;
    if (func_8012BD14(interaction) >= 0x4000)
        return 0;
    interaction->actor->phase++;
    func_80178BF8();
    return func_80172710;
}
