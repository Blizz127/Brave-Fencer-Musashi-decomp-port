/* Compare six compiled script-variable helpers and their generated direct calls
 * with test-authored load/store programs in synthetic guest RAM. */
#include "musashi_native_lane.h"
#include "musashi_bios_exception.h"
#include <stdio.h>
#include <string.h>

extern const MusashiNativeLaneEntry musashi_native_lane_generated[];
extern const size_t musashi_native_lane_generated_count;

enum {
    VARIABLES = 0x800ba1b8u, FIRST_CALLER = 0x801fe400u, MARKERS = 0xb9000u,
    STACK = 0x801fe000u, BUS_MARK = 37u, CHECK_BYTES = 12
};
static const uint32_t store_value = 0xd45bc781u;
static const struct Accessor {
    uint32_t pc;
    unsigned opcode, width;
    int writes;
} accessors[] = {
    {0x800291a0u, 40, 1, 1}, {0x800291b4u, 36, 1, 0}, /* SB, LBU */
    {0x800291c8u, 41, 2, 1}, {0x800291dcu, 33, 2, 0}, /* SH, LH  */
    {0x800291f0u, 43, 4, 1}, {0x80029204u, 35, 4, 0}  /* SW, LW  */
};

static int install_fixture(MusashiBootMemory *memory, const struct Accessor *helper) {
    /* The fixture computes the effective address in t2, stamps v1 before the
     * access, then returns normally. a3 carries the synthetic variable base. */
    const uint32_t words[] = {
        (4u << 21) | (7u << 16) | (10u << 11) | 33u,                  /* addu t2,a0,a3 */
        (13u << 26) | (3u << 16) | BUS_MARK,                          /* ori  v1,zero,mark */
        (helper->opcode << 26) | (10u << 21) | ((helper->writes ? 5u : 2u) << 16),
        (31u << 21) | 8u, 0u                                           /* jr ra; nop */
    };
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
        if (!musashi_boot_write32(memory, helper->pc + (uint32_t)i * 4u, words[i])) return 0;
    return 1;
}

struct Site {
    uint32_t address;
    uint8_t *backing;
    unsigned offset, backing_size;
    int mapped, direct;
};

enum Path { INTERPRETED, STANDALONE, NESTED };

static int check(MusashiBootMemory *memory, unsigned i, const struct Site *site, enum Path path) {
    const struct Accessor *helper = &accessors[i];
    const unsigned width = helper->width;
    const int allowed = site->mapped && (site->address & (width - 1u)) == 0;
    const int bus = path == INTERPRETED || !site->direct || !allowed;
    const uint32_t entry = path == NESTED ? FIRST_CALLER + 0x20u * i : helper->pc;
    const uint32_t args[] = {site->address - VARIABLES, store_value, 0, VARIABLES};
    uint8_t *markers = memory->bytes + MARKERS;
    uint8_t expected[CHECK_BYTES];
    unsigned start = site->offset < 4u ? 0u : site->offset - 4u;
    if (start > site->backing_size - CHECK_BYTES) start = site->backing_size - CHECK_BYTES;
    unsigned index = site->offset - start;
    /* High bits in every seed byte exercise the distinct byte and halfword return types. */
    for (unsigned j = 0; j < CHECK_BYTES; ++j) expected[j] = (uint8_t)(0x81u + j * 7u);
    memcpy(site->backing + start, expected, CHECK_BYTES);
    memset(markers, 0, 2);
    uint32_t expected_v0 = 0;
    if (allowed) {
        for (unsigned j = 0; j < width; ++j) {
            if (helper->writes) expected[index + j] = (uint8_t)(store_value >> (8u * j));
            else expected_v0 |= (uint32_t)expected[index + j] << (8u * j);
        }
        if (!helper->writes && width == 2 && (expected_v0 & 0x8000u))
            expected_v0 |= 0xffff0000u;
    }
    uint32_t v0 = UINT32_MAX, v1 = UINT32_MAX;
    int ok = musashi_native_lane_reference_call(memory, entry, args, 4, STACK,
                                                 path != INTERPRETED, &v0, &v1);
    if (ok != allowed || (ok && v0 != expected_v0) || v1 != (bus ? BUS_MARK : 0u) ||
        markers[0] != (path == NESTED) || markers[1] != (path == NESTED && ok) ||
        memcmp(site->backing + start, expected, CHECK_BYTES) != 0) {
        fprintf(stderr, "helper=%08x entry=%08x address=%08x path=%u ok=%d v0=%08x v1=%08x "
                "expected_ok=%d expected_v0=%08x bus=%d caller_markers=%u,%u\n",
                helper->pc, entry, site->address, (unsigned)path, ok, v0, v1,
                allowed, expected_v0, bus, (unsigned)markers[0], (unsigned)markers[1]);
        return 0;
    }
    return 1;
}

/* The evolving owner grants the second bus query for this word. The first
 * query belongs to RAM resolution; the second selects the device write. */
static unsigned ownership_queries, owned_writes;
static int changing_owner(uint32_t address, size_t width) {
    return address == VARIABLES && width == 4 && ++ownership_queries == 2;
}

static int owned_write(void *user, uint32_t address, uint32_t value) {
    uint32_t word;
    if (address != VARIABLES || value != 0x455au || ownership_queries != 2 ||
        !musashi_boot_read32(user, address, &word) || word != 0x81421122u) return 0;
    ++owned_writes;
    return 1;
}

static void discard_text(void *user, const uint8_t *data, int32_t size) {
    (void)user; (void)data; (void)size;
}

static int check_image_owners(MusashiBootMemory *memory) {
    /* Enter inside a guest-fetch range and outside its native entry wrapper. */
    enum { BOOT = 0x800291ccu, STORE_WORD = 0x800291f0u };
    /* Set a3 to VARIABLES and tail-call the word setter with a1 = 0x455a.
     * The synthetic guest setter also marks VARIABLES+4 on its return. */
    const uint32_t boot[] = {
        (15u << 26) | (7u << 16) | (VARIABLES >> 16),              /* lui a3,hi */
        (13u << 26) | (7u << 21) | (7u << 16) | (VARIABLES & 0xffffu),
        (2u << 26) | ((STORE_WORD >> 2) & 0x03ffffffu),            /* j setter */
        (13u << 26) | (5u << 16) | 0x455au                         /* ori a1,zero,value */
    };
    const uint32_t setter[] = {
        (43u << 26) | (7u << 21) | (5u << 16),                    /* sw a1,0(a3) */
        (31u << 21) | 8u,                                          /* jr ra */
        (40u << 26) | (7u << 21) | (5u << 16) | 4u                /* sb a1,4(a3) */
    };
    for (size_t i = 0; i < sizeof(boot) / sizeof(*boot); ++i)
        if (!musashi_boot_write32(memory, BOOT + 4u * i, boot[i])) return 0;
    for (size_t i = 0; i < sizeof(setter) / sizeof(*setter); ++i)
        if (!musashi_boot_write32(memory, STORE_WORD + 4u * i, setter[i])) return 0;

    const MusashiCallbackDevice callbacks = {0};
    MusashiBiosImageDevice owners[] = {
        {.intersects = musashi_bios_exception_intersects},
        {.userdata = memory, .intersects = changing_owner, .write32 = owned_write}
    };
    int passed = 1;
    for (unsigned i = 0; i < 2; ++i) {
        const int fixed = owners[i].intersects == musashi_bios_exception_intersects;
        const MusashiStartupPrefixDevice devices = {.callback = &callbacks, .bios_image = &owners[i]};
        MusashiResetGraphPrefixStop stop = {0};
        uint32_t word = 0;
        uint8_t *marker = memory->bytes + (VARIABLES - MUSASHI_NATIVE_LANE_GUEST_BASE) + 4;
        *marker = 0;
        ownership_queries = owned_writes = 0;
        if (!musashi_boot_write32(memory, VARIABLES, 0x81421122u)) return 0;
        MusashiResetGraphPrefixStatus status = musashi_boot_execute_source_function(
            memory, &devices, BOOT, discard_text, NULL, &stop);
        if (!musashi_boot_read32(memory, VARIABLES, &word) ||
            status != MUSASHI_RESETGRAPH_PREFIX_COMPLETE ||
            word != (fixed ? 0x455au : 0x81421122u) || *marker != (fixed ? 0 : 0x5a) ||
            ownership_queries != (fixed ? 0u : 2u) || owned_writes != (fixed ? 0u : 1u)) {
            fprintf(stderr, "script word owner fixed=%d status=%d stopped=%08x queries=%u "
                    "writes=%u ram=%08x marker=%u\n", fixed, (int)status, stop.call_address,
                    ownership_queries, owned_writes, word, *marker);
            passed = 0;
        }
    }
    return passed;
}

int main(void) {
    const size_t count = sizeof(accessors) / sizeof(accessors[0]);
    MusashiBootMemory *memory = musashi_native_lane_map_guest(sizeof(*memory), NULL);
    if (!memory) return 77;
    if (!musashi_native_lane_usable(memory) || musashi_native_lane_generated_count != 2 * count ||
        musashi_native_lane_install(musashi_native_lane_generated, 2 * count) != (int)(2 * count))
        return 1;

    const uint32_t base = MUSASHI_NATIVE_LANE_GUEST_BASE;
    const unsigned vars = VARIABLES - base;
    uint8_t *ram = memory->bytes, *pad = memory->scratchpad;
    unsigned attempted = 0;
    int passed = 1;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned w = accessors[i].width;
        if (!install_fixture(memory, &accessors[i])) return 1;
        const struct Site sites[] = {
            {VARIABLES - 4u, ram, vars - 4u, MUSASHI_RAM_SIZE, 1, 1},
            {VARIABLES + 1u, ram, vars + 1u, MUSASHI_RAM_SIZE, 1, 1},
            {VARIABLES + 2u, ram, vars + 2u, MUSASHI_RAM_SIZE, 1, 1},
            {base + MUSASHI_RAM_SIZE - w, ram, MUSASHI_RAM_SIZE - w, MUSASHI_RAM_SIZE, 1, 1},
            {base + MUSASHI_RAM_SIZE - 1u, ram, MUSASHI_RAM_SIZE - 1u, MUSASHI_RAM_SIZE, 1, 1},
            {base + MUSASHI_RAM_SIZE + 16u, ram, 16, MUSASHI_RAM_SIZE, 1, 0},
            {0x00001204u, ram, 0x1204, MUSASHI_RAM_SIZE, 1, 0},
            {0xa0602010u, ram, 0x2010, MUSASHI_RAM_SIZE, 1, 0},
            {0x1f800000u + MUSASHI_SCRATCHPAD_SIZE - w, pad,
             MUSASHI_SCRATCHPAD_SIZE - w, MUSASHI_SCRATCHPAD_SIZE, 1, 0},
            {0x9f800011u, pad, 17, MUSASHI_SCRATCHPAD_SIZE, 1, 0},
            /* The watched RAM location also catches unintended wrapping. */
            {0x80800000u, ram, 0, MUSASHI_RAM_SIZE, 0, 0},
            {0xc0000010u, ram, 16, MUSASHI_RAM_SIZE, 0, 0}
        };
        for (enum Path path = INTERPRETED; path <= NESTED; ++path) {
            for (size_t j = 0; j < sizeof(sites) / sizeof(sites[0]); ++j) {
                ++attempted;
                if (!check(memory, i, &sites[j], path)) {
                    passed = 0;
                    break;
                }
            }
        }
    }
    if (!check_image_owners(memory)) passed = 0;
    if (passed) printf("Verified %u script-variable accesses and both BIOS ownership contracts.\n", attempted);
    return passed ? 0 : 1;
}
