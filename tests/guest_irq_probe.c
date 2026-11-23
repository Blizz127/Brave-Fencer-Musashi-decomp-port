#ifdef NDEBUG
#undef NDEBUG
#endif
#include "musashi_guest_irq.h"
#include "musashi_irq_scheduler.h"
#include "musashi_spu_cd_audio.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* No window or source worker. Own only the single registration and mutex.
 * Entry beneath the BIOS kernel is modeled; CPU IRQ tokens, scheduler
 * ownership/return, live guest code and both controller owners are real. */
void (*vsync_callback)(void);
SDL_mutex *g_intrMutex;
int VSyncCallback(void (*callback)(void)) { vsync_callback = callback; return 0; }

#include "guest_irq_draw_completion.h"
#include "guest_irq_cd_dma3.h"

enum FadeClockPhase {
    STEP_READ = 1, LOAD_DELAY = 2, RISING_STORE = 4, CLAMP_LEVEL = 8, CLAMP_FLAGS = 16
};
enum FadeClockAction {
    NO_CHANGE, CHANGE_STEP, CHANGE_LIVE, CHANGE_BOTH_IMAGES, CHANGE_RETURN, REFUSE_CLOCK
};
typedef struct FadeCase {
    const char *name;
    uint16_t level, step, target, result;
    uint8_t flags, result_flags, dirty;
    unsigned phases;
    uint32_t clock_pc;
    enum FadeClockAction action;
} FadeCase;

/* Results come from the unsigned assignments and comparisons in 8002cdd8.c.
 * The rising route reads the step and stores the sum before a possible clamp. */
#define RISE_PHASES (STEP_READ | LOAD_DELAY | RISING_STORE)
#define CLAMP_PHASES (RISE_PHASES | CLAMP_LEVEL | CLAMP_FLAGS)
static const FadeCase fade_cases[] = {
    {"fade-za-rise",       0x2000,0x0222,0x4000,0x2222, 0x01,0x01,1,    RISE_PHASES},
    {"fade-rise-clamp",    0x3ffe,0x0222,0x4000,0x4000, 0x85,0x84,0x7e, CLAMP_PHASES},
    {"fade-rise-exact",    0x7fff,0x0001,0x8000,0x8000, 0x83,0x82,0x7e, CLAMP_PHASES},
    {"fade-rise-wrap",     0xfff0,0x0030,0xfff8,0x0020, 0x01,0x01,0,    RISE_PHASES},
    {"fade-equal-wrap",    0xffff,0x0001,0xffff,0x0000, 0xff,0xff,0,    RISE_PHASES},
    {"fade-rise-zero-step",0x0100,0x0000,0x0101,0x0100, 0x81,0x81,0,    RISE_PHASES},
    {"fade-disabled",      0x2000,0x0222,0x4000,0x2000, 0x80,0x80,0x7e, 0},
    {"fade-step-on-read",  0x2000,0x0222,0x4000,0x2011, 0x01,0x01,0,    RISE_PHASES,
        0x8002ce84u, CHANGE_STEP},
    {"fade-step-after-read",0x2000,0x0222,0x4000,0x2222,0x01,0x01,0,    RISE_PHASES,
        0x8002ce88u, CHANGE_STEP},
    {"fade-clock-store-live",0x2000,0x0222,0x4000,0x2000,0x01,0x01,0,    RISE_PHASES,
        0x8002ce94u, CHANGE_LIVE},
    {"fade-clock-clamp-images",0x3ffe,0x0222,0x4000,0x4220,0x85,0x85,0, RISE_PHASES | CLAMP_LEVEL,
        0x8002ceb0u, CHANGE_BOTH_IMAGES},
    {"fade-clock-flags-return",0x3ffe,0x0222,0x4000,0x4000,0x85,0x85,0,  CLAMP_PHASES,
        0x8002ceb8u, CHANGE_RETURN},
    {"fade-clock-step-refused",0x2000,0x0222,0x4000,0x2000,0x01,0x01,0,  STEP_READ,
        0x8002ce84u, REFUSE_CLOCK},
};

typedef struct Machine {
    DrawFixture draw;
    CdCompletion cd3;
    MusashiBootMemory memory;
    MusashiGuestIrqImage image;
    MusashiPsyCrossIrqScheduler scheduler;
    MusashiIrqController irq;
    MusashiDmaController dma;
    MusashiGuestIrqStop stop;
    MusashiExecutionClock clock;
    MusashiIrqDispatchDevice bus;
    MusashiSpuCdAudio *spu;
    MusashiCdSpuDevice spu_bus;
    int16_t pcm[2048*2];
    unsigned envelopes, queued_frames;
    uint32_t outer[32], cycles;
    uint32_t before, post;
    unsigned calls;
    const char *mode;
    const MusashiGuestIrqCard *card;
    const FadeCase *fade_case;
    unsigned fade_phases, clock_changes;
} Machine;

static int fade_clock(Machine *m, uint32_t cycles) {
    uint32_t pc = m->stop.pc;
    unsigned phase;
    switch (pc) {
    case 0x8002ce84u: phase = STEP_READ; break;
    case 0x8002ce88u: phase = LOAD_DELAY; break;
    case 0x8002ce94u: phase = RISING_STORE; break;
    case 0x8002ceb0u: phase = CLAMP_LEVEL; break;
    case 0x8002ceb8u: phase = CLAMP_FLAGS; break;
    default: return 1;
    }
    assert(cycles == (phase == LOAD_DELAY ? 2u : 3u));
    assert(!(m->fade_phases & phase));
    m->fade_phases |= phase;
    if (pc != m->fade_case->clock_pc) return 1;
    ++m->clock_changes;
    switch (m->fade_case->action) {
    case CHANGE_STEP:
        return musashi_boot_write16(&m->memory, 0x800a4ee2u, 0x11);
    case CHANGE_LIVE:
        m->memory.bytes[pc - 0x80000000u] ^= 1u;
        return 1;
    case CHANGE_BOTH_IMAGES:
        m->memory.bytes[pc - 0x80000000u] ^= 1u;
        m->image.sound_tick[pc - 0x8002cdd8u] ^= 1u;
        return 1;
    case CHANGE_RETURN:
        return musashi_boot_write32(&m->memory, 0x8006ca94u + 24u, 0);
    case REFUSE_CLOCK: return 0;
    default: abort();
    }
}

static uint64_t thread(void *p) { (void)p; return (uint64_t)SDL_ThreadID(); }
static int advance(void *p,uint32_t n) {
    Machine *m=p; m->cycles+=n;
    if (m->fade_case && !fade_clock(m,n)) return 0;
    if (m->draw.channel && !draw_advance(&m->draw,n)) return 0;
    if (m->cd3.channel && !musashi_cd_dma3_advance(m->cd3.channel,m->cycles)) return 0;
    return !m->spu || musashi_spu_cd_audio_advance(m->spu,m->cycles);
}
static int audio_ready(void *p) { return p!=NULL; }
static int audio_status(void *p,size_t *queued,int *playing) {
    if(!p)return 0;
    *queued=0; *playing=1; return 1;
}
static int audio_pcm(void *p,const int16_t *pcm,size_t n) {
    Machine *m=p; if(!pcm || n>2048) return 0;
    memcpy(m->pcm,pcm,n*2*sizeof(*pcm)); m->queued_frames+=(unsigned)n; return 1;
}
static int voice_read(void *p,uint32_t address,uint16_t *value) {
    Machine *m=p;
    if (!musashi_spu_cd_audio_read16(m->spu,address,value)) return 0;
    ++m->envelopes;
    return 1;
}
static int outer(void *p,const void *identity,MusashiCpuContext *out) {
    Machine *m=p;
    if (identity!=m->outer) return 0;
    *out=(MusashiCpuContext){.identity=identity,.gpr=m->outer,.pc=0x8004251cu,
        .npc=0x80042520u,.instruction_valid=1,.provenance=MUSASHI_CPU_CONTEXT_SOURCE};
    return 1;
}
static int kernel(void *p,uint16_t pending) {
    Machine *m=p;
    MusashiCpuStatusSnapshot s;
    assert(pending && musashi_cpu_status_inspect(m->scheduler.cpu_status,
        &m->scheduler.active_exception,&s));
    assert(s.active && s.cause_epc_valid && s.epc==0x8004251cu && !s.kernel_entered);
    return musashi_cpu_status_accept_kernel_entry(m->scheduler.cpu_status,&m->scheduler.active_exception);
}
static int unused(void *p,MusashiBootMemory *memory,uint32_t target) {
    (void)p; (void)memory; (void)target; abort();
}
static int guest(void *p,MusashiBootMemory *memory,uint32_t context,const uint32_t *saved,
                 const void *identity,const MusashiIrqDispatchDevice *io) {
    Machine *m=p;
    assert(identity==m->outer && identity==m->scheduler.active_continuation);
    ++m->calls;
    const MusashiGuestIrqImage *image=&m->image;
    static MusashiBootMemory other;
    if (!strcmp(m->mode,"owner")) memory=&other;
    MusashiGuestIrqImage clone;
    if (!strcmp(m->mode,"copy")) { clone=m->image;image=&clone; }
    MusashiGuestIrqServices services={.spu=m->spu ? &m->spu_bus : NULL,.card=m->card,
        .draw=m->draw.channel ? &m->draw.service : NULL, .cd=m->cd3.channel ? &m->cd3.cd : NULL,
        .cd_dma3=!strcmp(m->mode,"cd3-missing") ? NULL : m->cd3.channel};
    return musashi_boot_execute_guest_irq_with_services(image,memory,context,saved,io,
        &m->clock,&m->dma,&services,&m->stop);
}
static uint32_t word(MusashiBootMemory *m,uint32_t a) {
    uint32_t v; assert(musashi_boot_read32(m,a,&v)); return v;
}
static void put(MusashiBootMemory *m,uint32_t a,uint32_t v) { assert(musashi_boot_write32(m,a,v)); }

#include "guest_irq_card_request.h"

int main(int argc,char **argv) {
    if(argc<3 || argc>4) return 2;
    Machine *m=calloc(1,sizeof(*m)); assert(m);
    uint8_t *exe=malloc(0x65000u); assert(exe);
    FILE *f=fopen(argv[1],"rb"); assert(f && fread(exe,1,0x65000u,f)==0x65000u && fgetc(f)==EOF); fclose(f);
    assert(musashi_boot_map_irq_exe(&m->image,&m->memory,exe,0x65000u));
    m->mode=argv[2];
    int draw=!strncmp(m->mode,"draw-",5);
    int swap_gpu=!strcmp(m->mode,"draw-foreign-gpu");
    int swap_channel=!strcmp(m->mode,"draw-upload-foreign");
    int swap_timer=!strcmp(m->mode,"draw-foreign-timer");
    int stale=!strcmp(m->mode,"draw-stale"), unbound=!strcmp(m->mode,"draw-unbound");
    int replacement=!strcmp(m->mode,"draw-replacement");
    MusashiGuestIrqDrawHost replacement_host={0};
    Machine *other=NULL;
    uint64_t old_generation=0;
    if (draw) {
        assert(musashi_boot_bind_irq_draw(&m->image,&m->draw.host,&m->draw,draw_owns));
        assert(!musashi_boot_bind_irq_draw(&m->image,&m->draw.host,&m->draw,draw_owns));
        if (stale || unbound || replacement) {
            old_generation=m->draw.host.generation;
            assert(musashi_boot_map_irq_exe(&m->image,&m->memory,exe,0x65000u));
            assert(!m->image.draw_host && !m->image.owns_draw);
            if (stale) {
                assert(musashi_boot_bind_irq_draw(&m->image,&m->draw.host,&m->draw,draw_owns));
                assert(m->draw.host.generation!=old_generation);
            } else if (replacement) {
                assert(musashi_boot_bind_irq_draw(&m->image,&replacement_host,&m->draw,draw_owns));
                assert(replacement_host.generation==old_generation);
            }
        }
        if (swap_gpu || swap_timer || swap_channel) {
            other=calloc(1,sizeof(*other)); assert(other);
            assert(musashi_boot_map_irq_exe(&other->image,&other->memory,exe,0x65000u));
            assert(!musashi_boot_bind_irq_draw(&other->image,&m->draw.host,&m->draw,draw_owns));
            assert(musashi_boot_bind_irq_draw(&other->image,&other->draw.host,&other->draw,draw_owns));
            musashi_dma_controller_init(&other->dma,&other->irq);
            other->clock=(MusashiExecutionClock){other,advance};
            draw_begin(&other->draw,&other->memory,&other->dma,&other->clock,thread,
                       swap_channel ? "draw-upload" : "draw-complete");
        }
    }
    free(exe);
    int card_request = !strncmp(m->mode, "request-", 8);
    if (card_request && argc != 4) return 2;
    /* Optional private replay supplies data state only. Code stays in the
     * authenticated mapping; the public fixture supplies no game bytes. */
    if (argc==4 && !card_request) {
        f=fopen(argv[3],"rb"); assert(f && fseek(f,0x65000,SEEK_SET)==0);
        assert(fread(m->memory.bytes+0x65000,1,0x63000,f)==0x63000); fclose(f);
    } else {
        uint32_t record[12]={0x80042780u,0x8006cb14u,0x801fffc0u,0x8006bafcu,
            0,0,0,0,0,0,0,0x80074750u};
        for(unsigned i=0;i<12;i++) put(&m->memory,0x8006bb34u+4u*i,record[i]);
        assert(musashi_boot_write16(&m->memory,0x8006bafcu,1));
        assert(musashi_boot_write16(&m->memory,0x8006bb2cu,9));
        put(&m->memory,0x8006cb88u,0x1f801070u); put(&m->memory,0x8006cb8cu,0x1f801074u);
        put(&m->memory,0x8006cbc0u,0x1f8010f4u);
        put(&m->memory,0x8006bb00u,0x80042ce8u); put(&m->memory,0x8006bb0cu,0x80042e08u);
    }
    int equal = !strcmp(m->mode, "fade-equal");
    int wrap = !strcmp(m->mode, "fade-wrap");
    int limit = equal || !strcmp(m->mode, "fade-limit") || !strcmp(m->mode, "fade-limit-level-code") ||
        !strcmp(m->mode, "fade-limit-mode-code");
    for (size_t i = 0; i < sizeof fade_cases / sizeof fade_cases[0]; ++i)
        if (!strcmp(m->mode, fade_cases[i].name)) m->fade_case = &fade_cases[i];
    const FadeCase *fc = m->fade_case;
    int fade = limit || wrap || !strcmp(m->mode, "title-fade") || !strcmp(m->mode, "title-fade-code");
    int countdown=!strncmp(m->mode,"countdown-",10);
    int bad_countdown=!strcmp(m->mode,"countdown-code");
    uint16_t count=13;
    if (countdown) {
        if (!strcmp(m->mode,"countdown-zero")) count=0;
        if (!strcmp(m->mode,"countdown-final")) count=1;
        if (!strcmp(m->mode,"countdown-wrap")) count=0x8000;
        assert(musashi_boot_write16(&m->memory,0x800a4efau,0x4517));
        assert(musashi_boot_write16(&m->memory,0x800a4efcu,count));
        if (bad_countdown) m->memory.bytes[0x2d008u]^=1u;
    }
    int active=countdown || fade || fc || !strcmp(m->mode,"active") || !strcmp(m->mode,"soundless") ||
        !strcmp(m->mode,"voice-range") || !strcmp(m->mode,"sound-code");
    if (fade) {
        uint16_t start = limit ? 0x23aau : 0x3180u;
        if (equal) start = 0x2489u;
        if (wrap) start = 0x80u;
        assert(musashi_boot_write16(&m->memory, 0x800a4ee0u, start));
        assert(musashi_boot_write16(&m->memory, 0x800a4ee2u, 0x0108u));
        assert(musashi_boot_write16(&m->memory, 0x800a4ee4u, wrap ? 0u : 0x2381u));
        m->memory.bytes[0xa4ee6u] = 5u;
        m->memory.bytes[0xa4f16u] = 0u;
        if (!strcmp(m->mode, "fade-limit-level-code")) m->memory.bytes[0x2ceb0u] ^= 1u;
        if (!strcmp(m->mode, "fade-limit-mode-code")) m->memory.bytes[0x2ceb8u] ^= 1u;
        if (!strcmp(m->mode, "title-fade-code")) m->memory.bytes[0x2ce40u] ^= 1u;
    }
    if (fc) {
        assert(musashi_boot_write16(&m->memory, 0x800a4ee0u, fc->level));
        assert(musashi_boot_write16(&m->memory, 0x800a4ee2u, fc->step));
        assert(musashi_boot_write16(&m->memory, 0x800a4ee4u, fc->target));
        m->memory.bytes[0xa4ee6u] = fc->flags;
        m->memory.bytes[0xa4ee7u] = 0x6b;
        m->memory.bytes[0xa4f16u] = fc->dirty;
    }
    int card_only=!strcmp(m->mode,"card") || !strcmp(m->mode,"card-code");
    if (argc!=4 || !active) {
        for(unsigned i=0;i<8;i++) put(&m->memory,0x8006cb98u+4u*i,0);
        if(active)put(&m->memory,0x8006cba8u,0x8001096cu);
        if(active||card_only)put(&m->memory,0x8006cbb4u,0x800616d0u);
    }
    if(argc!=4 && active)
        for (unsigned i=0; i<8; ++i)
            assert(musashi_boot_write16(&m->memory,0x800a4992u+i*0x54u,(uint16_t)(16+i)));
    if(argc!=4 && (active||card_only))put(&m->memory,0x80072a34u,UINT32_MAX);
    if(active && strcmp(m->mode,"soundless")) {
        MusashiSpuCdAudioBackend sink={m,audio_ready,thread,audio_status,audio_pcm};
        m->spu=musashi_spu_cd_audio_create();
        assert(m->spu && musashi_spu_cd_audio_init_bios_muted(m->spu,&sink,0));
        m->spu_bus=(MusashiCdSpuDevice){.userdata=m,.read16=voice_read};
    }
    for(unsigned i=0;i<7;i++) put(&m->memory,0x8006cbc4u+4u*i,0);
    for(unsigned i=1;i<32;i++)m->outer[i]=0x1230000u+i;
    uint32_t initial[32]; memcpy(initial,m->outer,sizeof(initial));
    MusashiPsyCrossIrqScheduler *s=&m->scheduler;
    g_intrMutex=SDL_CreateMutex();assert(g_intrMutex);
    s->owner_thread=thread(NULL); s->memory=&m->memory;
    m->bus=(MusashiIrqDispatchDevice){.userdata=&m->irq,.read16=musashi_irq_controller_read16,
        .write16=musashi_irq_controller_write16,.execute=unused};
    s->device=&m->bus; s->raise_vblank=musashi_irq_controller_raise_vblank; s->raise_userdata=&m->irq;
    m->clock=(MusashiExecutionClock){m,advance}; musashi_dma_controller_init(&m->dma,&m->irq);
    assert(musashi_cpu_status_open(&s->cpu_status,thread,NULL));
    assert(musashi_psycross_irq_scheduler_bind_context(s,outer,m));
    assert(musashi_psycross_irq_scheduler_bind_guest(s,guest,m));
    s->before_dispatch=kernel; s->before_dispatch_userdata=m;
    assert(musashi_psycross_irq_scheduler_hook_entry_int(s,0x8006bb34u));
    assert(musashi_psycross_irq_scheduler_set_enabled(s,1));
    if (card_request) {
        test_card_requests(m, argv[3]);
        assert(musashi_psycross_irq_scheduler_uninstall(s));
        assert(musashi_cpu_status_close(s->cpu_status));
        SDL_DestroyMutex(g_intrMutex);
        free(m);
        return 0;
    }
    if (draw) {
        draw_begin(&m->draw,&m->memory,&m->dma,&m->clock,thread,m->mode);
        if (swap_channel) m->draw.service.channel=other->draw.channel;
        if (swap_gpu) m->draw.service.gpu=&other->draw.gpu_io;
        if (swap_timer) m->draw.service.timer=&other->draw;
        if (stale) m->draw.service.generation=old_generation;
    }
    int cd3=!strncmp(m->mode,"cd3-",4);
    if (cd3) cd_test_begin(&m->cd3,&m->memory,&m->dma,m->mode,thread,&m->cycles);
    int vblank=!draw && !cd3 && strcmp(m->mode,"dma")!=0, dma=!strcmp(m->mode,"dma")||!strcmp(m->mode,"both");
    assert(musashi_irq_controller_write16(&m->irq,0x1f801074u,9));
    if(vblank)assert(musashi_irq_controller_raise_vblank(&m->irq));
    if(dma) {
       assert(musashi_dma_controller_write32(&m->dma,0x1f8010f4u,0x00840000u));
       assert(musashi_dma_controller_complete(&m->dma,2));
       assert(m->dma.interrupt==0x84840000u);
    }
    int success=(countdown && !bad_countdown) || equal || wrap || !strcmp(m->mode,"fade-limit") || !strcmp(m->mode,"title-fade") || !strcmp(m->mode,"vblank") || dma || !strcmp(m->mode,"card") || !strcmp(m->mode,"active");
    if (fc) success = fc->action == NO_CHANGE || fc->action == CHANGE_STEP;
    if (cd3) success=!strcmp(m->mode,"cd3-complete") || !strcmp(m->mode,"cd3-next");
    if (draw) success=m->draw.queued ? m->draw.queued->complete :
                    !strcmp(m->mode,"draw-complete") || !strcmp(m->mode,"draw-timer") ||
                    !strcmp(m->mode,"draw-frame-zero") || !strcmp(m->mode,"draw-upload");
    if (!strcmp(m->mode,"wrongslot")) {
        m->irq.status=2u;
        assert(musashi_irq_controller_write16(&m->irq,0x1f801074u,2u));
        assert(musashi_boot_write16(&m->memory,0x8006bb2cu,2u));
        put(&m->memory,0x8006bb04u,0x80042ce8u);
    }
    if(!strcmp(m->mode,"code"))put(&m->memory,0x8005cedcu,word(&m->memory,0x8005cedcu)^1u);
    if(!strcmp(m->mode,"record"))put(&m->memory,0x8006bb3cu,word(&m->memory,0x8006bb3cu)^4u);
    if(!strcmp(m->mode,"callback"))put(&m->memory,0x8006cba8u,0x80042ce8u);
    if(!strcmp(m->mode,"sound-code"))put(&m->memory,0x8003d2d8u,word(&m->memory,0x8003d2d8u)^1u);
    if(!strcmp(m->mode,"card-code"))put(&m->memory,0x80062990u,word(&m->memory,0x80062990u)^1u);
    if(!strcmp(m->mode,"voice-range"))assert(musashi_boot_write16(&m->memory,0x800a4992u,24u));
    if(!strcmp(m->mode,"slotswap"))put(&m->memory,0x8006cba8u,0x800616d0u);
    m->before=word(&m->memory,0x8006cbb8u);
    int completed=musashi_psycross_irq_scheduler_dispatch_pending(s,m->outer);
    if(completed!=success) fprintf(stderr,"%s unexpected result=%d stop=%08x steps=%u cycles=%u voices=%u\n",
        m->mode,completed,m->stop.pc,m->stop.instructions,m->cycles,m->envelopes);
    assert(completed==success);
    m->post=word(&m->memory,0x8006cbb8u);
    assert(m->calls==1 && memcmp(initial,m->outer,sizeof(initial))==0);
    MusashiCpuStatusSnapshot state;assert(musashi_cpu_status_snapshot(s->cpu_status,&state));
    if (countdown) {
        uint16_t actual,neighbor;
        assert(musashi_boot_read16(&m->memory,0x800a4efcu,&actual));
        assert(musashi_boot_read16(&m->memory,0x800a4efau,&neighbor));
        assert(actual==(count && !bad_countdown ? (uint16_t)(count-1u) : count));
        assert(neighbor==0x4517);
    }
    if (fc) {
        uint16_t level, step, target;
        assert(musashi_boot_read16(&m->memory, 0x800a4ee0u, &level));
        assert(musashi_boot_read16(&m->memory, 0x800a4ee2u, &step));
        assert(musashi_boot_read16(&m->memory, 0x800a4ee4u, &target));
        assert(level == fc->result && target == fc->target);
        assert(step == (fc->action == CHANGE_STEP ? 0x11u : fc->step));
        assert(m->memory.bytes[0xa4ee6u] == fc->result_flags);
        assert(m->memory.bytes[0xa4ee7u] == 0x6b);
        assert(m->memory.bytes[0xa4f16u] == ((fc->flags & 1u) ? 1u : fc->dirty));
        assert(m->fade_phases == fc->phases && m->clock_changes == (fc->action != NO_CHANGE));
    }
    if (fade) {
        uint16_t level;
        assert(musashi_boot_read16(&m->memory, 0x800a4ee0u, &level));
        if (limit) {
            assert(level == (!strcmp(m->mode,"fade-limit-level-code") ? 0x22a2u : 0x2381u));
            assert(m->memory.bytes[0xa4ee6u] == (success ? 4u : 5u));
            assert(m->memory.bytes[0xa4f16u] == 1u);
        } else if (wrap) {
            assert(level == 0xff78u && m->memory.bytes[0xa4ee6u] == 5u);
            assert(m->memory.bytes[0xa4f16u] == 1u);
        } else {
            assert(level == (success ? 0x3078u : 0x3180u));
            assert(m->memory.bytes[0xa4f16u] == (unsigned)success);
        }
    }
    if(success) {
        assert(s->last_result==MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && state.sequence==1 &&
               !state.active && !state.faulted && state.sr==0x40000401u && m->stop.pc==0xb0u);
        assert(m->post==m->before+(unsigned)vblank && m->irq.status==0);
        if(dma)assert(m->dma.interrupt==0x00840000u);
        assert(musashi_psycross_irq_scheduler_dispatch_pending(s,m->outer) && m->calls==1);
    } else {
        assert(state.active && state.faulted && s->faulted && !s->return_consumed);
        unsigned steps=m->stop.instructions;
        assert(!musashi_psycross_irq_scheduler_dispatch_pending(s,m->outer) && steps==m->stop.instructions && m->calls==1);
        if (fc) assert(m->post == m->before + 1 && m->stop.pc == fc->clock_pc);
        else if (bad_countdown) assert(m->post==m->before+1 && m->stop.pc==0x8002d008u);
        else if(cd3) assert(m->post==m->before && (m->stop.pc==0x80042ea8u ||
            (!strcmp(m->mode,"cd3-image") && m->stop.pc==CD_TEST_CALLBACK)));
        else if(draw) assert(m->post==m->before);
        else if(!strcmp(m->mode,"code")||!strcmp(m->mode,"callback"))assert(m->post==m->before+1);
        else if(!strcmp(m->mode,"voice-range")) assert(m->post==m->before+1 && m->stop.pc==0x80031454u);
        else if(!strcmp(m->mode,"slotswap")) assert(m->post==m->before+1 && m->stop.pc==0x80042d30u);
        else if(!strcmp(m->mode,"card-code")) assert(m->post==m->before+1 && m->stop.pc==0x80062990u);
        else if(limit) assert(m->post==m->before+1 && m->stop.pc==
            (!strcmp(m->mode,"fade-limit-level-code") ? 0x8002ceb0u : 0x8002ceb8u));
        else if(!strcmp(m->mode,"title-fade-code")) assert(m->post==m->before+1 && m->stop.pc==0x8002ce40u);
        else if(!strcmp(m->mode,"soundless")||!strcmp(m->mode,"sound-code")) assert(m->post==m->before+1 && m->stop.pc==0x8003d2d8u);
        else if(!strcmp(m->mode,"wrongslot"))assert(m->post==m->before && steps>0 && m->stop.pc==0x800428d4u);
        else assert(m->post==m->before && steps==0);
    }
    if (cd3) cd_test_check(&m->cd3,&m->memory,&m->dma,m->mode,success);
    if (draw) draw_check(&m->draw,&m->memory,m->stop.pc,m->mode,success);
    if (other) {
        assert(!other->draw.chcr_reads && !other->draw.status_reads && !other->draw.timer_reads);
        assert(other->irq.status==8u && other->dma.interrupt==0x84840000u);
        assert(musashi_gpu_dma2_close(other->draw.channel));
        free(other);
    }
    if(m->spu)assert(m->envelopes==((!strcmp(m->mode,"sound-code") || !strcmp(m->mode,"title-fade-code") || ((limit || fc) && !success))?0u:24u) && musashi_spu_cd_audio_destroy(m->spu));
    printf("%s complete=%d steps=%u cycles=%u stop=%08x tick_delta=%u I_STAT=%04x DICR=%08x sticky=%d voice_reads=%u\n",
        m->mode,success,m->stop.instructions,m->cycles,m->stop.pc,m->post-m->before,m->irq.status,m->dma.interrupt,s->faulted,m->envelopes);
    assert(musashi_psycross_irq_scheduler_uninstall(s));assert(musashi_cpu_status_close(s->cpu_status));
    SDL_DestroyMutex(g_intrMutex);free(m);return 0;
}
