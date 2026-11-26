/* SPDX-License-Identifier: MIT */
#include "steam_knight_armor.h"
#include "../pc_port/forest_player_bytes.h"

enum { FIRST_PLATE = 8, CHASSIS_PARTS = 22, REGISTRATION_BYTES = 52 };

static const uint8_t *room_record(const uint8_t *scene, size_t bytes, uint32_t address, size_t n)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < UINT32_C(0x80190894) || (address & 1) || n > bytes ||
        address - origin > bytes - n)
        return NULL;
    return scene + (address - origin);
}

int bfm_steam_knight_armor_load(BfmSteamKnightArmor *armor, const uint8_t *scene, size_t bytes)
{
    if (!armor || !scene) return 0;
    BfmSteamKnightArmor mounted = {0};
    const unsigned first = FIRST_PLATE;
    const uint8_t *joints = room_record(scene, bytes, UINT32_C(0x80195b4c) + 2 * first,
                                         2 * BFM_STEAM_ARMOR_PLATES);
    if (!joints) return 0;
    for (unsigned i = 0; i < BFM_STEAM_ARMOR_PLATES; ++i) {
        unsigned variant = first + i;
        const uint8_t *resource = room_record(scene, bytes,
            UINT32_C(0x80195d74) + REGISTRATION_BYTES * variant, 12);
        if (!resource) return 0;
        BfmSteamKnightArmorPlate *plate = &mounted.plate[i];
        plate->joint = u16le(joints + 2 * i);
        plate->hp = s16le(resource);
        plate->flags = u16le(resource + 2);
        uint32_t shape = u32le(resource + 8);
        if (plate->joint >= CHASSIS_PARTS || plate->hp <= 0 ||
            shape != UINT32_C(0x80195b74) + variant * 16)
            return 0;
        const uint8_t *box = room_record(scene, bytes, shape, 16);
        if (!box || u32le(box) != 0) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            plate->box.low[axis] = s16le(box + 4 + 4 * axis);
            plate->box.high[axis] = s16le(box + 6 + 4 * axis);
            if (plate->box.low[axis] >= plate->box.high[axis]) return 0;
        }
    }
    *armor = mounted;
    return 1;
}

int bfm_steam_knight_armor_load_power(const MusashiDiscMedia *disc, const uint8_t *scene,
                                    size_t bytes, BfmForestFusionPower *power)
{
    if (!scene) return 0;
    const uint8_t *moves = room_record(scene, bytes, UINT32_C(0x801927a0), 40);
    if (!moves) return 0;
    const uint8_t *rows[2];
    for (unsigned i = 0; i < 2; ++i) {
        unsigned kind = u16le(moves + i * 20);
        if (kind != i + 1) return 0;
        rows[i] = room_record(scene, bytes, UINT32_C(0x80192944) + 12 * kind, 12);
    }
    return bfm_forest_fusion_power_rows(disc, moves, rows, power);
}

void bfm_steam_knight_armor_begin_swing(BfmSteamKnightArmor *armor)
{
    if (armor) armor->struck = 0;
}

int bfm_steam_knight_armor_strike(BfmSteamKnightArmor *armor, const BfmSteamKnightMachine *machine,
                                  const int16_t from[3], const int16_t ends[][3], size_t ray_count,
                                  unsigned attack_class, uint16_t damage, BfmSteamKnightArmorHit *hit)
{
    if (!armor || !machine || !from || !ends || !hit || !ray_count ||
        ray_count > BFM_FOREST_STRIKE_RAYS || (attack_class != 1 && attack_class != 2)) return 0;

    for (size_t ray = 0; ray < ray_count; ++ray) {
        unsigned nearest = BFM_STEAM_ARMOR_PLATES;
        uint64_t distance = UINT64_MAX;
        BfmSteamKnightArmorHit contact = {0};
        for (unsigned i = 0; i < BFM_STEAM_ARMOR_PLATES; ++i) {
            const BfmSteamKnightArmorPlate *plate = &armor->plate[i];
            if ((armor->struck & (1u << i)) || !(plate->flags & 0x8000) || plate->hp <= 0)
                continue;
            BfmForestTransform root;
            const MusashiForestPlayerPart *model;
            if (!bfm_steam_knight_machine_joint(machine, plate->joint, &root, &model))
                continue;
            BfmSteamKnightArmorHit probe = {0};
            if (!bfm_forest_guard_contact_point(&plate->box, &root, from, ends[ray],
                                                 probe.center, probe.point, probe.normal))
                continue;
            uint64_t squared = 0;
            for (unsigned axis = 0; axis < 3; ++axis) {
                int64_t delta = (int64_t)from[axis] - probe.point[axis];
                squared += (uint64_t)(delta * delta);
            }
            if (squared < distance) {
                nearest = i;
                distance = squared;
                contact = probe;
            }
        }
        if (nearest == BFM_STEAM_ARMOR_PLATES) continue;
        BfmSteamKnightArmorPlate *plate = &armor->plate[nearest];
        contact.variant = FIRST_PLATE + nearest;
        contact.joint = plate->joint;
        contact.attack_class = attack_class;
        contact.before = plate->hp;
        plate->hp -= damage;
        contact.after = plate->hp;
        armor->struck |= 1u << nearest;
        /* Breaking an outer plate exposes its inner plate to the next blow. */
        if (!(nearest & 1) && plate->hp <= 0) {
            armor->plate[nearest + 1].flags = 0x8800;
            armor->struck |= 1u << (nearest + 1);
        }
        *hit = contact;
        return 1;
    }
    return 0;
}
