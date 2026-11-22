/* Cold caller execution with five test providers in place of graphics services
 * and subsystem startup. The original wrapper itself executes on every run. */
#include "musashi_boot_memory.h"
#include "musashi_native_lane.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RETURN_JR = 0x80014230u, CALLER = 0x800101c8u, CD_ENTRY = 0x8005fc68u };
enum Trial { NORMAL, CLOCK_CHANGED, STACK_CHANGED };
typedef struct ReturnProbe {
    MusashiBootMemory *ram;
    MusashiExecutionClock clock;
    enum Trial trial;
    uint32_t pc, cycle, first, rows;
} ReturnProbe;
static unsigned service_count;

static int test_service(MusashiNativeLaneCall *call)
{
    static const uint32_t services[] = {
        0x80059234u, 0x800594ccu, 0x80059658u, 0x80016714u, 0x800142c8u
    };
    assert(service_count < sizeof services / sizeof services[0]);
    assert(musashi_native_lane_current() == services[service_count]);
    if (service_count < 3u) assert(call->r[4] == 0u);
    if (service_count == 3u) assert(call->r[4] == 0x800ba118u && call->r[5] == 152u);
    if (++service_count == 5u) call->r[4] = 0x12345678u;
    return 1;
}

static int count_cycles(void *data, uint32_t cost)
{
    ReturnProbe *p = data;
    p->cycle += cost;
    return 1;
}

static int advance(void *data, uint32_t cost)
{
    ReturnProbe *p = data;
    if (p->trial != NORMAL && p->pc == CALLER)
        p->clock.advance = count_cycles;
    return count_cycles(data,cost);
}

static int checkpoint(void *data, const void *state)
{
    ReturnProbe *p = data;
    MusashiCpuContext c;
    assert(musashi_boot_cpu_context(state,MUSASHI_CPU_CONTEXT_SOURCE,&c));
    p->pc = c.pc;
    if (c.pc == RETURN_JR && p->trial == STACK_CHANGED) {
        assert(c.gpr[29] == 0x801fffc0u && c.gpr[31] == CALLER);
        assert(musashi_boot_write32(p->ram,c.gpr[29]-8u,0x87654321u));
    }
    if ((uint32_t)(c.pc-CALLER) >= 12u && c.pc != CD_ENTRY) return 1;
    unsigned i = p->rows++;
    assert(i < 4u && c.pc == (i == 3u ? CD_ENTRY : CALLER+4u*i));
    if (!i) p->first = p->cycle;
    assert(p->cycle-p->first == 2u*i && service_count == 5u);
    assert(c.npc == (i == 2u ? CD_ENTRY : c.pc+4u));
    assert(c.delay_slot == (i == 2u) && c.branch_pc == (i == 2u ? CALLER+4u : 0u));
    assert(c.gpr[29] == 0x801fffc0u && c.gpr[30] == c.gpr[29]);
    assert(c.gpr[31] == (i >= 2u ? CALLER+12u : CALLER));
    assert(c.gpr[4] == (i ? 0u : 0x12345678u));
    return c.pc != CD_ENTRY;
}

static void quiet(void *data, const uint8_t *bytes, int32_t count)
{
    (void)data; (void)bytes; (void)count;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *fp = fopen(argv[1],"rb");
    if (!fp) return 77;
    uint8_t *image = malloc(0x65000u);
    assert(image && fread(image,1,0x65000u,fp) == 0x65000u && fgetc(fp) == EOF);
    fclose(fp);
    int alias = 0;
    MusashiBootMemory *ram = musashi_native_lane_map_guest(sizeof *ram,&alias);
    if (!ram) { free(image); return 77; }
    MusashiNativeLaneEntry entries[] = {
        {.guest_pc=0x80059234u,.fn=test_service,.name="graphics1"},
        {.guest_pc=0x800594ccu,.fn=test_service,.name="graphics2"},
        {.guest_pc=0x80059658u,.fn=test_service,.name="graphics3"},
        {.guest_pc=0x80016714u,.fn=test_service,.name="clear"},
        {.guest_pc=0x800142c8u,.fn=test_service,.name="initialize"}
    };
    assert(musashi_native_lane_install(entries,5u) == 5);
    for (enum Trial trial=NORMAL; trial<=STACK_CHANGED; ++trial) {
        ReturnProbe p = {.ram=ram,.trial=trial};
        p.clock = (MusashiExecutionClock){&p,advance};
        MusashiCallbackDevice callback = {.userdata=&p,.checkpoint=checkpoint};
        MusashiStartupPrefixDevice devices = {.clock=&p.clock,.callback=&callback};
        MusashiEntryRunStop stop;
        memset(ram,0,sizeof *ram);
        assert(musashi_boot_map_exe(ram,image,0x65000u));
        service_count = 0;
        assert(musashi_boot_run_entry_with_devices(ram,0,&devices,quiet,NULL,NULL,NULL,&stop)
               == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
        if (trial == CLOCK_CHANGED) {
            assert(stop.cpu.pc == CALLER && p.rows == 1u && p.cycle == p.first+2u);
        } else {
            assert(stop.cpu.pc == CD_ENTRY && p.rows == 4u && p.cycle == p.first+6u);
        }
    }
    free(image);
    puts("Cold graphics wrapper return, CD call and clock ownership passed.");
    return 0;
}
