/* Follow the original constructor and change one owner at a time before the
 * first sound-library call. The normal data-free suite skips this media test. */
#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CTOR=0x800100a0u, CTOR_RETURN=0x80010108u, MAIN_RESUME=0x80010198u,
       EXCHANGE=0x80043060u };
enum Mode { NORMAL, PAID_NATIVE, CHANGED_CTOR, CHANGED_EDGE, CHANGED_GAME, OCCUPIED };
typedef struct {
    MusashiBootMemory *memory;
    MusashiExecutionClock clock;
    enum Mode mode;
    uint32_t pc, original_word;
    unsigned preset, elapsed, start, ctor_issues, game_issues;
} Probe;

static int count_cycles(void *user,uint32_t cycles)
{
    ((Probe *)user)->elapsed += cycles;
    return 1;
}

static int advance(void *user,uint32_t cycles)
{
    Probe *p = user;
    if (p->pc == MAIN_RESUME+4u && p->mode != NORMAL)
        p->clock.advance = count_cycles;
    return count_cycles(p,cycles);
}

static void change_word(Probe *p,uint32_t address,int restore)
{
    if (!restore)
        assert(musashi_boot_read32(p->memory,address,&p->original_word));
    assert(musashi_boot_write32(p->memory,address,
        restore ? p->original_word : p->original_word ^ 4u));
}

static int observe(void *user,const void *state)
{
    Probe *p = user;
    MusashiCpuContext c;
    assert(musashi_boot_cpu_context(state,MUSASHI_CPU_CONTEXT_SOURCE,&c));
    p->pc = c.pc;
    uint32_t *r = (uint32_t *)(uintptr_t)c.gpr;
    if ((uint32_t)(c.pc-CTOR) < CTOR_RETURN+8u-CTOR) {
        ++p->ctor_issues;
        if (c.pc == CTOR && p->preset)
            assert(musashi_boot_write32(p->memory,0x80062998u,1u));
        if (p->mode == CHANGED_CTOR && (c.pc == CTOR || c.pc == CTOR+4u))
            change_word(p,CTOR+0x44u,c.pc != CTOR);
        if (p->mode == CHANGED_EDGE && c.pc == CTOR_RETURN+4u)
            r[31] = 0;
        return 1;
    }
    if (c.pc == MAIN_RESUME) {
        p->start = p->elapsed;
        if (p->mode == CHANGED_EDGE) r[31] = MAIN_RESUME;
        if (p->mode == CHANGED_GAME) change_word(p,MAIN_RESUME+24u,0);
    }
    if (c.pc == EXCHANGE) {
        assert(p->mode == NORMAL && p->game_issues == 7u && p->elapsed == p->start+14u);
        assert(r[4] == 0 && r[16] == 0xdead011bu && r[17] == 0x12343697u);
        assert(r[29] == 0x801fffc0u && r[30] == r[29] && r[31] == MAIN_RESUME+28u);
        assert(c.npc == EXCHANGE+4u && !c.delay_slot && !c.branch_pc);
        return 0;
    }
    if ((uint32_t)(c.pc-MAIN_RESUME) >= 28u) return 1;
    unsigned i = p->game_issues++;
    assert(c.pc == MAIN_RESUME+4u*i && p->elapsed == p->start+2u*i);
    assert(c.npc == (i == 6u ? EXCHANGE : c.pc+4u));
    assert(c.delay_slot == (i == 6u) && c.branch_pc == (i == 6u ? MAIN_RESUME+20u : 0u));
    if (i == 1u) {
        if (p->mode == CHANGED_GAME) change_word(p,MAIN_RESUME+24u,1);
        r[16] = 0xdead6003u;
    }
    if (i == 2u && p->mode != NORMAL) return 0;
    if (i == 3u) r[17] = 0x12344067u;
    return 1;
}

static int occupied(MusashiNativeLaneCall *call)
{
    (void)call;
    assert(0 && "diagnostic stops before the occupied delay slot");
    return 0;
}

static void quiet(void *user,const uint8_t *bytes,int32_t size)
{
    (void)user; (void)bytes; (void)size;
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
            static const MusashiNativeLaneEntry row = {MAIN_RESUME+24u,0u,occupied,"test",0u,1u};
            assert(musashi_native_lane_install(&row,1u) == 1);
        }
        for (unsigned preset=0; preset<2u; ++preset) {
            memset(memory,0,sizeof *memory);
            assert(musashi_boot_map_exe(memory,image,0x65000u));
            Probe p = {.memory=memory,.mode=mode,.preset=preset};
            p.clock = (MusashiExecutionClock){&p,advance};
            MusashiCallbackDevice callback = {.userdata=&p,.checkpoint=observe};
            MusashiStartupPrefixDevice devices = {.clock=&p.clock,.callback=&callback};
            MusashiEntryRunStop stop;
            assert(musashi_boot_run_entry_with_devices(memory,0,&devices,quiet,NULL,NULL,NULL,&stop)
                == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
            assert(p.ctor_issues == (preset ? 14u : 22u));
            if (mode == NORMAL)
                assert(stop.cpu.pc == EXCHANGE);
            else if (mode == PAID_NATIVE)
                assert(stop.cpu.pc == MAIN_RESUME+4u && p.game_issues == 2u &&
                    p.elapsed == p.start+4u);
            else
                assert(stop.cpu.pc == MAIN_RESUME+8u && p.game_issues == 3u &&
                    stop.cpu.r[16] == 0xdead011bu && p.elapsed == p.start+4u);
        }
    }
    free(image);
    puts("Game main resumed after both original constructor paths; exchange and ownership checks passed.");
    return 0;
}
