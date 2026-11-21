/* The normal source runner fetches instructions from the player's authenticated
 * executable. The test owns only devices and synthetic initial state. */
#include "musashi_boot_memory.h"
#include "musashi_dma_controller.h"
#include "musashi_gpu_controller.h"
#include "musashi_irq_controller.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct ResetWrite {
    uint32_t address, value;
    unsigned width;
} ResetWrite;

typedef struct ResetProbe {
    MusashiGpuController gpu;
    MusashiDmaController dma;
    MusashiIrqController irq;
    ResetWrite writes[7];
    unsigned attempts, accepted, deny, steps, cycles, count;
    uint32_t last_pc, last_v0, denied_pc;
    int query_info, query_status, query_return;
} ResetProbe;

static int accept(void *user) { return user != NULL; }
static int draw_mode(void *user, uint32_t word) { (void)word; return accept(user); }
static int display_enable(void *user, int enabled) { (void)enabled; return accept(user); }

static int allow_access(ResetProbe *probe) {
    if (++probe->attempts == probe->deny) {
        probe->denied_pc = probe->last_pc;
        return 0;
    }
    return 1;
}

static int log_write(ResetProbe *probe, uint32_t address, uint32_t value,
                     unsigned width) {
    ResetWrite *row;
    if (probe->count >= sizeof(probe->writes) / sizeof(probe->writes[0])) return 0;
    row = &probe->writes[probe->count++];
    row->address = address;
    row->value = value;
    row->width = width;
    ++probe->accepted;
    return 1;
}

static int read_word(void *user, uint32_t a, uint32_t *v) {
    ResetProbe *p = user;
    int ok;
    if (!allow_access(p)) return 0;
    ok = a == 0x1f801810u || a == 0x1f801814u
        ? musashi_gpu_controller_read32(&p->gpu, a, v)
        : musashi_dma_controller_read32(&p->dma, a, v);
    if (ok) ++p->accepted;
    return ok;
}

static int write_word(void *user, uint32_t a, uint32_t v) {
    ResetProbe *p = user;
    int ok;
    if (!allow_access(p)) return 0;
    ok = a == 0x1f801810u || a == 0x1f801814u
        ? musashi_gpu_controller_write32(&p->gpu, a, v)
        : musashi_dma_controller_write32(&p->dma, a, v);
    return ok && log_write(p, a, v, 4);
}

static int read_half(void *user, uint32_t a, uint16_t *v) {
    ResetProbe *p = user;
    if (!allow_access(p) || !musashi_irq_controller_read16(&p->irq, a, v)) return 0;
    ++p->accepted;
    return 1;
}

static int write_half(void *user, uint32_t a, uint16_t v) {
    ResetProbe *p = user;
    return allow_access(p) && musashi_irq_controller_write16(&p->irq, a, v) &&
        log_write(p, a, v, 2);
}

static int tick(void *user, unsigned cost) {
    ResetProbe *p = user;
    p->cycles += cost;
    return 1;
}

static int observe(void *user, const void *continuation) {
    ResetProbe *p = user;
    MusashiCpuContext context;
    if (++p->steps > 30000u ||
        !musashi_boot_cpu_context(continuation, MUSASHI_CPU_CONTEXT_SOURCE, &context)) return 0;
    p->last_pc = context.pc;
    p->last_v0 = context.gpr[2];
    if (context.pc == 0x8005c1f0u)
        p->query_info = context.gpr[2] == 0x400u && context.gpr[3] == 2;
    if (context.pc == 0x8005c234u)
        p->query_status = context.gpr[2] == 0x14802000u;
    if (context.pc == 0x8005c294u)
        p->query_return = context.gpr[2] == 0;
    return 1;
}

static void no_console(void *user, const uint8_t *data, int32_t size) {
    ResetProbe *p = user;
    (void)data;
    (void)size;
    p->steps = 30001u;
}

static const uint32_t buffers[] = {0x80078874u, 0x800c5510u};
static const size_t lengths[] = {256, 6144};

static int execute(MusashiBootMemory *ram, const unsigned char *exe, size_t size,
                   int query, unsigned cut, ResetProbe *p) {
    MusashiEntryState entry;
    MusashiGpuBackend backend = {.userdata=p, .reset=accept, .draw_mode=draw_mode,
        .display_enable=display_enable, .clear_fifo=accept, .ready=accept};
    MusashiGpuDevice legacy = {0};
    MusashiCallbackDevice callback = {.userdata=p, .reset_device=&legacy, .checkpoint=observe};
    MusashiInputDevice input = {.userdata=p, .read16=read_half, .write16=write_half};
    MusashiGpuIoDevice gpu = {.userdata=p, .read32=read_word, .write32=write_word};
    MusashiCdRegisterDevice dma = {.userdata=p, .read32=read_word, .write32=write_word};
    MusashiExecutionClock clock = {.userdata=p, .advance=tick};
    MusashiStartupPrefixDevice devices = {.callback=&callback, .input=&input,
        .cd_registers=&dma, .gpu_io=&gpu, .clock=&clock};
    MusashiResetGraphPrefixStop stop = {0};
    MusashiResetGraphPrefixStatus status;
    unsigned i;
    memset(p, 0, sizeof *p);
    p->deny = cut;
    if (!musashi_boot_load_entry(ram, exe, size, 0, &entry)) return 0;
    for (i = 0; i < 2; ++i) {
        uint8_t *area = musashi_boot_ram_span(ram, buffers[i] - 1u, lengths[i] + 2u);
        if (!area) return 0;
        memset(area, 0xa7, lengths[i] + 2u);
    }
    if (!musashi_gpu_controller_init(&p->gpu, &backend)) return 0;
    musashi_irq_controller_init(&p->irq);
    musashi_dma_controller_init(&p->dma, &p->irq);
    if (!musashi_irq_controller_write16(&p->irq, 0x1f801074u, 9) ||
        !musashi_dma_controller_write32(&p->dma, 0x1f8010f0u, 0x33333333u)) return 0;

    status = musashi_boot_execute_source_function(ram, &devices,
        query ? 0x8005c1c0u : 0x8005bd7cu, no_console, p, &stop);

    if (cut)
        return status == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT && p->attempts == cut &&
            p->accepted == cut - 1 && p->denied_pc && stop.call_address == p->denied_pc &&
            stop.target_address == p->denied_pc;
    return status == MUSASHI_RESETGRAPH_PREFIX_COMPLETE && stop.target_address == UINT32_MAX &&
        p->query_info && p->query_status && p->query_return && !p->last_v0;
}

int musashi_test_owned_reset(const unsigned char *exe, size_t size, int query_only) {
    /* Writes observed on the mode-zero PCSX-Redux cold boot, beginning after
     * the earlier BIOS submission. The final two are the standalone query. */
    static const ResetWrite expected[] = {
        {0x1f801074u, 0, 2}, {0x1f8010a8u, 0x401u, 4},
        {0x1f8010f0u, 0x33333b33u, 4}, {0x1f801814u, 0, 4},
        {0x1f801074u, 9u, 2}, {0x1f801814u, 0x10000007u, 4},
        {0x1f801810u, 0xe1003000u, 4}
    };
    MusashiBootMemory *ram = malloc(sizeof *ram);
    ResetProbe full, partial;
    unsigned i, j, attempts = query_only ? 6u : 14u;
    unsigned start = query_only ? 5u : 0u;
    int ok = 0;
    if (!ram || !execute(ram, exe, size, query_only, 0, &full)) goto done;
    if (full.attempts != attempts || full.accepted != attempts ||
        full.count != 7u - start || full.irq.mask != 9 ||
        full.cycles != (query_only ? 83u : 57883u)) goto done;
    for (i = 0; i < full.count; ++i)
        if (full.writes[i].address != expected[start+i].address ||
            full.writes[i].width != expected[start+i].width ||
            full.writes[i].value != expected[start+i].value) goto done;
    for (i = 0; i < 2; ++i) {
        const uint8_t *p = musashi_boot_ram_span(ram, buffers[i] - 1u, lengths[i] + 2u);
        if (!p || p[0] != 0xa7 || p[lengths[i]+1] != 0xa7) goto done;
        for (j = 1; j <= lengths[i]; ++j)
            if (p[j] != (query_only ? 0xa7 : 0)) goto done;
    }
    for (i = 1; i <= attempts; ++i) {
        if (!execute(ram, exe, size, query_only, i, &partial)) goto done;
        for (j = 0; j < partial.count; ++j)
            if (partial.writes[j].address != full.writes[j].address ||
                partial.writes[j].width != full.writes[j].width ||
                partial.writes[j].value != full.writes[j].value) goto done;
    }
    ok = 1;
done:
    free(ram);
    return ok;
}
