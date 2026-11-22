#ifndef BFM_FOREST_SECONDARY_EVENT_H
#define BFM_FOREST_SECONDARY_EVENT_H
#include "forest_player_state.h"
#include "forest_event.h"

extern s16 D_800B9A08;
extern s32 D_8012707C;
s32 func_80149954(ForestPlayerState *actor);
void func_80015978(u16 *source, u16 *position);
void func_8002D4C8(u16 command, u16 parameter);
void func_8012E5CC(void *position, u16 command, u16 parameter);

#if defined(__mips__) && !defined(__LP64__)
extern u16 D_80126B5C[];
/* The MAIN body accepts two unsigned halfwords. This issuer ignores its result. */
void func_80147364(u16 command, u16 parameter);
#define FOREST_SECONDARY_PLAYER_SOUND(command, value) func_80147364((command), (value))
void func_800CCF28(ForestPlayerState *actor);
#define FOREST_SECONDARY_POSITION(v) func_80015978(D_80126B5C, (v))
#define FOREST_SECONDARY_SOUND(command, value) func_8002D4C8((command), (value))
#define FOREST_SECONDARY_UPDATE(actor) func_800CCF28(actor)
#else
#include <string.h>
/* The resident gather reads three halfwords at +6, +10 and +14 of the global player. */
static inline void bfm_secondary_player_position(u16 *position)
{
    const unsigned char *player = (const unsigned char *)&D_80126B58;
    memcpy(position, player + 6, 2);
    memcpy(position + 1, player + 10, 2);
    memcpy(position + 2, player + 14, 2);
}
#define FOREST_SECONDARY_POSITION(v) bfm_secondary_player_position(v)
/* Service owners must refuse unqualified current overlays or sound backends. */
void bfm_forest_secondary_sound(u16 command, u16 parameter);
void bfm_forest_secondary_command(u16 command, u16 parameter);
#define FOREST_SECONDARY_PLAYER_SOUND(command, value) bfm_forest_secondary_command((command), (value))
void bfm_forest_secondary_update(ForestPlayerState *actor);
#define FOREST_SECONDARY_SOUND(command, value) bfm_forest_secondary_sound((command),(value))
#define FOREST_SECONDARY_UPDATE(actor) bfm_forest_secondary_update(actor)
#endif
#endif
