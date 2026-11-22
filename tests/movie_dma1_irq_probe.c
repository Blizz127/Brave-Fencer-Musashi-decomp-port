/* The mounted program runs its installed callback on a generated frame.
 * Its queue wraps and both native DMA devices continue during the interrupt. */
#include "musashi_guest_irq.h"
#include "musashi_gpu_controller.h"
#include "musashi_gpu_dma2.h"
#include "musashi_mdec_controller.h"
#include "musashi_scanline_timer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static MusashiBootMemory ram;
static MusashiGuestIrqImage image;
static MusashiScanlineTimer timer;
static MusashiMdecController decoder;
static MusashiIrqController irq;
static MusashiDmaController dma;
static MusashiGpuController gpu;
static MusashiGpuDma2 *channel;
static MusashiGpuIoDevice bus;
static unsigned cycles, pixels, returns;
static const uint32_t buffers[]={0x80150000u,0x80157800u};

static int healthy(void *p) { return p==&gpu; }
static uint64_t thread(void *p) { return healthy(p); }
static int set_display(void *p,int enable) { (void)enable; return healthy(p); }
static int set_mode(void *p,uint32_t value) { (void)value; return healthy(p); }

static int store_pixel(void *p,uint16_t x,uint16_t y,uint16_t pixel) {
    unsigned column=pixels/3520u, index=pixels%3520u;
    uint16_t expected;
    assert(x==column*16u+index%16u && y==10u+index/16u);
    assert(musashi_boot_read16(&ram,buffers[column%2u]+index*2u,&expected));
    assert(pixel==expected);
    ++pixels;
    return healthy(p);
}

static int read_bus(void *p,uint32_t address,uint32_t *value) {
    assert(p==&gpu);
    if (address>=0x1f801090u && address<=0x1f801098u)
        return musashi_mdec_read32(&decoder,address,value);
    if (address==MUSASHI_GPU_GP0 || address==MUSASHI_GPU_GP1)
        return musashi_gpu_controller_read32(&gpu,address,value);
    return musashi_gpu_dma2_read32(channel,address,value);
}
static int write_bus(void *p,uint32_t address,uint32_t value) {
    assert(p==&gpu);
    if (address>=0x1f801090u && address<=0x1f801098u)
        return musashi_mdec_write32(&decoder,address,value);
    if (address==MUSASHI_GPU_GP0 || address==MUSASHI_GPU_GP1)
        return musashi_gpu_controller_write32(&gpu,address,value);
    return musashi_gpu_dma2_write32(channel,address,value);
}
static int advance(void *unused,uint32_t delta) {
    (void)unused;
    cycles+=delta;
    return musashi_mdec_advance(&decoder,cycles) && musashi_gpu_dma2_advance(channel,cycles) &&
        musashi_scanline_timer_advance(&timer,cycles);
}
static int owns(void *machine,const MusashiGuestIrqDraw *offer) {
    return machine==&ram && offer->memory==&ram && offer->dma==&dma && offer->channel==channel &&
        offer->mdec==&decoder && offer->timer==&timer && offer->gpu==&bus;
}
static int exception_return(void *p) {
    assert(p==&irq);
    ++returns;
    return 1;
}
static void write_decoder(uint32_t address,uint32_t value) {
    assert(musashi_mdec_write32(&decoder,address,value));
}
static void output_request(uint32_t destination) {
    write_decoder(0x1f801090u,destination);
    write_decoder(0x1f801094u,0x00380020u);
    write_decoder(0x1f801098u,0x01000200u);
}
static void setup_devices(void) {
    musashi_dma_controller_init(&dma,&irq);
    assert(musashi_dma_controller_write32(&dma,0x1f8010f0u,0x3b3bbbbbu));
    assert(musashi_dma_controller_write32(&dma,0x1f8010f4u,0x00960000u));
    musashi_mdec_init(&decoder,&ram,&dma);
    write_decoder(0x1f801824u,0x60000000u);
    write_decoder(0x1f801820u,0x40000001u);
    for (unsigned i=0;i<32u;++i) write_decoder(0x1f801820u,0x01010101u);
    write_decoder(0x1f801820u,0x60000000u);
    for (unsigned i=0;i<32u;++i) write_decoder(0x1f801820u,i<4u ? 0x5a825a82u : 0u);
    /* Twenty columns of fourteen 16x16 macroblocks with varying DC coefficients. */
    const unsigned blocks=280u;
    write_decoder(0x1f801820u,0x3a000000u|6u*blocks);
    for (unsigned k=0;k<blocks*6u;++k)
        write_decoder(0x1f801820u,0xfe000400u+(k%6u)*9u+(k/6u*7u)%100u);
    assert(decoder.output_size==blocks*512u);
    output_request(buffers[0]);
    cycles=1792u*8u;
    assert(musashi_mdec_advance(&decoder,cycles));
    assert(decoder.transfers==1 && decoder.output_cursor==7168 && decoder.chcr[1]==0x200u);
    assert(irq.status==8);
    MusashiGpuBackend backend={.userdata=&gpu,.reset=healthy,.draw_mode=set_mode,
        .display_enable=set_display,.clear_fifo=healthy,.ready=healthy,.store_vram=store_pixel};
    assert(musashi_gpu_controller_init(&gpu,&backend));
    assert(musashi_gpu_controller_write32(&gpu,MUSASHI_GPU_GP1,0x04000002u));
    MusashiGpuDma2Device provider={&gpu,thread,healthy};
    channel=musashi_gpu_dma2_open(&ram,&dma,&gpu,&provider,cycles);
    assert(channel!=NULL);
    bus=(MusashiGpuIoDevice){.userdata=&gpu,.read32=read_bus,.write32=write_bus};
    assert(musashi_scanline_timer_init_paced(&timer,0));
    assert(musashi_scanline_timer_write32(&timer,0x1f801114u,0x107u));
    assert(musashi_scanline_timer_vblank_at(&timer,0));
    assert(musashi_irq_controller_write16(&irq,0x1f801074u,0xdu));
}

static void load_source(const char *path,void *destination,size_t size) {
    FILE *stream=fopen(path,"rb");
    assert(stream && destination && fread(destination,1,size,stream)==size && fgetc(stream)==EOF);
    assert(fclose(stream)==0);
}
static void install(const char *exe,const char *overlay) {
    static uint8_t source[0x65000];
    load_source(exe,source,sizeof source);
    assert(musashi_boot_map_irq_exe(&image,&ram,source,sizeof source));
    load_source(overlay,musashi_boot_ram_span(&ram,0x800ce5f8u,178176u),178176u);
    const uint32_t state[][2]={
        {0x8006bafcu,1},{0x8006bb0cu,0x80042e08u}, {0x8006bb2cu,8},
        {0x8006bb34u,0x80042780u}, {0x8006bb38u,0x8006cb14u},
        {0x8006bb3cu,0x801fffc0u}, {0x8006bb40u,0x8006bafcu}, {0x8006bb60u,0x80074750u},
        {0x8006cb68u,0x80042f8cu}, {0x8006cb84u,0x8006cb64u},
        {0x8006cb88u,0x1f801070u}, {0x8006cb8cu,0x1f801074u}, {0x8006cbc0u,0x1f8010f4u},
        {0x8006cbc8u,0x800d24d0u}, {0x8006cbccu,0x8005ba90u},
        {0x8006ba94u,0x1f801814u}, {0x8006ba98u,0x1f801110u},
        {0x80072748u,0x8005b7b0u}, {0x80072760u,0x8005b1c4u},
        {0x80072788u,0x100u}, {0x80072780u,0x80072740u}, {0x8007278cu,0x02000400u}, {0x80072794u,0x8001099cu},
        {0x80072858u,0x1f801810u}, {0x8007285cu,0x1f801814u},
        {0x80072860u,0x1f8010a0u}, {0x80072864u,0x1f8010a4u}, {0x80072868u,0x1f8010a8u},
        {0x8007288cu,61u}, {0x80072890u,61u},
        {0x800ec9b0u,0x80150000u}, {0x800ec9b4u,0x80157800u}, {0x800ec9b8u,0},
        {0x800ec9bcu,0x000a0000u}, {0x800ec9c0u,0x00dc0140u}, {0x800ec9c4u,0x00fa0000u}, {0x800ec9c8u,0x00dc0140u}, {0x800ec9ccu,0u},
        {0x800ec9d0u,0x000a0000u}, {0x800ec9d4u,0x00dc0010u}, {0x800ec9dcu,0u}, {0x800ec9f0u,1u}
    };
    for (size_t i=0;i<sizeof state/sizeof state[0];++i)
        assert(musashi_boot_write32(&ram,state[i][0],state[i][1]));
    musashi_boot_select_overlay_0004_words(1);
}

static uint32_t read_word(uint32_t address) {
    uint32_t value;
    assert(musashi_boot_read32(&ram,address,&value));
    return value;
}
int main(int argc,char **argv) {
    assert(argc==4);
    install(argv[1],argv[2]);
    setup_devices();
    uint32_t saved[12];
    for (unsigned i=0;i<12;++i) saved[i]=read_word(0x8006bb34u+4u*i);
    MusashiExecutionClock clock={NULL,advance};
    MusashiGuestIrqDrawHost host={0};
    assert(musashi_boot_bind_irq_draw(&image,&host,&ram,owns));
    MusashiGuestIrqDraw offer={.memory=&ram,.dma=&dma,.channel=channel,.clock=&clock,.gpu=&bus,
        .timer=&timer,.timer_read16=musashi_scanline_timer_read16,.host=&host,
        .generation=host.generation,.mdec=&decoder};
    MusashiGuestIrqServices services={.draw=&offer};
    MusashiGuestIrqStop stop={0};
    MusashiIrqDispatchDevice io={.userdata=&irq,.read16=musashi_irq_controller_read16,
        .write16=musashi_irq_controller_write16,.return_from_exception=exception_return};
    const char *mode=argv[3];
    if (!strcmp(mode,"foreign")) offer.mdec=NULL;
    if (!strcmp(mode,"badcode")) ram.bytes[0xd2510]^=1u;
    if (!strcmp(mode,"changedowner")) decoder.madr[1]+=4u;
    if (!strcmp(mode,"active")) output_request(buffers[1]);
    if (!strcmp(mode,"selector")) assert(musashi_boot_write32(&ram,0x8006cbc8u,0x8005ba90u));
    if (!strcmp(mode,"geometry")) assert(musashi_boot_write16(&ram,0x800ec9d4u,32u));
    if (!strcmp(mode,"overlap")) assert(musashi_boot_write32(&ram,0x800ec9b4u,buffers[0]+4u));
    if (!strcmp(mode,"member")) musashi_boot_select_overlay_0004_words(0);
    for (unsigned i=0;i<100;++i) {
        int result=musashi_boot_execute_guest_irq_with_services(&image,&ram,0x8006bb34u,saved,
            &io,&clock,&dma,&services,&stop);
        MusashiGpuDma2State state;
        assert(musashi_gpu_dma2_get_state(channel,&state));
        if (strcmp(mode,"pass")) {
            assert(!result && !returns && !pixels && stop.pc==0x80042ea8u);
            assert(!read_word(0x800ec9b8u) && read_word(0x800ec9d0u)==0x000a0000u);
            assert(!state.starts && !state.transfers);
            return 0;
        }
        assert(result==MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && stop.pc==0xb0u);
        if (pixels==70400u && state.transfers==20u && !(irq.status&irq.mask)) {
            /* Each of the twenty decoder outputs and GPU uploads completes
             * in its own interrupt, as on the first movie frame. */
            assert(state.starts==20u && returns==40u && decoder.transfers==20u);
            assert(decoder.chcr[1]==0x200u && decoder.output_cursor==143360u);
            assert(decoder.output_cursor==decoder.output_size && read_word(0x800ec9d8u)==1);
            assert(!read_word(0x800ec9b8u) && read_word(0x800ec9d0u)==0x00fa0000u);
            assert(musashi_gpu_dma2_close(channel));
            return 0;
        }
        if (!(irq.status&irq.mask)) {
            uint64_t due=UINT64_MAX;
            if (decoder.chcr[1]&0x01000000u) due=decoder.output_due;
            if ((state.chcr&0x01000000u) && state.due<due) due=state.due;
            assert(due!=UINT64_MAX && due>=cycles && advance(NULL,due-cycles));
        }
    }
    return 1;
}
