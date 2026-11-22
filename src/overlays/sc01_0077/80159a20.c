/* Four shared updates and a kind-specific callback run before the flag decision. */
#include "../../../include/forest_player_state.h"

void func_80159A20(ForestPlayerState *actor)
{
    func_80150B28(actor);
    func_80158638(actor);
    func_8015126C(actor);
    func_8014900C(actor);
    FOREST_PLAYER_CALL(8018925C, actor->kind & 0x7fff, actor);
    actor->flags |= 0x08000000;
    switch (actor->kind) {
    case 2:
    case 3:
    case 11:
    case 12:
        actor->flags &= 0xf7ffffff;
        break;
    case 25:
        switch (actor->mode) {
        case 1:
            if (actor->action == 3)
                actor->flags &= 0xf7ffffff;
            break;
        case 4:
            actor->flags &= 0xf7ffffff;
            break;
        }
        break;
    }
}
