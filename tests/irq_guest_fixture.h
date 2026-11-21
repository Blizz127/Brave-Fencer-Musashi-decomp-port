#ifndef MUSASHI_TEST_IRQ_GUEST_FIXTURE_H
#define MUSASHI_TEST_IRQ_GUEST_FIXTURE_H

#include "musashi_guest_irq.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct FixtureIrqGuest {
    MusashiGuestIrqImage image;
    MusashiGuestIrqStop stop;
    uint64_t cycles;
    MusashiDmaController *dma;
    const MusashiExecutionClock *clock;
} FixtureIrqGuest;

static inline int fixture_irq_guest_map_file(FixtureIrqGuest *guest,
    MusashiBootMemory *ram, const char *path) {
    if (!path || !*path) return 0;
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    const size_t size = 0x65000u;
    uint8_t *exe = malloc(size);
    int ok = exe && fread(exe, 1, size, file) == size && fgetc(file) == EOF &&
        !ferror(file) && musashi_boot_map_irq_exe(&guest->image, ram, exe, size);
    fclose(file);
    free(exe);
    return ok;
}

static inline int fixture_irq_guest_clock(void *userdata, uint32_t cost) {
    FixtureIrqGuest *guest = userdata;
    guest->cycles += cost;
    return 1;
}

static inline int fixture_irq_guest_dispatch(void *userdata, MusashiBootMemory *ram,
    uint32_t context, const uint32_t installed[12], const void *interrupted,
    const MusashiIrqDispatchDevice *bus) {
    FixtureIrqGuest *guest = userdata;
    MusashiExecutionClock clock = {guest, fixture_irq_guest_clock};
    if (!interrupted) return 0;
    return musashi_boot_execute_guest_irq(&guest->image, ram, context, installed,
        bus, guest->clock ? guest->clock : &clock, guest->dma, &guest->stop);
}

/* State supplied only by tests that do not run the game's IRQ initialization. */
static inline void fixture_irq_vblank_setup(MusashiBootMemory *ram) {
    const uint32_t saved[12] = {0x80042780u, 0x8006cb14u, 0x801fffc0u, 0x8006bafcu,
                                0, 0, 0, 0, 0, 0, 0, 0x80074750u};
    for (unsigned i = 0; i < 12; ++i)
        assert(musashi_boot_write32(ram, 0x8006bb34u + 4u*i, saved[i]));
    assert(musashi_boot_write16(ram, 0x8006bafcu, 1u));
    assert(musashi_boot_write16(ram, 0x8006bb2cu, 1u));
    assert(musashi_boot_write32(ram, 0x8006bb00u, 0x80042ce8u));
    assert(musashi_boot_write32(ram, 0x8006cb88u, 0x1f801070u));
    assert(musashi_boot_write32(ram, 0x8006cb8cu, 0x1f801074u));
    for (unsigned i = 0; i < 8; ++i)
        assert(musashi_boot_write32(ram, 0x8006cb98u + 4u*i, 0));
}

/* Direct handler diagnostics have no scheduler or suspended outer CPU. This
 * bus supplies a single pending fixture edge and observes its acknowledgement
 * and guest terminal service. It cannot grant a production exception token. */
typedef struct FixtureVblankBus {
    uint16_t status;
    unsigned returns;
} FixtureVblankBus;

static inline int fixture_vblank_read(void *userdata, uint32_t address, uint16_t *value) {
    FixtureVblankBus *bus = userdata;
    if (address == 0x1f801070u) *value = bus->status;
    else if (address == 0x1f801074u) *value = 1;
    else return 0;
    return 1;
}

static inline int fixture_vblank_write(void *userdata, uint32_t address, uint16_t value) {
    FixtureVblankBus *bus = userdata;
    if (address != 0x1f801070u) return 0;
    bus->status &= value;
    return 1;
}

static inline int fixture_vblank_return(void *userdata) {
    FixtureVblankBus *bus = userdata;
    if (bus->status || bus->returns) return 0;
    ++bus->returns;
    return 1;
}

static inline int fixture_irq_direct_vblank(FixtureIrqGuest *guest, MusashiBootMemory *ram) {
    FixtureVblankBus pending = {1u, 0};
    MusashiIrqDispatchDevice bus = {.userdata=&pending, .read16=fixture_vblank_read,
        .write16=fixture_vblank_write, .return_from_exception=fixture_vblank_return};
    uint32_t saved[12];
    for (unsigned i = 0; i < 12; ++i)
        if (!musashi_boot_read32(ram, 0x8006bb34u + 4u*i, &saved[i])) return 0;
    return fixture_irq_guest_dispatch(guest, ram, 0x8006bb34u, saved, &pending, &bus) ==
        MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && guest->stop.pc == 0xb0u &&
        guest->stop.instructions && pending.returns == 1;
}

#endif
