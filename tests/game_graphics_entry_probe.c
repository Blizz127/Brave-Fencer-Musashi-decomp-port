#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CTOR=0x800100a0u, EXCHANGE=0x80043060u, CALLER=0x800101b4u,
       GRAPHICS=0x800141f0u, LIBRARY=0x80059234u, SETTING=0x80074778u,
       SAVED_RA=0x801fffb8u };
enum Mode { NORMAL, PAID_GLOBAL, PAID_STACK, CHANGED_RETURN, CHANGED_SOURCE, OCCUPIED };
typedef struct {
    MusashiBootMemory *memory;
    MusashiExecutionClock clock;
    enum Mode mode;
    uint32_t pc, original;
    unsigned preset, elapsed, start, caller_issues, wrapper_issues;
} Probe;

static uint32_t read_word(Probe *p,uint32_t address)
{
    uint32_t word;
    assert(musashi_boot_read32(p->memory,address,&word));
    return word;
}

static int count_cycles(void *user,uint32_t cycles)
{
    ((Probe *)user)->elapsed += cycles;
    return 1;
}

static int advance(void *user,uint32_t cycles)
{
    Probe *p = user;
    if ((p->mode == PAID_STACK && p->pc == GRAPHICS+4u) ||
        (p->mode != PAID_STACK && p->mode != NORMAL && p->pc == CALLER+8u))
        p->clock.advance = count_cycles;
    return count_cycles(p,cycles);
}

static int checkpoint(void *user,const void *state)
{
    Probe *p = user;
    MusashiCpuContext c;
    assert(musashi_boot_cpu_context(state,MUSASHI_CPU_CONTEXT_SOURCE,&c));
    p->pc = c.pc;
    uint32_t *r = (uint32_t *)(uintptr_t)c.gpr;
    if (c.pc == CTOR && p->preset)
        assert(musashi_boot_write32(p->memory,0x80062998u,1u));
    if (c.pc == EXCHANGE) {
        assert(musashi_boot_write32(p->memory,0x8006cbe8u,0x10203040u));
        assert(musashi_boot_write32(p->memory,SETTING,0x12345678u));
    }
    if (c.pc == EXCHANGE+20u && p->mode == CHANGED_RETURN) r[31] = 0;
    if (c.pc == CALLER) {
        if (p->mode == CHANGED_RETURN) r[31] = CALLER;
        assert(r[2] == 0x10203040u && !read_word(p,0x8006cbe8u));
        p->start = p->elapsed;
        if (p->mode == CHANGED_SOURCE) {
            p->original = read_word(p,CALLER+16u);
            assert(musashi_boot_write32(p->memory,CALLER+16u,p->original ^ 4u));
        }
    }
    if ((uint32_t)(c.pc-CALLER) < 20u) {
        static const unsigned cycle[] = {0,2,4,7,9};
        unsigned i = p->caller_issues++;
        assert(i < 5u && c.pc == CALLER+4u*i && p->elapsed == p->start+cycle[i]);
        assert(c.npc == (i == 4u ? GRAPHICS : c.pc+4u));
        assert(c.delay_slot == (i == 4u) && c.branch_pc == (i == 4u ? CALLER+12u : 0u));
        if (i == 1u) {
            assert(r[2] == 992u);
            r[2] = 0xa123bc45u;
            if (p->mode == CHANGED_SOURCE)
                assert(musashi_boot_write32(p->memory,CALLER+16u,p->original));
        }
        if (i == 3u && p->mode != NORMAL && p->mode != PAID_STACK) return 0;
    }
    if ((uint32_t)(c.pc-GRAPHICS) < 16u) {
        static const unsigned cycle[] = {11,13,16,18};
        unsigned i = p->wrapper_issues++;
        assert(i < 4u && c.pc == GRAPHICS+4u*i && p->elapsed == p->start+cycle[i]);
        assert(r[29] == (i == 0 ? 0x801fffc0u : 0x801fffa8u));
        assert(r[31] == (i == 3u ? GRAPHICS+16u : CALLER+20u));
        assert(c.npc == (i == 3u ? LIBRARY : c.pc+4u));
        if (!i) assert(musashi_boot_write32(p->memory,SAVED_RA,0x21345067u));
        if (i == 3u) r[4] = 0x321u;
    }
    if (c.pc != LIBRARY) return 1;
    assert(c.npc == LIBRARY+4u && !c.delay_slot && !c.branch_pc && !r[4]);
    assert(p->mode == NORMAL && r[31] == GRAPHICS+16u && p->elapsed == p->start+20u);
    return 0;
}

static int occupied(MusashiNativeLaneCall *call)
{
    (void)call;
    assert(0 && "the probe ends before the occupied graphics entry");
    return 0;
}

static void quiet(void *user,const uint8_t *bytes,int32_t length)
{
    (void)user; (void)bytes; (void)length;
}

int main(int argc,char **argv)
{
    if (argc != 2) return 2;
    FILE *file = fopen(argv[1],"rb");
    if (!file) return 77;
    uint8_t *image = malloc(0x65000u);
    assert(image && fread(image,1,0x65000u,file) == 0x65000u && fgetc(file) == EOF);
    fclose(file);
    int aliased = 0;
    MusashiBootMemory *memory = musashi_native_lane_map_guest(sizeof *memory,&aliased);
    if (!memory) { free(image); return 77; }
    assert(musashi_native_lane_usable(memory));
    for (enum Mode mode=NORMAL; mode<=OCCUPIED; ++mode) {
        if (mode == OCCUPIED) {
            static const MusashiNativeLaneEntry row = {
                .guest_pc=GRAPHICS,.fn=occupied,.name="test",.returns=1
            };
            assert(musashi_native_lane_install(&row,1u) == 1);
        }
        for (unsigned preset=0; preset<2u; ++preset) {
            Probe p = {.memory=memory,.mode=mode,.preset=preset};
            p.clock = (MusashiExecutionClock){&p,advance};
            MusashiCallbackDevice callback = {.userdata=&p,.checkpoint=checkpoint};
            MusashiStartupPrefixDevice devices = {.callback=&callback,.clock=&p.clock};
            MusashiEntryRunStop stop;
            memset(memory,0,sizeof *memory);
            assert(musashi_boot_map_exe(memory,image,0x65000u));
            assert(musashi_boot_run_entry_with_devices(memory,0,&devices,quiet,NULL,NULL,NULL,&stop)
                == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
            if (mode == NORMAL) {
                assert(stop.cpu.pc == LIBRARY && p.caller_issues == 5u && p.wrapper_issues == 4u);
                assert(read_word(&p,SAVED_RA) == CALLER+20u);
            } else if (mode == PAID_STACK) {
                assert(stop.cpu.pc == GRAPHICS+4u && p.elapsed == p.start+16u);
                assert(read_word(&p,SAVED_RA) == 0x21345067u);
            } else if (mode == PAID_GLOBAL) {
                assert(stop.cpu.pc == CALLER+8u && p.elapsed == p.start+7u);
            } else {
                assert(stop.cpu.pc == CALLER+12u && p.elapsed == p.start+7u);
            }
            assert(read_word(&p,SETTING) == (mode == PAID_GLOBAL ? 0x12345678u : 0xa123bc45u));
        }
    }
    free(image);
    puts("Startup graphics caller, physical stack save, exchange returns and clock ownership passed.");
    return 0;
}
