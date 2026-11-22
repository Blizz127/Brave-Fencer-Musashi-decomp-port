#ifndef MUSASHI_FOREST_ANIMATION_COMMANDS_H
#define MUSASHI_FOREST_ANIMATION_COMMANDS_H

#include "forest_action.h"

void func_8015444C(ForestPlayerState *actor, u32 *started, u32 *flags, u32 *suppress);
s32 func_801549F8(ForestPlayerState *actor);
void func_801553C0(ForestPlayerState *actor);
void func_80155394(ForestPlayerState *actor);
void func_801553A8(ForestPlayerState *actor);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80126738[], D_80126748[], D_80126838[], D_80126848[];
void func_80059A1C(u32 operand, void *resource);
#define ANIMATION_COUNTDOWN(actor) (*(const u8 *)&(actor)->animation_flags)
#define ANIMATION_ENTER() ((void)0)
#define ANIMATION_WORD(script, index) (((u32 *)(script))[index])
#define ANIMATION_MODEL_FRAME(model, address) (((u32 *)(model))[8] = (address))
#define ANIMATION_RESOURCE(operand, id) func_80059A1C((operand), D_##id)
#define ANIMATION_ADDRESS(id) ((u32)D_##id)
#define CONTROL_READ(word) (*(word))
#define CONTROL_WRITE(word, value) (*(word) = (value))
#else
#include <string.h>

/* Both recursive entries and script-word reads consume the caller's budget.
 * Read little-endian words from ordinary mapped memory. A script read leaves
 * the actor and script unchanged. Write one word at model_address + 0x20.
 * The owner stops and restores the query on unavailable memory, service or
 * budget. The resource operation preserves its original external effects. */
void bfm_forest_animation_enter(void);
u32 bfm_forest_animation_word(u32 script_address, u32 index);
void bfm_forest_animation_model_frame(u32 model_address, u32 frame_address);
void bfm_forest_animation_resource(u32 operand, u32 resource_address);
#define ANIMATION_COUNTDOWN(actor) ((u8)(actor)->animation_flags)
#define ANIMATION_ENTER() bfm_forest_animation_enter()
#define ANIMATION_WORD(script, index) bfm_forest_animation_word((script), (index))
#define ANIMATION_MODEL_FRAME(model, address) bfm_forest_animation_model_frame((model), (address))
#define ANIMATION_RESOURCE(operand, id) bfm_forest_animation_resource((operand), 0x##id##u)
#define ANIMATION_ADDRESS(id) 0x##id##u

/* A mapped control word can share storage with an actor field. */
static inline u32 forest_animation_control_read(const void *word)
{
    u32 value;
    memcpy(&value, word, sizeof value);
    return value;
}
static inline void forest_animation_control_write(void *word, u32 value)
{
    memcpy(word, &value, sizeof value);
}
#define CONTROL_READ(word) forest_animation_control_read(word)
#define CONTROL_WRITE(word, value) forest_animation_control_write((word), (value))
#endif
#endif
