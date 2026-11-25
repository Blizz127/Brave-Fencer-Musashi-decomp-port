/* A refused clock and a mounted-word change both stop before retirement. */
#include "startup_mounted_fixture.h"

static int check_stop(MusashiBootMemory *memory, int change_word) {
    StartupMountedFixture fixture = {
        .memory=memory, .endpoint=BFM_GRAPHICS_ENTRY, .interrupt_at=BFM_GAME_ENTRY,
        .change_word=(unsigned)change_word
    };
    MusashiResetGraphPrefixStatus status = startup_mounted_run(&fixture);
    return status == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT && fixture.interrupted &&
        !fixture.reached && fixture.entry.pc == BFM_GAME_ENTRY &&
        fixture.stop.boundary.call_address == BFM_GAME_ENTRY &&
        fixture.stop.cpu.pc == fixture.entry.pc && fixture.stop.cpu.npc == fixture.entry.npc &&
        !memcmp(fixture.stop.cpu.r, fixture.entry.r, sizeof fixture.entry.r);
}

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    MusashiBootMemory *memory;
    int load = startup_mounted_load(argv[1], &memory);
    if (load) return load;
    MusashiBootMemory *initial = malloc(sizeof *initial);
    if (!initial) return 1;
    memcpy(initial, memory, sizeof *memory);
    int refused = check_stop(memory, 0);
    memcpy(memory, initial, sizeof *memory);
    free(initial);
    return refused && check_stop(memory, 1) ? 0 : 1;
}
