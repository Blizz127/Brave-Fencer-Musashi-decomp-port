#ifdef NDEBUG
#undef NDEBUG
#endif
/* No game bytes or disc assumptions. The optional argument is the user's
 * original executable; the project's full-file mapper verifies its identity. */
#include <assert.h>
#include "../pc_port/mips_formatter.c"

enum { SLOT = 0x8006cbc4u, CONTROL_CELL = SLOT-4u, OLD = 0x80123450u,
       CALLBACK = 0x8005ba90u, CONTROL = 0xfe123456u, UNSET = 0xd15ea5edu };
static MusashiBootMemory memory, foreign;
typedef struct Bus {
    uint32_t slot, published, control;
    unsigned reads, writes, cycles;
    int reject, corrupt;
} Bus;

static uint32_t get(uint32_t addr) {
    uint32_t result;
    assert(musashi_boot_read32(&memory, addr, &result));
    return result;
}

static void put(uint32_t addr, uint32_t value) {
    assert(musashi_boot_write32(&memory, addr, value));
}

static int read_dicr(void *context, uint32_t addr, uint32_t *result) {
    Bus *bus = context;
    assert(addr == DMA_REG_DICR && get(bus->slot) == bus->published);
    ++bus->reads;
    *result = bus->control;
    if (bus->corrupt)
        memory.bytes[DMA_REG_END-0x80000000u-1u] ^= 1;
    return bus->reject != 1;
}

static int write_dicr(void *context, uint32_t addr, uint32_t value) {
    Bus *bus = context;
    assert(addr == DMA_REG_DICR);
    ++bus->writes;
    if (bus->reject == 2) return 0;
    bus->control = value;
    return 1;
}

static int advance(void *context, uint32_t cycles) {
    ((Bus *)context)->cycles += cycles;
    return 1;
}

static void seed(Bus *bus, unsigned index, uint32_t callback, uint32_t old) {
    *bus = (Bus){.slot = SLOT+index*4u, .published = callback, .control = CONTROL};
    put(CONTROL_CELL, DMA_REG_DICR);
    put(bus->slot, old);
}

static void boundaries(void) {
    Bus bus = {0};
    MusashiDmaCallbackRegistrationDevice device = {&bus, read_dicr, write_dicr};
    DmaRegistrationFrame frame = {&memory, &device, 0};
    FormatterCpu cpu = {0};
    CdIrqReturn returns[5] = {0};
    uint32_t out = UNSET;
    returns[3].pc = 0x8005b908u;
    returns[4].pc = DMA_REG_VENEER+0x20u;
    returns[4].target = DMA_REG_BODY;
    cpu.pc = DMA_REG_BODY;
    cpu.r[31] = returns[4].pc;
    assert(cd_irq_select_dma(&cpu, returns, 5, &frame));
    assert(!code_word(&foreign,&cpu,cpu.pc,&out) && out == UNSET);
    assert(!cpu_read32(&foreign,&cpu,DMA_REG_DICR,&out) && out == UNSET);
    assert(!cpu_write32(&foreign,&cpu,DMA_REG_DICR,0));
    assert(!cpu_read32(&memory,&cpu,DMA_REG_DICR-4u,&out) && out == UNSET);
    assert(!cpu_write32(&memory,&cpu,DMA_REG_DICR-4u,0));
    assert(!cd_irq_select_dma(&cpu,returns,4,&frame) && !cpu.dma_registration);
    cpu.r[31] += 4u;
    assert(!cd_irq_select_dma(&cpu,returns,5,&frame));
    cpu.r[31] -= 4u;
    returns[3].pc += 4u;
    assert(!cd_irq_select_dma(&cpu,returns,5,&frame));
    returns[3].pc -= 4u;
    frame.device = NULL;
    assert(!cd_irq_select_dma(&cpu,returns,5,&frame));
    frame.device = &device;
    cpu.pc = DMA_REG_END;
    assert(!cd_irq_select_dma(&cpu,returns,5,&frame));
    assert(cd_irq_select_dma(&cpu,returns,4,&frame) && !cpu.dma_registration);
    assert(!bus.reads && !bus.writes);
    assert(!musashi_boot_call_80042f8c(&foreign,NULL,2,0,&out) && out == UNSET);
    assert(!musashi_boot_call_800425e0(&foreign,NULL,2,0,&out) && out == UNSET);
}

static void changed_source(void) {
    Bus bus;
    MusashiDmaCallbackRegistrationDevice device = {&bus,read_dicr,write_dicr};
    put(0x8006cb84u,0x80060000u);
    put(0x80060004u,DMA_REG_BODY);
    for (int veneer=0; veneer<=1; ++veneer) {
        uint32_t previous = UNSET;
        seed(&bus,2,CALLBACK,OLD);
        bus.corrupt=1;
        int accepted = veneer ?
            musashi_boot_call_800425e0(&memory,&device,2,CALLBACK,&previous) :
            musashi_boot_call_80042f8c(&memory,&device,2,CALLBACK,&previous);
        assert(!accepted && previous==UNSET && bus.reads==1 && !bus.writes);
        assert(get(bus.slot)==CALLBACK && bus.control==CONTROL);
        memory.bytes[DMA_REG_END-0x80000000u-1u] ^= 1;
    }
}

static void run_carrier(void) {
    const uint32_t saved_sp = 0x801e0000u;
    uint32_t instruction;
    Bus bus;
    MusashiDmaCallbackRegistrationDevice device = {&bus,read_dicr,write_dicr};
    DmaRegistrationFrame active = {&memory,&device,0};
    MusashiCdIrqServices services = {.dma=&device};
    MusashiExecutionClock clock = {&bus,advance};
    CdIrqReturn returns[5] = {0};
    returns[0] = (CdIrqReturn){.target=0x8001a338u,.pc=0x80045308u,.sp=saved_sp-0x50u};
    returns[3].pc = 0x8005b908u;
    returns[4] = (CdIrqReturn){.target=DMA_REG_BODY,.pc=DMA_REG_VENEER+0x20u,
                               .sp=saved_sp-0xd0u};
    for (int scenario=0; scenario<4; ++scenario) {
        uint32_t old = scenario == 1 ? CALLBACK : OLD;
        seed(&bus,2,CALLBACK,old);
        bus.reject = scenario == 2 ? 1 : scenario == 3 ? 2 : 0;
        FormatterCpu cpu = {0};
        cpu.pc = DMA_REG_VENEER+0x18u; cpu.npc=cpu.pc+4u;
        cpu.r[2]=DMA_REG_BODY; cpu.r[4]=2; cpu.r[5]=CALLBACK;
        cpu.r[16]=0x1234; cpu.r[29]=returns[4].sp; cpu.r[31]=returns[3].pc;
        cpu.clock=&clock;
        assert(cd_irq_word(&memory,&cpu,cpu.pc,&instruction));
        assert(formatter_step(&memory,&cpu) && formatter_step(&memory,&cpu));
        assert(cpu.pc == DMA_REG_BODY && cpu.r[31] == returns[4].pc);
        assert(cd_irq_call(&memory,&cpu,&services,returns,4,saved_sp,
                           DMA_REG_VENEER+0x18u,instruction));
        assert(!cd_irq_call(&memory,&cpu,&services,returns,3,saved_sp,
                            DMA_REG_VENEER+0x18u,instruction));
        unsigned steps=0;
        while (cpu.pc != returns[4].pc && steps++ < 64) {
            assert(cd_irq_select_dma(&cpu,returns,5,&active));
            assert(cd_irq_word(&memory,&cpu,cpu.pc,&instruction));
            if (!formatter_step(&memory,&cpu)) break;
        }
        assert(get(bus.slot) == CALLBACK && cpu.r[16] == 0x1234 && cpu.r[29] == returns[4].sp);
        if (scenario < 2) {
            assert(cpu.pc == returns[4].pc && cpu.r[31] == returns[4].pc && cpu.r[2] == old);
            assert(cd_irq_select_dma(&cpu,returns,4,&active) && !cpu.dma_registration);
        } else {
            assert(cpu.pc != returns[4].pc && bus.reads == 1 && bus.writes == (scenario == 3));
        }
        static const unsigned expected_steps[] = {28,11,16,24};
        static const unsigned expected_cycles[] = {65,27,40,57};
        assert(steps==expected_steps[scenario] && bus.cycles==expected_cycles[scenario]);
        printf("DMA carrier mode=%d body_steps=%u cycles_with_call=%u reads=%u writes=%u\n",
               scenario,steps,bus.cycles,bus.reads,bus.writes);
    }
}

int main(int argc, char **argv) {
    boundaries();
    puts("DMA synthetic refusal checks passed");
    if (argc == 1) return 0;
    assert(argc == 2);
    FILE *f=fopen(argv[1],"rb");
    assert(f);
    uint8_t *bytes=malloc(CODE_IMAGE_SIZE+1u);
    assert(bytes && fread(bytes,1,CODE_IMAGE_SIZE+1u,f) == CODE_IMAGE_SIZE);
    assert(fclose(f) == 0);
    assert(musashi_boot_map_exe(&memory,bytes,CODE_IMAGE_SIZE));
    free(bytes);
    changed_source();
    run_carrier();
    puts("DMA guest registration and return checks passed");
    return 0;
}
