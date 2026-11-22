/* Tests only. Admit synthetic device state through the real channel and timer
 * owners, then check the game frame result from the caller-supplied US image. */
#include "musashi_gpu_controller.h"
#include "musashi_gpu_dma2.h"
#include "musashi_scanline_timer.h"

typedef struct DrawFixture {
    MusashiGpuController gpu;
    MusashiGpuDma2 *channel;
    MusashiDmaController *dma;
    MusashiScanlineTimer timer;
    MusashiGuestIrqDraw service;
    MusashiGpuIoDevice gpu_io;
    MusashiGuestIrqDrawHost host;
    const MusashiBootMemory *memory;
    const MusashiExecutionClock *clock;
    uint64_t cycle;
    uint16_t sampled_count;
    unsigned chcr_reads, status_reads, timer_reads;
} DrawFixture;

static int draw_ready(void *p) { return p!=NULL; }
static int draw_mode(void *p,uint32_t value) { (void)value; return draw_ready(p); }
static int draw_enable(void *p,int enabled) { (void)enabled; return draw_ready(p); }

static int draw_gpu_read(void *p,uint32_t address,uint32_t *out) {
    DrawFixture *d=p;
    if (address==0x1f8010a8u && musashi_gpu_dma2_read32(d->channel,address,out)) {
        ++d->chcr_reads;
        return 1;
    }
    if (address==MUSASHI_GPU_GPUSTAT && musashi_gpu_controller_read32(&d->gpu,address,out)) {
        ++d->status_reads;
        return 1;
    }
    return 0;
}

static int draw_timer_read(void *p,uint32_t address,uint16_t *out) {
    DrawFixture *d=p;
    if (address!=0x1f801110u || !musashi_scanline_timer_read16(&d->timer,address,out)) return 0;
    ++d->timer_reads;
    d->sampled_count=*out;
    return 1;
}

static int draw_owns(void *machine,const MusashiGuestIrqDraw *offer) {
    DrawFixture *d=machine;
    return offer->memory==d->memory && offer->clock==d->clock && offer->dma==d->dma &&
        offer->channel==d->channel && offer->gpu==&d->gpu_io && offer->gpu->userdata==d &&
        offer->gpu->read32==draw_gpu_read && offer->timer==d && offer->timer_read16==draw_timer_read;
}

static int draw_advance(DrawFixture *d,uint32_t delta) {
    d->cycle+=delta;
    return musashi_gpu_dma2_advance(d->channel,d->cycle) &&
           musashi_scanline_timer_advance(&d->timer,d->cycle);
}

static void draw_begin(DrawFixture *d,MusashiBootMemory *ram,MusashiDmaController *dma,
                       MusashiExecutionClock *clock,uint64_t (*thread)(void *),const char *mode) {
    /* The independent test starts with an empty queue at slot five and a
     * deliberately stale frame value. Both start from the paced timer;
     * neither is taken from a save, disc, or gameplay RAM capture. */
    assert(musashi_boot_write32(ram,0x8007288cu,5u));
    assert(musashi_boot_write32(ram,0x80072890u,!strcmp(mode,"draw-queue") ? 6u : 5u));
    assert(musashi_boot_write32(ram,0x80072790u,1u));
    assert(musashi_boot_write32(ram,0x80072794u,!strcmp(mode,"draw-binding") ? 0u : 0x8001099cu));
    assert(musashi_boot_write32(ram,0x80072868u,0x1f8010a8u));
    assert(musashi_boot_write32(ram,0x8006ba94u,MUSASHI_GPU_GPUSTAT));
    assert(musashi_boot_write32(ram,0x8006ba98u,0x1f801110u));
    assert(musashi_boot_write32(ram,0x8006ba9cu,3u));
    assert(musashi_boot_write16(ram,0x800b9a02u,!strcmp(mode,"draw-frame-slot") ? 2u : 1u));
    assert(musashi_boot_write32(ram,0x800ae814u,0x314159u));
    assert(musashi_boot_write32(ram,0x8006cbccu,0x8005ba90u));
    if (!strcmp(mode,"draw-image")) ram->bytes[0x5bc14u]^=1u;

    MusashiGpuBackend backend={.userdata=d,.reset=draw_ready,.draw_mode=draw_mode,
        .display_enable=draw_enable,.clear_fifo=draw_ready,.ready=draw_ready};
    assert(musashi_gpu_controller_init(&d->gpu,&backend));
    assert(musashi_gpu_controller_write32(&d->gpu,MUSASHI_GPU_GP1,0x04000002u));
    assert(musashi_scanline_timer_init_paced(&d->timer,0));
    assert(musashi_scanline_timer_write32(&d->timer,0x1f801114u,0x107u));
    assert(musashi_scanline_timer_vblank_at(&d->timer,0));
    d->cycle=14u*2146u+(!strcmp(mode,"draw-timer") ? 2100u : 100u);
    assert(musashi_scanline_timer_advance(&d->timer,d->cycle));

    MusashiGpuDma2Device device={d,thread,draw_ready};
    d->dma=dma;
    d->clock=clock;
    d->memory=ram;
    d->channel=musashi_gpu_dma2_open(ram,dma,&d->gpu,&device,d->cycle);
    assert(d->channel && d->channel==dma->gpu_owner);
    d->gpu_io=(MusashiGpuIoDevice){.userdata=d,.read32=draw_gpu_read};
    d->service=(MusashiGuestIrqDraw){.memory=ram,.dma=dma,.channel=d->channel,
        .clock=clock,.gpu=&d->gpu_io,.timer=d,.timer_read16=draw_timer_read,
        .generation=d->host.generation,.host=&d->host};
    if (!strcmp(mode,"draw-lease")) d->service.channel=NULL;
    if (!strcmp(mode,"draw-owner")) d->service.memory=NULL;
    if (!strcmp(mode,"draw-clock")) d->service.clock=NULL;

    /* One terminating zero-payload header. Only the transfer owner raises IRQ3. */
    uint32_t dpcr;
    assert(musashi_dma_controller_read32(dma,0x1f8010f0u,&dpcr));
    assert(musashi_dma_controller_write32(dma,0x1f8010f0u,dpcr|0x800u));
    assert(musashi_dma_controller_write32(dma,0x1f8010f4u,0x00840000u));
    assert(musashi_boot_write32(ram,0x801fff00u,0x00ffffffu));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a0u,0x801fff00u));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a8u,0x01000401u));
    assert(draw_advance(d,1u) && !dma->irq->status);
    assert(draw_advance(d,1u) && dma->irq->status==8u && dma->interrupt==0x84840000u);
}

static void draw_check(DrawFixture *d,MusashiBootMemory *ram,uint32_t stop,
                       const char *mode,int success) {
    uint32_t frame,busy;
    assert(musashi_boot_read32(ram,0x80072790u,&busy));
    assert(musashi_boot_read32(ram,0x800ae814u,&frame));
    if (success) {
        assert(stop==0xb0u && !busy && d->chcr_reads==2 && d->status_reads==1 && d->timer_reads==1);
        assert(d->sampled_count==(!strcmp(mode,"draw-timer") ? 15u : 14u));
        assert(frame==(uint32_t)d->sampled_count-3u);
    } else {
        assert(stop==0x80042ea8u && busy==1u && frame==0x314159u);
        assert(!d->chcr_reads && !d->status_reads && !d->timer_reads);
    }
    MusashiGpuDma2State dma_state;
    assert(musashi_gpu_dma2_get_state(d->channel,&dma_state));
    assert(dma_state.transfers==1u && dma_state.header_words==1u && !dma_state.accepted_words);
    assert(!d->gpu.accepted_gp0_words && d->dma->interrupt==0x00840000u);
    assert(musashi_gpu_dma2_close(d->channel));
}
