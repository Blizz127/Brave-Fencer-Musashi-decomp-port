/* SPDX-License-Identifier: MIT */
#include "steam_knight_throw.h"
#include <limits.h>
#include <string.h>

enum { UNUSED, ROLL, LIFTABLE, AIM, AIR, SLIDE, ARRIVED, REPLAY, LANDED, FINISHED };
enum { ONE = 4096, GRAVITY = 3 * ONE / 2 };

typedef struct { int wall, aim_forward, aim_side, destination; } Course;

/* The hall and yard face +Z and the final wall faces -X. The sights fit
 * the measured flight from the held root in those forward and lateral axes. */
static const Course courses[] = {
    {-1792, -1840, -128, -960}, {1152, 1200, -128, 1920}, {1280, 1360, 2912, 2689}
};
static unsigned along(const BfmSteamKnightThrow *m) { return m->round == 2 ? 0 : 2; }
static int sign(const BfmSteamKnightThrow *m) { return m->round == 2 ? -1 : 1; }
static int32_t units(int32_t q) { return q / ONE - (q % ONE < 0); }

static void publish(BfmSteamKnightThrow *m)
{
    for (unsigned i = 0; i < 3; ++i) {
        int32_t n = units(m->position[i]);
        if (n < INT16_MIN) n = INT16_MIN;
        if (n > INT16_MAX) n = INT16_MAX;
        m->body[i] = (int16_t)n;
    }
}

static void place(BfmSteamKnightThrow *m, const int16_t root[3])
{
    for (unsigned i = 0; i < 3; ++i) m->position[i] = (int32_t)root[i] * ONE;
    publish(m);
}

static uint32_t square_root(uint64_t n)
{
    uint64_t bit = (uint64_t)1 << 62, root = 0;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else root >>= 1;
        bit >>= 2;
    }
    return (uint32_t)root;
}

int bfm_steam_throw_collapse_q12(BfmSteamKnightThrow *m, unsigned round, const int32_t root[3])
{
    if (!m || !root || round >= 3 || (m->phase != UNUSED && m->phase != FINISHED))
        return 0;
    for (unsigned i = 0; i < 3; ++i)
        if (units(root[i]) < INT16_MIN || units(root[i]) > INT16_MAX) return 0;
    int axis = round == 2 ? 0 : 2, direction = round == 2 ? -1 : 1;
    if (direction * root[axis] >= courses[round].wall * ONE) return 0;
    int32_t start[3] = {root[0], root[1], root[2]};
    *m = (BfmSteamKnightThrow){0};
    m->round = round;
    m->phase = ROLL;
    m->floor_y = start[1];
    memcpy(m->position, start, sizeof start);
    publish(m);
    return 1;
}

int bfm_steam_throw_collapse(BfmSteamKnightThrow *m, unsigned round, const int16_t root[3])
{
    if (!root) return 0;
    int32_t start[3];
    for (unsigned i = 0; i < 3; ++i) start[i] = (int32_t)root[i] * ONE;
    return bfm_steam_throw_collapse_q12(m, round, start);
}

static unsigned roll(BfmSteamKnightThrow *m)
{
    unsigned axis = along(m);
    int remaining = courses[m->round].wall - sign(m) * units(m->position[axis]);
    if (remaining >= 320)
        m->position[axis] += sign(m) * (10 * ONE + ONE / 2);
    ++m->age;
    /* Two diminishing bounces settle into shallow alternating contacts.
     * Sub-unit starting height changes the floor contact ending the roll. */
    unsigned bounce_end = m->floor_y - units(m->floor_y) * ONE < ONE / 2 ? 26 : 25;
    int32_t offset;
    if (m->age <= 15) {
        int n = (int)m->age;
        offset = -12 * ONE * n + GRAVITY * n * (n + 1) / 2;
    } else if (m->age <= bounce_end) {
        int n = (int)m->age - 16;
        offset = -8 * ONE * n + GRAVITY * n * (n + 1) / 2;
    } else {
        unsigned since_contact = m->age - bounce_end - 1;
        offset = since_contact % 2 ? (since_contact % 4 == 1 ? ONE : 2 * ONE) : 0;
    }
    if (remaining < 320 && offset == 0) {
        m->phase = LIFTABLE;
        m->position[1] = m->floor_y;
        publish(m);
        return BFM_STEAM_THROW_LIFTABLE;
    }
    m->position[1] = m->floor_y + offset;
    publish(m);
    return 0;
}

int bfm_steam_throw_release(BfmSteamKnightThrow *m, const int16_t root[3])
{
    if (!m || !root || m->phase != LIFTABLE) return 0;
    unsigned a = along(m), side = 2 - a;
    const Course *c = &courses[m->round];
    int32_t d[3];
    d[a] = sign(m) * c->aim_forward - root[a];
    d[side] = c->aim_side - root[side];
    d[1] = -2240 - root[1];
    if (sign(m) * d[a] <= 0 || d[1] >= 0) return 0;
    uint64_t length_sq = 0;
    for (unsigned i = 0; i < 3; ++i) length_sq += (uint64_t)((int64_t)d[i] * d[i]);
    uint32_t length = square_root(length_sq << 16);
    int16_t start[3] = {root[0], root[1], root[2]};
    place(m, start);
    for (unsigned i = 0; i < 3; ++i)
        m->velocity[i] = (int32_t)((int64_t)d[i] * (48 * ONE * 256) / length);
    m->released_age = 0;
    m->phase = AIM;
    return 1;
}

static unsigned aim(BfmSteamKnightThrow *m)
{
    unsigned side = 2 - along(m);
    int32_t cross = courses[m->round].aim_side * ONE;
    for (unsigned i = 0; i < 3; ++i) m->position[i] += m->velocity[i];
    if ((m->velocity[side] > 0 && m->position[side] >= cross) ||
        (m->velocity[side] < 0 && m->position[side] <= cross))
        m->position[side] = cross;
    if (m->position[1] > -2240 * ONE) return 0;
    m->position[side] = cross;
    publish(m);
    memcpy(m->apex, m->body, sizeof m->apex);
    m->velocity[1] = 0;
    m->velocity[along(m)] = sign(m) * 24 * ONE;
    m->phase = AIR;
    m->age = 0;
    return m->round == 2 ? BFM_STEAM_THROW_WEST_FALL : 0;
}

static unsigned fall(BfmSteamKnightThrow *m)
{
    unsigned a = along(m);
    ++m->age;
    if (m->round == 2 && m->phase == AIR && m->age == 45) {
        place(m, m->apex);
        m->velocity[1] = 0;
        m->age = 0;
        m->phase = REPLAY;
        return BFM_STEAM_THROW_WEST_REPLAY;
    }
    m->position[a] += m->velocity[a];
    m->velocity[1] += GRAVITY;
    m->position[1] += m->velocity[1];
    if (m->round == 2) {
        if (m->position[1] < 0) return 0;
        m->position[1] = 0;
        m->phase = LANDED;
        m->age = 0;
        return BFM_STEAM_THROW_WEST_IMPACT;
    }
    /* The first breach has a raised stone threshold. Its mounted upper
     * surface ends 32 units above the yard at Z=-1408. */
    m->contact_y = (m->round == 0 && m->position[2] <= -1408 * ONE)
                 ? m->floor_y - 32 * ONE : m->floor_y;
    if (m->position[1] < m->contact_y) return 0;
    m->position[1] = m->contact_y;
    m->velocity[1] = 0;
    m->phase = SLIDE;
    return 0;
}

static unsigned slide(BfmSteamKnightThrow *m)
{
    unsigned a = along(m);
    int32_t destination = courses[m->round].destination * ONE;
    m->velocity[a] -= ONE / 4;
    if (m->velocity[a] < 0) m->velocity[a] = 0;
    m->position[a] += m->velocity[a];
    if (m->round == 0 && m->contact_y < m->floor_y && m->position[a] > -1408 * ONE) {
        if (m->position[a] >= -1344 * ONE) {
            m->position[1] = m->floor_y;
            m->contact_y = m->floor_y;
        } else if (m->velocity[1] == 0) {
            m->velocity[1] = 2 * GRAVITY;
        } else {
            m->position[1] += m->velocity[1];
            m->velocity[1] += GRAVITY;
        }
    }
    if (m->position[a] < destination) return 0;
    m->position[a] = destination;
    m->position[1] = m->floor_y;
    m->phase = m->released_age < 40 ? ARRIVED : FINISHED;
    return BFM_STEAM_THROW_BODY_ARRIVED;
}

unsigned bfm_steam_throw_step(BfmSteamKnightThrow *m)
{
    if (!m) return 0;
    if (m->phase == ROLL) return roll(m);
    if (m->phase == UNUSED || m->phase == LIFTABLE || m->phase == FINISHED)
        return 0;
    ++m->released_age;
    unsigned events = 0;
    if (m->phase == AIM)
        events = aim(m);
    else if (m->phase == AIR || m->phase == REPLAY)
        events = fall(m);
    else if (m->phase == SLIDE)
        events = slide(m);
    else if (m->phase == LANDED && ++m->age == 28) {
        m->phase = FINISHED;
        events = BFM_STEAM_THROW_HERO_REPOSITION;
    }
    if (!m->crossed && sign(m) * m->position[along(m)] + ONE / 2 >= courses[m->round].wall * ONE) {
        events |= BFM_STEAM_THROW_GATE_CROSSED;
        m->crossed = 1;
    }
    if (m->round < 2 && m->released_age == 40) {
        events |= BFM_STEAM_THROW_HERO_REPOSITION;
        if (m->phase == ARRIVED) m->phase = FINISHED;
    }
    publish(m);
    return events;
}
