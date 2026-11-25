/* Exercise the guest CRT, game caller and constructor on an actual mounted image. */
#include "startup_mounted_fixture.h"

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    StartupMountedFixture fixture = {.endpoint=BFM_GRAPHICS_ENTRY};
    int load = startup_mounted_load(argv[1], &fixture.memory);
    if (load) return load;
    startup_mounted_run(&fixture);
    if (!fixture.reached || fixture.entry.pc != BFM_GAME_ENTRY ||
        fixture.graphics.pc != BFM_GRAPHICS_ENTRY || fixture.graphics.npc != BFM_GRAPHICS_ENTRY + 4u ||
        fixture.graphics.r[4] != 0 || fixture.graphics.r[31] != BFM_AFTER_GRAPHICS ||
        !fixture.entry.r[29] || fixture.graphics.r[29] >= fixture.entry.r[29] ||
        fixture.graphics_clock - fixture.entry_clock != 111u) return 1;
    return 0;
}
