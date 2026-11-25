/* Observe a cold call to the graphics initializer, then decline its first clock. */
#include "startup_mounted_fixture.h"

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    StartupMountedFixture f = {.interrupt_at = BFM_GRAPHICS_ENTRY};
    int result = startup_mounted_load(argv[1], &f.memory);
    if (result) return result;

    MusashiResetGraphPrefixStatus status = startup_mounted_run(&f);
    const MusashiEntryCpuSnapshot *graphics = &f.graphics;
    if (status != MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT ||
        f.entry.pc != BFM_GAME_ENTRY || graphics->pc != BFM_GRAPHICS_ENTRY ||
        graphics->npc != graphics->pc + 4u || graphics->r[4] != 0 ||
        graphics->r[31] != BFM_AFTER_GRAPHICS || graphics->r[29] >= f.entry.r[29] ||
        graphics->r[29] != graphics->r[30] || graphics->r[28] != f.entry.r[28] ||
        !f.interrupted || f.returned.pc || f.cd.pc) return 1;
    return f.stop.cpu.pc != graphics->pc || f.stop.cpu.npc != graphics->npc ||
           f.stop.cpu.hi != graphics->hi || f.stop.cpu.lo != graphics->lo ||
           memcmp(f.stop.cpu.r, graphics->r, sizeof graphics->r) != 0;
}
