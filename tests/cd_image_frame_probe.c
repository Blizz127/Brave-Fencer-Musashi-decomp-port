/* Exercise the host admission point using synthetic CPU frames and RAM.
 * A successful admission never performs a GPU write in this test. */
#include <assert.h>
#include "../pc_port/mips_formatter.c"

static int unexpected_write(void *context, uint32_t address, uint32_t value) {
    (void)context;
    (void)address;
    (void)value;
    abort();
}

static int permitted_form(MusashiBootMemory *ram, FormatterCpu *cpu, CdIrqReturn *frames,
                          uint32_t saved_sp, uint32_t call, unsigned depth, int direct) {
    MusashiCdIrqServices services = {0};
    uint32_t encoding = direct ?
        ((3u << 26) | ((cpu->pc >> 2) & 0x03ffffffu)) :
        ((2u << 21) | (31u << 11) | 9u);
    return cd_irq_call(ram, cpu, &services, frames, depth, saved_sp, call, encoding);
}

static int permitted(MusashiBootMemory *ram, FormatterCpu *cpu, CdIrqReturn *frames,
                     uint32_t saved_sp, uint32_t call, unsigned depth) {
    return permitted_form(ram, cpu, frames, saved_sp, call, depth, call == 0x800599d8u);
}

int main(void) {
    static MusashiBootMemory ram;
    const uint32_t stack = 0x80190880u;
    const uint32_t rectangle = 0x80031040u, pixels = 0x80040820u;
    CdIrqReturn frames[3] = {
        {.target = 0x8001a338u, .pc = 0x80045308u, .sp = stack - 0x50u},
        {.target = 0x800599b8u, .pc = 0x8001a49cu, .sp = stack - 0x88u,
         .a0 = rectangle, .a1 = pixels}
    };
    MusashiGpuIoDevice writer = {.write32 = unexpected_write};
    FormatterCpu cpu = {0};
    assert(musashi_boot_write32(&ram, 0x8006cc88u, frames[0].target));
    assert(musashi_boot_write32(&ram, 0x80072780u, 0x80072740u));
    assert(musashi_boot_write32(&ram, 0x80072748u, 0x8005b7b0u));
    assert(musashi_boot_write32(&ram, 0x80072760u, 0x8005b1c4u));
    cpu.r[5] = cpu.r[16] = rectangle;
    cpu.r[17] = pixels;
    cpu.r[29] = stack - 0xa8u;
    for (unsigned i = 0; i < 2; ++i) {
        const uint32_t call = i ? 0x800599fcu : 0x800599d8u;
        cpu.pc = i ? 0x8005b7b0u : 0x80059760u;
        cpu.r[4] = i ? 0x8005b1c4u : 0x80074184u;
        cpu.r[6] = i ? 8u : 101u;
        cpu.r[7] = i ? pixels : 373u;
        cpu.r[31] = call + 8u;
        cpu.gpu_io_device = &writer;
        assert(permitted(&ram, &cpu, frames, stack, call, 2));
        assert(!permitted_form(&ram, &cpu, frames, stack, call, 2, i != 0));
        assert(!permitted(&ram, &cpu, frames, stack, call, 1));
        ++cpu.r[31];
        assert(!permitted(&ram, &cpu, frames, stack, call, 2));
        --cpu.r[31];
        ++cpu.r[16];
        assert(!permitted(&ram, &cpu, frames, stack, call, 2));
        --cpu.r[16];
        assert(musashi_boot_write32(&ram, 0x8006cc88u, 0));
        assert(!permitted(&ram, &cpu, frames, stack, call, 2));
        assert(musashi_boot_write32(&ram, 0x8006cc88u, frames[0].target));
        cpu.gpu_io_device = NULL;
        assert(permitted(&ram, &cpu, frames, stack, call, 2) == !i);
    }
    cpu.gpu_io_device = &writer;
    assert(musashi_boot_write32(&ram, 0x80072760u, 0));
    assert(!permitted(&ram, &cpu, frames, stack, 0x800599fcu, 2));
    /* The existing long LIST path remains independent of the new short one. */
    frames[1].pc = 0x8001a728u;
    assert(permitted(&ram, &cpu, frames, stack, 0x800599fcu, 2));
    return 0;
}
