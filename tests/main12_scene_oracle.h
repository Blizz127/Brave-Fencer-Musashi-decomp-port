/* A test-only reference for the mounted scene setup and its resident clears.
 * Decode the game's own words from private test RAM. The formatter exposes
 * ordinary loads at retirement; also check that the physical R3000 load
 * delay never changes an operand used by this path. */
enum {
    SCENE_TEST_CALL = 0x800d1d7cu, SCENE_TEST_RETURN = 0x800d1d84u,
    SCENE_TEST_SETUP = 0x801378f0u, SCENE_TEST_CHILD = 0x8013a860u,
    SCENE_TEST_CLEAR = 0x80016714u
};

typedef struct {
    uint32_t r[32], pc, npc, branch;
    unsigned cycles, instructions, reads, writes, loads_delayed, load_uses;
    unsigned pending_reg;
    uint32_t pending_old;
    unsigned width;
    uint32_t address, data;
    int store;
} SceneOracle;

static uint8_t *scene_oracle_bytes(MusashiBootMemory *ram, uint32_t address, unsigned width)
{
    const uint32_t pos = address - 0x80000000u;
    CHECK(pos < sizeof ram->bytes && width <= sizeof ram->bytes - pos &&
          (pos & (width - 1u)) == 0);
    return ram->bytes + pos;
}

static uint32_t scene_oracle_read(MusashiBootMemory *ram, uint32_t address, unsigned width)
{
    const uint8_t *bytes = scene_oracle_bytes(ram, address, width);
    uint32_t value = 0;
    for (unsigned i = 0; i < width; ++i) value |= (uint32_t)bytes[i] << (8u * i);
    return value;
}

static void scene_oracle_issue(SceneOracle *model, MusashiBootMemory *ram)
{
    const uint32_t pc = model->pc, immediate_pc = model->npc;
    CHECK((pc >= ROOM_RESET_CALLER && pc < ROOM_RESET_CALLER + 112u) ||
          (pc >= 0x8017bfe8u && pc <= 0x8017bff4u) ||
          (pc >= SCENE_TEST_SETUP && pc < SCENE_TEST_SETUP + 232u) ||
          (pc >= SCENE_TEST_CHILD && pc < SCENE_TEST_CHILD + 80u) ||
          (pc >= SCENE_TEST_CLEAR && pc < SCENE_TEST_CLEAR + 164u));
    const uint32_t word = scene_oracle_read(ram, pc, 4);
    const unsigned op = word >> 26, rs = (word >> 21) & 31u, rt = (word >> 16) & 31u;
    const unsigned rd = (word >> 11) & 31u, shift = (word >> 6) & 31u, function = word & 63u;
    const uint32_t immediate = word & 65535u;
    const uint32_t signed_immediate = (uint32_t)(int32_t)(int16_t)immediate;
    const uint32_t lhs = model->r[rs], rhs = model->r[rt];
    uint32_t next = immediate_pc + 4u;
    unsigned dest = 0;
    uint32_t result = 0;
    int uses_rs = 0, uses_rt = 0, branch = 0, load = 0;
    model->width = 0;
    model->store = 0;
    if (op == 0) {
        dest = rd;
        switch (function) {
        case 0: uses_rt = 1; result = rhs << shift; break;
        case 3: uses_rt = 1; result = (uint32_t)((int32_t)rhs >> shift); break;
        case 8: uses_rs = 1; branch = 1; next = lhs; break;
        case 33: uses_rs = uses_rt = 1; result = lhs + rhs; break;
        case 37: uses_rs = uses_rt = 1; result = lhs | rhs; break;
        default: CHECK(0);
        }
    } else if (op == 2u || op == 3u) {
        branch = 1;
        next = ((pc + 4u) & 0xf0000000u) | ((word & 0x03ffffffu) << 2);
        if (op == 3u) { dest = 31; result = pc + 8u; }
    } else if (op == 4u || op == 5u) {
        uses_rs = uses_rt = 1;
        branch = 1;
        if ((lhs == rhs) == (op == 4u)) next = pc + 4u + (signed_immediate << 2);
    } else if (op == 9u || op == 10u || op == 12u || op == 15u) {
        dest = rt;
        uses_rs = op != 15u;
        if (op == 9u) result = lhs + signed_immediate;
        else if (op == 10u) result = (int32_t)lhs < (int32_t)signed_immediate;
        else if (op == 12u) result = lhs & immediate;
        else result = immediate << 16;
    } else {
        CHECK(op == 35u || op == 37u || op == 40u || op == 41u || op == 43u);
        uses_rs = 1;
        model->address = lhs + signed_immediate;
        model->width = op == 35u || op == 43u ? 4u : op == 40u ? 1u : 2u;
        model->store = op >= 40u;
        if (model->store) {
            uint8_t *bytes = scene_oracle_bytes(ram, model->address, model->width);
            uses_rt = 1; model->data = rhs;
            for (unsigned i = 0; i < model->width; ++i) bytes[i] = (uint8_t)(rhs >> (i * 8u));
            ++model->writes;
        } else {
            dest = rt;
            result = scene_oracle_read(ram, model->address, model->width);
            model->data = result;
            load = 1;
            ++model->reads;
        }
    }
    if (model->pending_reg) {
        ++model->loads_delayed;
        if ((uses_rs && rs == model->pending_reg) || (uses_rt && rt == model->pending_reg)) {
            ++model->load_uses;
            CHECK(model->r[model->pending_reg] == model->pending_old);
        }
    }
    model->pending_reg = load ? dest : 0;
    if (load) model->pending_old = model->r[dest];
    if (dest) model->r[dest] = result;
    model->branch = branch ? pc : 0;
    model->pc = immediate_pc;
    model->npc = next;
    model->cycles += 2u + (model->width != 0);
    ++model->instructions;
}
