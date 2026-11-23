/* The title mixer calls the resident sound interface on its interrupt stack.
 * Fetch each guest word from the same mounted program and keep the existing
 * formatter, device ownership and per-instruction clock. */
typedef struct { uint32_t start, end; unsigned family; } TitleMixerSymbol;
enum { TITLE_VOICE_ATTR=1, TITLE_REVERB_MASK, TITLE_KEYS, MUSIC_SEQUENCE, MUSIC_RELEASE, MUSIC_REQUEST };
static const TitleMixerSymbol title_mixer_symbols[] = {
    {0x8003b250u,0x8003b280u,TITLE_VOICE_ATTR},
    {0x8003be74u,0x8003be98u,TITLE_REVERB_MASK},
    {0x8003c23cu,0x8003c438u,TITLE_KEYS},
    {0x8003af4cu,0x8003aff0u,TITLE_VOICE_ATTR},
    {0x8003b0e4u,0x8003b14cu,TITLE_VOICE_ATTR},
    {0x8003c92cu,0x8003cf68u,TITLE_VOICE_ATTR},
    {0x8003be98u,0x8003c0a0u,TITLE_REVERB_MASK},
    {0x8003d3b4u,0x8003d3f8u,MUSIC_RELEASE},
    {0x8003b45cu,0x8003b954u,MUSIC_REQUEST},
    {0x8003b0e4u,0x8003b14cu,MUSIC_RELEASE}
};
typedef struct TitleMixerRun {
    const MusashiGuestIrqImage *image;
    const MusashiMain12Owner *owner;
    const MusashiCdSpuDevice *spu;
    const MusashiExecutionClock *clock;
    void *spu_user, *clock_user;
    int (*spu_read)(void *,uint32_t,uint16_t *);
    int (*spu_write)(void *,uint32_t,uint16_t);
    int (*advance)(void *,uint32_t);
    int (*reverb_mask)(const MusashiCdSpuDevice *,uint32_t *);
    const TitleMixerSymbol *symbol;
    uint32_t root_sp, frame_sp, voices, record, direction, mask_cell, emitted_key;
    uint32_t issued_pc, issued_word, issued_npc, issued_branch, issued_hi, issued_lo;
    uint32_t issued_gpr[32];
    unsigned issued_delay;
    uint16_t reverb[2];
    unsigned reverb_seen, key_seen, gain_written;
    uint16_t gain[2];
    int music;
} TitleMixerRun;

#include "opening_music_service.h"

static int title_service_ancestry(const IrqGuestReturn *stack, unsigned depth) {
    return stack && depth==6u && stack[0].callee==IRQ_DISPATCH &&
        stack[1].callee==IRQ_VBLANK && stack[2].callee==IRQ_GAME_VBLANK &&
        stack[3].callee==IRQ_SOUND_TICK && stack[4].callee==IRQ_CHANNEL_TICK &&
        stack[5].callee==IRQ_MIX_UPDATE && stack[5].address==0x8002d01cu &&
        stack[5].sp==0x8006ca74u;
}

static const TitleMixerSymbol *title_mixer_symbol(uint32_t pc, unsigned family) {
    if (family == MUSIC_SEQUENCE) return music_game_symbol(pc);
    for (size_t i=0;i<sizeof title_mixer_symbols/sizeof title_mixer_symbols[0];++i) {
        const TitleMixerSymbol *s=&title_mixer_symbols[i];
        if (s->family==family && pc>=s->start && pc<s->end) return s;
    }
    return NULL;
}

static int title_mixer_bound(MusashiBootMemory *m,const FormatterCpu *cpu) {
    const TitleMixerRun *r=cpu->title_mixer;
    uint32_t value;
    if (!r || !r->image || r->image!=cpu->irq_image || r->image->identity!=r->image ||
        r->image->memory!=m || !r->owner || r->owner!=r->image->main12_owner ||
        r->owner->image!=r->image || r->owner->memory!=m || !r->owner->exe_text ||
        !r->owner->resident_matches || !r->spu || r->spu!=cpu->cd_spu_device ||
        !r->spu_user || r->spu->userdata!=r->spu_user ||
        !r->spu_read || !r->spu_write || r->spu->read16!=r->spu_read || r->spu->write16!=r->spu_write ||
        !r->clock || r->clock!=cpu->clock || !r->advance || r->clock->advance!=r->advance ||
        r->clock->userdata!=r->clock_user) return 0;
    if (r->music && r->symbol && r->symbol->family == MUSIC_SEQUENCE)
        return r->root_sp == 0x8006ca94u;
    if (r->music && r->symbol && r->symbol->family == MUSIC_REQUEST)
        return r->root_sp == 0x8006ca94u && music_data_equals(m,0x800a4eccu,4,6) &&
            music_data_equals(m,0x8006b54cu,4,0x1f801c00u);
    if (r->music) {
        if (!r->voices || (r->voices & ~0xffffu)) return 0;
        if (r->record) {
            unsigned voice = (r->record - 0x800c6de0u) / 0x60u;
            if (voice >= 16u || r->record != 0x800c6de0u + 0x60u*voice ||
                r->voices != (1u << voice)) return 0;
            const uint8_t *flags = musashi_boot_ram_span(m,r->record+0x4au,5u);
            return flags && flags[0] && (r->symbol->family == TITLE_VOICE_ATTR ?
                flags[3] : r->symbol->family == MUSIC_RELEASE && !flags[3] && flags[4]);
        }
        return musashi_boot_read32(m, r->mask_cell, &value) && (value & 0xffffu) == r->voices;
    }
    if (!r->voices || (r->voices & ~0x00ff0000u)) return 0;
    if (r->record) {
        const uint8_t *flag=musashi_boot_ram_span(m,r->record+0x44u,1u);
        return flag && !*flag && musashi_boot_read32(m,r->record+0x40u,&value) &&
            value>=16u && value<24u && (1u<<value)==r->voices;
    }
    return musashi_boot_read32(m,r->mask_cell,&value) && (value&0x00ff0000u)==r->voices;
}

static int title_mixer_word(MusashiBootMemory *m,const FormatterCpu *cpu,uint32_t *out) {
    const TitleMixerRun *r=cpu->title_mixer;
    uint32_t shape;
    return title_mixer_bound(m,cpu) && r->symbol && !(cpu->pc&3u) &&
        cpu->pc>=r->symbol->start && cpu->pc<r->symbol->end && formatter_admitted(cpu,&shape) &&
        r->owner->resident_matches(r->owner,r->symbol->start,r->symbol->end-r->symbol->start) &&
        musashi_boot_read32(m,cpu->pc,out);
}

/* Called only for accepted guest halfword transfers. Shadow the guest's
 * actual read until the matching EON store, and count successful key strobes. */
static void title_mixer_spu_result(const FormatterCpu *cpu,uint32_t addr,uint16_t value,int store) {
    TitleMixerRun *r=cpu->title_mixer;
    if (!r) return;
    if (r->symbol->family == MUSIC_REQUEST && store && (addr == 0x1f801d84u || addr == 0x1f801d86u)) {
        unsigned side = (addr-0x1f801d84u)/2u;
        r->gain_written |= 1u << side;
        r->gain[side] = value;
    } else if (addr==0x1f801d98u || addr==0x1f801d9au) {
        unsigned i=(addr-0x1f801d98u)/2u;
        if (store) r->reverb_seen&=~(1u<<i);
        else {r->reverb[i]=value;r->reverb_seen|=1u<<i;}
    } else if (store && r->symbol->family==TITLE_KEYS) {
        uint32_t base=r->direction ? 0x1f801d88u : 0x1f801d8cu;
        if (addr==base || addr==base+2u) {
            unsigned i=(addr-base)/2u;
            r->key_seen|=1u<<i;
            r->emitted_key|=(uint32_t)value<<(16u*i);
        }
    }
}

static int title_mixer_access(MusashiBootMemory *m,const FormatterCpu *cpu,
                              uint32_t addr,unsigned width,unsigned store,uint32_t value) {
    const TitleMixerRun *r=cpu->title_mixer;
    if (!title_mixer_bound(m,cpu) || !r->symbol || r->issued_pc!=cpu->pc ||
        (width!=1u && width!=2u && width!=4u) || (addr&(width-1u)) || store>1u) return 0;
    if (r->music && r->symbol->family == MUSIC_SEQUENCE) {
        if (r->symbol->start == 0x80039b20u &&
            music_note_release_status(m,cpu->pc,addr,width,store,value)) return 1;
        if (music_data_access(m, r->root_sp, cpu->r[29], addr, width, store, value))
            return !store || addr < cpu->r[29] || addr >= r->root_sp ||
                (addr < r->frame_sp && width <= r->frame_sp - addr);
        if (!store && width == 2u && addr >= 0x8006aef8u && addr < 0x8006b008u)
            return r->owner->resident_matches(r->owner,addr & ~3u,4u);
        uint32_t target;
        return !store && width == 4u && addr >= CODE_IMAGE_BASE && addr <= CODE_IMAGE_END-4u &&
            r->owner->resident_matches(r->owner,addr,4u) && musashi_boot_read32(m,addr,&target) &&
            (music_game_symbol(target) == r->symbol || music_callee(r->symbol->start,0,target));
    }
    if (addr>=cpu->r[29] && addr<r->root_sp && width<=r->root_sp-addr)
        return cpu->r[29]<=r->root_sp && r->root_sp-cpu->r[29]<=(r->music ? 0x400u : 0x70u) &&
            (!store || (addr<r->frame_sp && width<=r->frame_sp-addr));
    if (addr>=0x8006c000u && addr<0x8006cc00u) return 0; /* other interrupt frames */
    if (r->music && r->symbol->family == MUSIC_REQUEST) {
        if (addr >= 0x800a4eccu && addr < 0x800a4ed8u)
            return !store && width <= 0x800a4ed8u-addr;
        if (addr == 0x1f801d84u || addr == 0x1f801d86u)
            return width == 2u && (!store || music_data_equals(m,0x800a4ed4u+addr-0x1f801d84u,2,value));
        if (addr == 0x8006b100u || addr == 0x8006b102u) {
            unsigned side = (addr-0x8006b100u)/2u;
            return width == 2u && store && (r->gain_written & (1u << side)) &&
                value == r->gain[side] && music_data_equals(m,0x800a4ed4u+2u*side,2,value);
        }
        return !store && width == 4u && addr == 0x8006b54cu;
    }
    if (r->record && addr>=r->record && addr-r->record<=0x40u-width) return !store;
    if (addr==0x8006b0e4u) { /* game-visible active voices */
        uint32_t previous;
        if (width!=4u || r->symbol->family!=TITLE_KEYS) return 0;
        if (!store) return 1;
        if (r->key_seen!=3u || r->emitted_key!=r->voices ||
            !musashi_boot_read32(m,addr,&previous)) return 0;
        return value==(r->direction ? previous|r->voices : previous&~r->voices);
    }
    if (!store && width==4u && addr>=CODE_IMAGE_BASE && addr<=CODE_IMAGE_END-4u)
        return r->owner->resident_matches(r->owner,addr,4u);
    if (width!=2u) return 0;
    if (r->symbol->family == MUSIC_RELEASE) {
        if (addr < 0x1f801c00u || addr >= 0x1f801d00u) return 0;
        uint32_t offset = addr-0x1f801c00u;
        return (offset%16u == 8u || offset%16u == 10u) && (r->voices & (1u << (offset/16u)));
    }
    if (r->symbol->family==TITLE_VOICE_ATTR) {
        /* Each selected hardware voice owns its volume, pitch, start address
         * and ADSR controls. Envelope readback and repeat remain separate. */
        if (!store || addr<0x1f801c00u || addr>=0x1f801d80u) return 0;
        uint32_t offset=addr-0x1f801c00u;
        return (offset%16u)<=10u && (r->voices&(1u<<(offset/16u)))!=0;
    }
    if (r->symbol->family==TITLE_KEYS) {
        uint32_t base=r->direction ? 0x1f801d88u : 0x1f801d8cu;
        if (!store || (addr!=base && addr!=base+2u)) return 0;
        unsigned i=(addr-base)/2u;
        return !(r->key_seen&(1u<<i)) && value==((r->voices>>(16u*i))&0xffffu);
    }
    if (addr!=0x1f801d98u && addr!=0x1f801d9au) return 0;
    if (!store) return 1;
    unsigned i=(addr-0x1f801d98u)/2u;
    uint32_t eon;
    if (!(r->reverb_seen&(1u<<i)) || !r->reverb_mask || !r->reverb_mask(r->spu,&eon) ||
        cpu->title_mixer!=r || !title_mixer_bound(m,cpu) || r->issued_pc!=cpu->pc) return 0;
    uint16_t change=(uint16_t)(r->voices>>(16u*i));
    uint16_t before=(uint16_t)(eon>>(16u*i));
    return r->reverb[i]==before && value==(r->direction ? (uint16_t)(before|change) :
                                                          (uint16_t)(before & (uint16_t)~change));
}

/* The formatter executes all guest arithmetic/control flow. Check the
 * addresses of ordinary integer accesses at issue, after advancing the
 * device clock, and where the real RAM or MMIO access takes place. */
static int title_mixer_issue(MusashiBootMemory *m,FormatterCpu *cpu,uint32_t word) {
    unsigned op=word>>26,fn=word&63u;
    if (!op) return fn==0u || fn==2u || fn==3u || fn==4u || fn==6u || fn==7u ||
        fn==8u || fn==9u || fn==16u || fn==18u || (fn>=24u && fn<=27u) ||
        (fn>=32u && fn<=39u) || fn==42u || fn==43u;
    if (op==1u) return ((word>>16)&31u)<=1u;
    if (op>=2u && op<=15u) return 1;
    unsigned width;
    switch (op) {
    case 32: case 36: case 40: width=1u; break;
    case 33: case 37: case 41: width=2u; break;
    case 35: case 43: width=4u; break;
    default: return 0;
    }
    uint32_t address=cpu->r[(word>>21)&31u]+(uint32_t)(int32_t)(int16_t)word;
    uint32_t value=cpu->r[(word>>16)&31u];
    if (width==1u) value&=0xffu;
    else if (width==2u) value&=0xffffu;
    return title_mixer_access(m,cpu,address,width,op>=40u,value);
}

static int title_mixer_after_clock(MusashiBootMemory *m,FormatterCpu *cpu,uint32_t word) {
    uint32_t live;
    const TitleMixerRun *r=cpu->title_mixer;
    return r && r->issued_pc==cpu->pc && r->issued_word==word &&
        r->issued_npc==cpu->npc && r->issued_branch==cpu->branch_pc && r->issued_delay==cpu->delay_slot &&
        r->issued_hi==cpu->hi && r->issued_lo==cpu->lo && !memcmp(r->issued_gpr,cpu->r,sizeof r->issued_gpr) &&
        title_mixer_word(m,cpu,&live) && live==word && title_mixer_issue(m,cpu,word);
}

static int title_mixer_step(MusashiBootMemory *m,FormatterCpu *cpu,MusashiGuestIrqStop *stop,
                            uint32_t word) {
    TitleMixerRun *r=cpu->title_mixer;
    r->issued_pc=stop->pc=cpu->pc;
    r->issued_word=word;r->issued_npc=cpu->npc;r->issued_branch=cpu->branch_pc;
    r->issued_delay=cpu->delay_slot;r->issued_hi=cpu->hi;r->issued_lo=cpu->lo;
    memcpy(r->issued_gpr,cpu->r,sizeof r->issued_gpr);
    if (stop->instructions>=8192u || !title_mixer_issue(m,cpu,word) || !formatter_step(m,cpu)) return 0;
    ++stop->instructions;
    return !(cpu->r[29]&3u) && cpu->r[29]<=r->root_sp && r->root_sp-cpu->r[29]<=(r->music ? 0x400u : 0x70u);
}

#include "opening_player_status.h"

static int title_mixer_guest_call(MusashiBootMemory *m,FormatterCpu *cpu,
    const MusashiGuestIrqServices *services,const MusashiExecutionClock *clock,
    const IrqGuestReturn *ancestors,unsigned depth,uint32_t caller,uint32_t entry,
    MusashiGuestIrqStop *stop) {
    int music = m && cpu && music_root_call(m, cpu, ancestors, depth, caller, entry, 1, 1);
    if (!m || !cpu || !stop || !services || !services->spu || !clock ||
        (!music && !title_service_ancestry(ancestors,depth)) ||
        !cpu->irq_image || cpu->title_mixer || cpu->delay_slot || cpu->branch_pc ||
        cpu->pc!=entry || cpu->npc!=entry+4u || cpu->merge_pending || cpu->gte_load_pending ||
        cpu->dma_registration || cpu->main12_row_pc || cpu->movie_output ||
        (!music && !irq_title_mix_entry(cpu,m,caller,entry,1))) return 0;
    unsigned family;
    if (music) family = MUSIC_SEQUENCE;
    else if (entry==title_mixer_symbols[0].start) family=TITLE_VOICE_ATTR;
    else if (entry==title_mixer_symbols[1].start) family=TITLE_REVERB_MASK;
    else if (entry==title_mixer_symbols[2].start) family=TITLE_KEYS;
    else return 0;
    TitleMixerRun run={.image=cpu->irq_image,.owner=cpu->irq_image->main12_owner,
        .spu=services->spu,.clock=clock,.reverb_mask=services->spu_reverb_mask,
        .spu_user=services->spu->userdata,.spu_read=services->spu->read16,.spu_write=services->spu->write16,
        .clock_user=clock->userdata,.advance=clock->advance,
        .symbol=title_mixer_symbol(entry,family),.root_sp=cpu->r[29],.frame_sp=cpu->r[29],.music=music};
    if (family==TITLE_VOICE_ATTR) {
        if (cpu->r[4]<16u || cpu->r[4]>=24u) return 0;
        run.voices=1u<<cpu->r[4];run.record=cpu->r[5];
    } else if (!music) {
        run.voices=cpu->r[5];run.direction=cpu->r[4];
        if (run.direction>1u) return 0;
        if (family==TITLE_KEYS) run.mask_cell=run.direction ? 0x800c7d20u : 0x800a2b98u;
        else run.mask_cell=run.direction ? 0x800c7d2cu : 0x800a2ba0u;
    }
    struct {uint32_t ret,sp;const TitleMixerSymbol *symbol;} frames[16]={{caller+8u,run.root_sp,run.symbol}};
    uint32_t saved[8],gp=cpu->r[28],fp=cpu->r[30];
    memcpy(saved,&cpu->r[16],sizeof saved);
    unsigned n=1,table_reg=0;
    uint32_t table_address=0,table_value=0;
    cpu->title_mixer=&run;
    if (music && entry == MUSIC_STATUS_START && music_status_image(m, cpu)) {
        music_observe_entry(run.symbol, caller);
        if (music_status_native(m, cpu, &run, stop))
            n = 0;
        else {
            cpu->title_mixer = NULL;
            return 0;
        }
    }
    while (n) {
        run.symbol=frames[n-1].symbol;
        family=run.symbol->family;
        run.frame_sp=frames[n-1].sp;
        uint32_t pc=stop->pc=cpu->pc,w;
        if (cpu->delay_slot || cpu->npc!=pc+4u || !title_mixer_word(m,cpu,&w)) break;
        unsigned op=w>>26,fn=w&63u,rs=(w>>21)&31u,rt=(w>>16)&31u;
        if (music && pc == run.symbol->start)
            music_observe_entry(run.symbol, frames[n-1].ret - 8u);
        int indirect=!op && fn==8u, ret=indirect && rs==31u;
        int link=op==3u || (!op && fn==9u);
        int branch=(op>=1u && op<=7u) || indirect || link;
        if (!op && fn==9u && (!music || family!=MUSIC_SEQUENCE || ((w>>11)&31u)!=31u)) break;
        uint32_t dest=0,sp=cpu->r[29],load_cell=0,loaded=0;
        const TitleMixerSymbol *child=NULL;
        if (link || indirect) {
            dest=op ? ((pc+4u)&0xf0000000u)|((w&0x03ffffffu)<<2) : cpu->r[rs];
            if (link) {
                if (!op && (rs!=table_reg || !table_address || table_value!=dest ||
                    !run.owner->resident_matches(run.owner, table_address, 4u) ||
                    !cd_irq_ram_word(m, table_address, dest))) break;
                child=title_mixer_symbol(dest,family);
                if (music && family==MUSIC_SEQUENCE) child=music_callee(run.symbol->start,pc,dest);
                if (!child || child->start!=dest || n>=(music ? 16u : 8u)) break;
            } else if (ret) {
                if (dest!=frames[n-1].ret) break;
            } else {
                uint32_t current;
                if (rs!=table_reg || !table_address || dest!=table_value ||
                    title_mixer_symbol(dest,family)!=run.symbol ||
                    !run.owner->resident_matches(run.owner,table_address,4u) ||
                    !musashi_boot_read32(m,table_address,&current) || current!=dest) break;
            }
        } else if (op==35u && rt) {
            uint32_t a=cpu->r[rs]+(uint32_t)(int32_t)(int16_t)w;
            if (!(a&3u) && a>=CODE_IMAGE_BASE && a<=CODE_IMAGE_END-4u &&
                run.owner->resident_matches(run.owner,a,4u) && musashi_boot_read32(m,a,&loaded) &&
                (title_mixer_symbol(loaded,family)==run.symbol ||
                 (music && family==MUSIC_SEQUENCE && music_callee(run.symbol->start,0,loaded)))) load_cell=a;
        }
        if (branch && stop->instructions>=8191u) break;
        if (!title_mixer_step(m,cpu,stop,w)) break;
        if (load_cell && cpu->r[rt]==loaded) {table_address=load_cell;table_value=loaded;table_reg=rt;}
        if (!branch) continue;
        uint32_t delay_word;
        stop->pc=cpu->pc;
        if (!cpu->delay_slot || cpu->branch_pc!=pc || cpu->pc!=pc+4u ||
            !title_mixer_word(m,cpu,&delay_word)) break;
        unsigned delay_op=delay_word>>26,delay_fn=delay_word&63u;
        if ((delay_op>=1u && delay_op<=7u) || (!delay_op && (delay_fn==8u || delay_fn==9u))) break;
        if (link || indirect) {
            if (cpu->npc!=dest || (link && cpu->r[31]!=pc+8u)) break;
        } else {
            dest=cpu->npc;
            if (title_mixer_symbol(dest,family)!=run.symbol) break;
        }
        if (!title_mixer_step(m,cpu,stop,delay_word) || cpu->pc!=dest ||
            cpu->npc!=dest+4u || cpu->delay_slot || cpu->branch_pc) break;
        if (ret) {
            if (cpu->r[29]!=frames[n-1].sp) break;
            --n;
        }
        if (link) {
            if (cpu->r[29]!=sp) break;
            if (music && family==MUSIC_SEQUENCE &&
                !music_child_entry(m,cpu,&run,child,sp)) break;
            frames[n].ret=pc+8u;frames[n].sp=sp;frames[n].symbol=child;++n;
        }
        if (link || indirect) table_address=0;
    }
    int ok=!n && cpu->pc==caller+8u && cpu->r[31]==caller+8u && cpu->r[29]==run.root_sp &&
        cpu->r[28]==gp && cpu->r[30]==fp && !memcmp(saved,&cpu->r[16],sizeof saved);
    cpu->title_mixer=NULL;
    return ok;
}
