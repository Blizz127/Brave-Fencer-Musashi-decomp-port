/* Tests only. Admit synthetic device state through the real channel and timer
 * owners, then check the game frame result from the caller-supplied US image. */
#include "musashi_gpu_controller.h"
#include "musashi_gpu_dma2.h"
#include "musashi_scanline_timer.h"

enum { TEST_DRAW_LIST = 0x801e3000u, TEST_LIST_OPERATION = 0x8005b710u };
typedef struct DrawQueueCase {
    const char *name;
    uint32_t tail, head, operation, list, argument;
    int complete;
} DrawQueueCase;

static const DrawQueueCase queue_cases[] = {
    {"draw-pending",           2, 3, TEST_LIST_OPERATION, TEST_DRAW_LIST,      0, 1},
    {"draw-pending-frame-zero", 11, 12, TEST_LIST_OPERATION, TEST_DRAW_LIST,   0, 1},
    {"draw-pending-next",      7, 8, TEST_LIST_OPERATION, TEST_DRAW_LIST,      0, 1},
    {"draw-pending-wrap",     63, 0, TEST_LIST_OPERATION, TEST_DRAW_LIST,      0, 1},
    {"draw-pending-operation", 7, 8, 0,                   TEST_DRAW_LIST,      0, 0},
    {"draw-pending-argument",  7, 8, TEST_LIST_OPERATION, TEST_DRAW_LIST,      1, 0},
    {"draw-pending-unaligned", 7, 8, TEST_LIST_OPERATION, TEST_DRAW_LIST + 1u, 0, 0},
    {"draw-pending-multiple",  7, 9, TEST_LIST_OPERATION, TEST_DRAW_LIST,      0, 0}
};

static const DrawQueueCase *draw_queue_case(const char *name) {
    for (size_t i = 0; i < sizeof(queue_cases)/sizeof(queue_cases[0]); ++i)
        if (strcmp(queue_cases[i].name, name) == 0) return &queue_cases[i];
    return NULL;
}

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
    uint16_t sampled_count, frame_slot;
    unsigned chcr_reads, status_reads, timer_reads;
    const DrawQueueCase *queued;
    unsigned gpu_writes, dma_writes, fills, pixels;
    int upload, upload_active;
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

static int draw_gpu_write(void *p, uint32_t address, uint32_t value) {
    DrawFixture *d = p;
    if (address == MUSASHI_GPU_GP1) {
        if (!musashi_gpu_controller_write32(&d->gpu, address, value)) return 0;
        ++d->gpu_writes;
        return 1;
    }
    if (!musashi_gpu_dma2_write32(d->channel, address, value)) return 0;
    ++d->dma_writes;
    return 1;
}

static int draw_test_fill(void *p, const MusashiGpuFill *fill) {
    DrawFixture *d = p;
    assert(fill->x == 48 && fill->y == 37 && fill->width == 16 && fill->height == 5);
    assert(fill->color == (29u | (12u << 5)));
    ++d->fills;
    return 1;
}

enum { UPLOAD_X=73, UPLOAD_Y=29, UPLOAD_WIDTH=32, UPLOAD_HEIGHT=24,
       UPLOAD_PIXELS=UPLOAD_WIDTH*UPLOAD_HEIGHT, UPLOAD_WORDS=UPLOAD_PIXELS/2 };

static uint16_t upload_pixel(unsigned index) { return (uint16_t)(0x411du + 3u*index); }

static int draw_store_pixel(void *p,uint16_t x,uint16_t y,uint16_t pixel) {
    DrawFixture *d=p;
    unsigned i=d->pixels;
    assert(i<UPLOAD_PIXELS && x==UPLOAD_X+i%UPLOAD_WIDTH && y==UPLOAD_Y+i/UPLOAD_WIDTH);
    assert(pixel==upload_pixel(i));
    ++d->pixels;
    return 1;
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
        offer->gpu->read32==draw_gpu_read && offer->gpu->write32==draw_gpu_write &&
        offer->timer==d && offer->timer_read16==draw_timer_read;
}

static int draw_advance(DrawFixture *d,uint32_t delta) {
    d->cycle+=delta;
    return musashi_gpu_dma2_advance(d->channel,d->cycle) &&
           musashi_scanline_timer_advance(&d->timer,d->cycle);
}

static void draw_begin_upload(DrawFixture *d,MusashiBootMemory *ram) {
    const uint32_t source=0x801e6000u;
    assert(musashi_gpu_controller_write32(&d->gpu,MUSASHI_GPU_GP0,0xa0000000u));
    assert(musashi_gpu_controller_write32(&d->gpu,MUSASHI_GPU_GP0,UPLOAD_X | (UPLOAD_Y<<16)));
    assert(musashi_gpu_controller_write32(&d->gpu,MUSASHI_GPU_GP0,UPLOAD_WIDTH | (UPLOAD_HEIGHT<<16)));
    for (unsigned i=0;i<UPLOAD_WORDS;++i) {
        uint32_t word=upload_pixel(2u*i) | ((uint32_t)upload_pixel(2u*i+1u)<<16);
        assert(musashi_boot_write32(ram,source+4u*i,word));
    }
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a0u,source));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a4u,(UPLOAD_HEIGHT<<16) | (UPLOAD_WIDTH/2u)));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a8u,0x01000201u));
    assert(d->pixels==UPLOAD_PIXELS && d->gpu.stored_pixels==UPLOAD_PIXELS);
    MusashiGpuDma2State s;
    assert(musashi_gpu_dma2_get_state(d->channel,&s) && s.chcr==0x01000201u && s.starts==s.transfers+1);
    if (!d->upload_active) {
        assert(draw_advance(d,UPLOAD_WORDS) && !d->dma->irq->status);
        assert(draw_advance(d,1) && d->dma->irq->status==8 && d->dma->interrupt==0x84840000u);
    }
}

static void draw_begin(DrawFixture *d,MusashiBootMemory *ram,MusashiDmaController *dma,
                       MusashiExecutionClock *clock,uint64_t (*thread)(void *),const char *mode) {
    /* Fill GPU and timer state from the synthetic fixture. */
    const DrawQueueCase *q = draw_queue_case(mode);
    d->queued = q;
    d->upload=!strncmp(mode,"draw-upload",11);
    d->upload_active=!strcmp(mode,"draw-upload-active");
    uint32_t head = 5, tail = !strcmp(mode,"draw-queue") ? 6u : 5u;
    if (q) {
        head = q->head;
        tail = q->tail;
        const uint32_t record = 0x800c5510u + 96u * tail;
        assert(musashi_boot_write32(ram, record, q->operation));
        assert(musashi_boot_write32(ram, record + 4, q->list));
        assert(musashi_boot_write32(ram, record + 8, q->argument));
        assert(musashi_boot_write32(ram, 0x8007285cu, MUSASHI_GPU_GP1));
        assert(musashi_boot_write32(ram, 0x80072860u, 0x1f8010a0u));
        assert(musashi_boot_write32(ram, 0x80072864u, 0x1f8010a4u));
        /* One terminating, three-word fill packet at a test-only RAM address. */
        assert(musashi_boot_write32(ram, TEST_DRAW_LIST, 0x03ffffffu));
        assert(musashi_boot_write32(ram, TEST_DRAW_LIST + 4, 0x020060e8u));
        assert(musashi_boot_write32(ram, TEST_DRAW_LIST + 8, 48u | (37u << 16)));
        assert(musashi_boot_write32(ram, TEST_DRAW_LIST + 12, 16u | (5u << 16)));
    }
    assert(musashi_boot_write32(ram,0x8007288cu,head));
    assert(musashi_boot_write32(ram,0x80072890u,tail));
    assert(musashi_boot_write32(ram,0x80072790u,1u));
    assert(musashi_boot_write32(ram,0x80072794u,!strcmp(mode,"draw-binding") ? 0u : 0x8001099cu));
    assert(musashi_boot_write32(ram,0x80072868u,0x1f8010a8u));
    assert(musashi_boot_write32(ram,0x8006ba94u,MUSASHI_GPU_GPUSTAT));
    assert(musashi_boot_write32(ram,0x8006ba98u,0x1f801110u));
    assert(musashi_boot_write32(ram,0x8006ba9cu,3u));
    d->frame_slot=1;
    if (!strcmp(mode,"draw-frame-zero") || !strcmp(mode,"draw-pending-frame-zero"))
        d->frame_slot=0;
    else if (!strcmp(mode,"draw-frame-slot")) d->frame_slot=2;
    else if (!strcmp(mode,"draw-frame-out-of-range")) d->frame_slot=UINT16_MAX;
    assert(musashi_boot_write16(ram,0x800b9a02u,d->frame_slot));
    assert(musashi_boot_write32(ram,0x800ae810u,0x2468acu));
    assert(musashi_boot_write32(ram,0x800ae814u,0x314159u));
    assert(musashi_boot_write32(ram,0x8006cbccu,0x8005ba90u));
    if (!strcmp(mode,"draw-image")) ram->bytes[0x5bc14u]^=1u;

    MusashiGpuBackend backend={.userdata=d,.reset=draw_ready,.draw_mode=draw_mode,
        .display_enable=draw_enable,.clear_fifo=draw_ready,.ready=draw_ready,.fill_vram=draw_test_fill,
        .store_vram=draw_store_pixel};
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
    d->gpu_io=(MusashiGpuIoDevice){.userdata=d,.read32=draw_gpu_read,.write32=draw_gpu_write};
    d->service=(MusashiGuestIrqDraw){.memory=ram,.dma=dma,.channel=d->channel,
        .clock=clock,.gpu=&d->gpu_io,.timer=d,.timer_read16=draw_timer_read,
        .generation=d->host.generation,.host=&d->host};
    if (!strcmp(mode,"draw-lease")) d->service.channel=NULL;
    if (!strcmp(mode,"draw-owner")) d->service.memory=NULL;
    if (!strcmp(mode,"draw-clock")) d->service.clock=NULL;

    /* Only the native transfer owner raises the DMA IRQ. */
    uint32_t dpcr;
    assert(musashi_dma_controller_read32(dma,0x1f8010f0u,&dpcr));
    assert(musashi_dma_controller_write32(dma,0x1f8010f0u,dpcr|0x800u));
    assert(musashi_dma_controller_write32(dma,0x1f8010f4u,0x00840000u));
    if (d->upload && !d->upload_active) {
        draw_begin_upload(d,ram);
        if (!strcmp(mode,"draw-upload-other-irq")) {
            /* Leave DMA2's completed CHCR idle, acknowledge its IRQ, then
             * register the same callback in an unrelated DMA slot. */
            assert(musashi_dma_controller_write32(dma,0x1f8010f4u,0x04880000u));
            assert(musashi_irq_controller_write16(dma->irq,0x1f801070u,0));
            assert(musashi_boot_write32(ram,0x8006cbd0u,0x8005ba90u));
            assert(musashi_dma_controller_complete(dma,3));
            assert(dma->irq->status==8 && dma->interrupt==0x88880000u);
        }
        return;
    }
    /* In the active-upload negative, the preceding linked list supplies a
     * genuine, still-pending IRQ while the next request is busy. */
    assert(musashi_boot_write32(ram,0x801fff00u,0x00ffffffu));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a0u,0x801fff00u));
    assert(musashi_gpu_dma2_write32(d->channel,0x1f8010a8u,0x01000401u));
    assert(draw_advance(d,1u) && !dma->irq->status);
    assert(draw_advance(d,1u) && dma->irq->status==8u && dma->interrupt==0x84840000u);
    if (d->upload_active) draw_begin_upload(d,ram);
}

static void draw_check(DrawFixture *d,MusashiBootMemory *ram,uint32_t stop,
                       const char *mode,int success) {
    uint32_t frame[2],busy;
    const uint32_t seed[2]={0x2468acu,0x314159u};
    assert(musashi_boot_read32(ram,0x80072790u,&busy));
    assert(musashi_boot_read32(ram,0x800ae810u,&frame[0]));
    assert(musashi_boot_read32(ram,0x800ae814u,&frame[1]));
    if (success) {
        assert(d->frame_slot<=1 && frame[d->frame_slot]==(uint32_t)d->sampled_count-3u);
        unsigned neighbor=1u-d->frame_slot;
        assert(frame[neighbor]==seed[neighbor]);
    } else assert(frame[0]==seed[0] && frame[1]==seed[1]);
    if (d->queued && success) {
        assert(stop==0xb0u && !busy && d->timer_reads==1);
    } else if (success) {
        assert(stop==0xb0u && !busy && d->chcr_reads==2 && d->status_reads==1 && d->timer_reads==1);
        assert(d->sampled_count==(!strcmp(mode,"draw-timer") ? 15u : 14u));
    } else {
        assert(stop==0x80042ea8u && busy==1u);
        assert(!d->chcr_reads && !d->status_reads && !d->timer_reads);
    }
    MusashiGpuDma2State dma_state;
    assert(musashi_gpu_dma2_get_state(d->channel,&dma_state));
    if (d->queued) {
        uint32_t head, tail;
        assert(musashi_boot_read32(ram, 0x8007288cu, &head) && head==d->queued->head);
        assert(musashi_boot_read32(ram, 0x80072890u, &tail));
        assert(tail == (success ? head : d->queued->tail));
    }
    if (d->upload) {
        assert(!dma_state.fault && dma_state.starts==(d->upload_active ? 2u : 1u) && dma_state.transfers==1);
        assert(dma_state.chcr==(d->upload_active ? 0x01000201u : 0x201u));
        assert(dma_state.header_words==0 && dma_state.payload_words==UPLOAD_WORDS);
        assert(dma_state.accepted_in_transfer==UPLOAD_WORDS && d->gpu.accepted_gp0_words==3u+UPLOAD_WORDS);
        assert(d->pixels==UPLOAD_PIXELS && !d->fills && !d->gpu_writes && !d->dma_writes);
    } else if (d->queued && success) {
        assert(dma_state.starts==2 && dma_state.transfers==2 && dma_state.start_madr==(TEST_DRAW_LIST & 0xffffffu));
        assert(dma_state.accepted_words==3 && d->gpu.accepted_gp0_words==3);
        assert(d->fills==1 && d->gpu_writes==1 && d->dma_writes==3);
    } else {
        assert(dma_state.starts==1 && dma_state.transfers==1 && dma_state.header_words==1 && !dma_state.accepted_words);
        assert(!d->gpu.accepted_gp0_words && !d->fills && !d->gpu_writes && !d->dma_writes);
    }
    assert(d->dma->interrupt==(!strcmp(mode,"draw-upload-other-irq") ? 0x00880000u : 0x00840000u));
    assert(musashi_gpu_dma2_close(d->channel));
}
