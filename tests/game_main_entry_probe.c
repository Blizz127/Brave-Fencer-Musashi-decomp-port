/* Cold player with a real fixed guest window. Swapping the clock during a
 * stack store distinguishes a native side exit from a completed source store. */
#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAIN = 0x80010178u, CRT_DELAY = 0x80010098u, CONSTRUCTOR = 0x800100a0u };
enum Case { ORDINARY, CHANGE_CLOCK, CHANGE_CODE, CHANGE_EDGE, REGISTERED_DELAY };
typedef struct Probe {
    MusashiBootMemory *ram;
    MusashiExecutionClock clock;
    enum Case mode;
    unsigned elapsed, start, issues;
    uint32_t pc, original_delay;
} Probe;

static int next_charge(void *data, uint32_t cost) {
    ((Probe *)data)->elapsed += cost;
    return 1;
}

static int advance(void *data, uint32_t cost) {
    Probe *p = data;
    if (p->pc == MAIN+4u && p->mode != ORDINARY)
        p->clock.advance = next_charge;
    return next_charge(p,cost);
}

static int checkpoint(void *data, const void *continuation) {
    static const unsigned cycles[] = {0,2,5,8,11,14,16,18,20};
    Probe *p = data;
    MusashiCpuContext c;
    assert(musashi_boot_cpu_context(continuation,MUSASHI_CPU_CONTEXT_SOURCE,&c));
    p->pc = c.pc;
    uint32_t *regs = (uint32_t *)(uintptr_t)c.gpr;
    if (p->mode == CHANGE_EDGE && c.pc == CRT_DELAY) regs[31] = 0;
    if (c.pc == MAIN) {
        if (p->mode == CHANGE_EDGE) regs[31] = CRT_DELAY+4u;
        p->start = p->elapsed;
        regs[16] = 0x123abcd1u;
        regs[17] = 0x456e9872u;
        if (p->mode == CHANGE_CODE) {
            assert(musashi_boot_read32(p->ram,MAIN+28u,&p->original_delay));
            assert(musashi_boot_write32(p->ram,MAIN+28u,p->original_delay ^ 4u));
        }
    }
    if (c.pc == MAIN+4u && p->mode == CHANGE_CODE)
        assert(musashi_boot_write32(p->ram,MAIN+28u,p->original_delay));
    if ((uint32_t)(c.pc-MAIN) >= 32u && c.pc != CONSTRUCTOR) return 1;
    unsigned i = p->issues++;
    assert(i < 9u && c.pc == (i == 8u ? CONSTRUCTOR : MAIN+4u*i));
    assert(p->elapsed-p->start == cycles[i]);
    assert(c.branch_pc == (i == 7u ? MAIN+24u : 0u) && c.delay_slot == (i == 7u));
    assert(c.npc == (i == 7u ? CONSTRUCTOR : c.pc+4u));
    assert(regs[31] == (i >= 7u ? MAIN+32u : CRT_DELAY+4u));
    return i != 8u && !(i == 2u && p->mode != ORDINARY);
}

static int occupied_delay(MusashiNativeLaneCall *call) {
    (void)call;
    assert(0 && "the registered delay slot must use its existing source path");
    return 0;
}

static void quiet(void *data,const uint8_t *value,int32_t size) {
    (void)data; (void)value; (void)size;
}

int main(int argc,char **argv) {
    if (argc != 2) return 2;
    FILE *file = fopen(argv[1],"rb");
    if (!file) return 77;
    uint8_t *image = malloc(0x65000u);
    assert(image && fread(image,1,0x65000u,file) == 0x65000u && fgetc(file) == EOF);
    fclose(file);
    int alias = 0;
    MusashiBootMemory *ram = musashi_native_lane_map_guest(sizeof *ram,&alias);
    if (!ram) { free(image); return 77; }
    assert(musashi_native_lane_usable(ram));
    for (enum Case mode = ORDINARY; mode <= REGISTERED_DELAY; ++mode) {
        Probe p = {.ram=ram, .mode=mode};
        p.clock = (MusashiExecutionClock){&p,advance};
        MusashiCallbackDevice callback = {.userdata=&p, .checkpoint=checkpoint};
        MusashiStartupPrefixDevice devices = {.clock=&p.clock,.callback=&callback};
        MusashiEntryRunStop stop;
        uint32_t saved;
        memset(ram,0,sizeof *ram);
        assert(musashi_boot_map_exe(ram,image,0x65000u));
        if (mode == REGISTERED_DELAY) {
            static const MusashiNativeLaneEntry row = {MAIN+28u,0u,occupied_delay,"test",0u,1u,1u};
            assert(musashi_native_lane_install(&row,1u) == 1);
        }
        assert(musashi_boot_run_entry_with_devices(ram,0,&devices,quiet,NULL,NULL,NULL,&stop)
            == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
        assert(musashi_boot_read32(ram,0x801ffff4u,&saved));
        if (mode == ORDINARY) {
            assert(stop.cpu.pc == CONSTRUCTOR && p.issues == 9u && saved == CRT_DELAY+4u);
            assert(musashi_boot_read32(ram,0x801fffecu,&saved) && saved == 0x456e9872u);
            assert(musashi_boot_read32(ram,0x801fffe8u,&saved) && saved == 0x123abcd1u);
        } else if (mode == CHANGE_CLOCK) {
            assert(stop.cpu.pc == MAIN+4u && p.issues == 2u && p.elapsed-p.start == 5u && saved == 0u);
        } else {
            assert(stop.cpu.pc == MAIN+8u && p.issues == 3u && saved == CRT_DELAY+4u);
        }
    }
    free(image);
    puts("Game main: eight cold caller issues, stack saves, provider and return-edge controls passed.");
    return 0;
}
