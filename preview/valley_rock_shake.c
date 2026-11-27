/* SPDX-License-Identifier: MIT */
#include "valley_rock_shake.h"

/* Each mounted effect is a sequence of (world Y, hold fields) pairs. A zero
 * hold ends the effect. The room stores two effects followed by its actor
 * state table; the player's Earth impact uses a separate MAIN/10 effect. */
static const uint8_t *effect(const uint8_t *payload, size_t bytes,
                             size_t offset, unsigned max_pairs)
{
    if (!payload || offset >= bytes) return NULL;
    size_t pairs = (bytes - offset) / 2;
    for (unsigned i = 0; i < max_pairs && i < pairs; ++i)
        if (!payload[offset + 2 * i + 1])
            return payload + offset;
    return NULL;
}

int bfm_valley_rock_shake_load(BfmValleyRockShake *shake, const uint8_t *scene,
    size_t scene_bytes, const uint8_t *main, size_t main_bytes)
{
    if (!shake) return 0;
    const size_t roll = 0x80186114u - 0x80128158u;
    const size_t landing = 0x80186100u - 0x80128158u;
    const size_t states = 0x80186150u - 0x80128158u;
    const size_t gate = 0x801862acu - 0x80128158u;
    const size_t earth = 0x800d3414u - 0x800cedf8u;
    *shake = (BfmValleyRockShake){
        .gate = effect(scene, scene_bytes, gate, 40),
        .earth = effect(main, main_bytes, earth, 9),
        .roll = effect(scene, scene_bytes, roll, (states - roll) / 2),
        .landing = effect(scene, scene_bytes, landing, (roll - landing) / 2)
    };
    return shake->earth && shake->roll && shake->landing && shake->gate;
}

void bfm_valley_rock_shake_request(BfmValleyRockShake *shake, BfmValleyShakeEvent event)
{
    if (!shake) return;
    switch (event) {
    case BFM_VALLEY_SHAKE_EARTH_STRIKE: shake->next = shake->earth; break;
    case BFM_VALLEY_SHAKE_ROLL: shake->next = shake->roll; break;
    case BFM_VALLEY_SHAKE_GATE: shake->next = shake->gate; break;
    case BFM_VALLEY_SHAKE_LANDING: shake->next = shake->landing; break;
    default: return;
    }
    shake->hold = 0;
}

int bfm_valley_rock_shake_step(BfmValleyRockShake *shake)
{
    if (!shake) return 0;
    /* The original submits its camera before advancing the effect. A new
     * request replaces the envelope but does not change the submitted view. */
    int current = shake->world_y;
    if (shake->hold) {
        --shake->hold;
    } else if (shake->next) {
        shake->hold = shake->next[1];
        if (shake->hold) {
            int value = shake->next[0];
            shake->world_y = value < 128 ? value : value - 256;
            shake->next += 2;
        } else {
            shake->next = NULL;
            shake->world_y = 0;
        }
    }
    return current;
}
