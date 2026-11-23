/* The game sound tick runs its pending music work between its voice status
 * sample and its ordinary channel tick. Keep the mounted game's timing for
 * the sequencer and its descendants. Only the lower 16 voices belong here.
 * This file is included after the mixer state and registered device symbols. */
#include "opening_music_data.h"

static const TitleMixerSymbol *title_mixer_symbol(uint32_t pc,unsigned family);

static const TitleMixerSymbol music_functions[] = {
    {0x80037fc4u,0x8003819cu,MUSIC_SEQUENCE}, /* service the two passes */
    {0x8003836cu,0x800383a4u,MUSIC_SEQUENCE}, /* player status */
    {0x80038a58u,0x80038fc4u,MUSIC_SEQUENCE}, /* update active players */
    {0x80038fc4u,0x80038ffcu,MUSIC_SEQUENCE}, /* variable-length delta */
    {0x80038ffcu,0x8003916cu,MUSIC_SEQUENCE},
    {0x80039308u,0x80039b20u,MUSIC_SEQUENCE},
    {0x80039b20u,0x80039c5cu,MUSIC_SEQUENCE},
    {0x80039c70u,0x80039decu,MUSIC_SEQUENCE}, /* meta */
    {0x80039decu,0x80039f14u,MUSIC_SEQUENCE}, /* controller */
    {0x80039f50u,0x8003a098u,MUSIC_SEQUENCE},
    {0x8003a098u,0x8003a0d0u,MUSIC_SEQUENCE},
    {0x8003a234u,0x8003a3d8u,MUSIC_SEQUENCE}
};

static const TitleMixerSymbol *music_game_symbol(uint32_t pc) {
    for (size_t i=0; i<sizeof music_functions/sizeof music_functions[0]; ++i)
        if (pc>=music_functions[i].start && pc<music_functions[i].end) return &music_functions[i];
    return NULL;
}

static int music_root_call(MusashiBootMemory *m, const FormatterCpu *cpu,
    const IrqGuestReturn *stack, unsigned depth, uint32_t caller, uint32_t target,
    int swept, int at_entry) {
    if (!((caller == 0x8002cee8u && target == 0x80037fc4u) ||
          (swept && caller == 0x8002cf2cu && target == 0x8003836cu))) return 0;
    MusicDataTrack track;
    uint16_t flags;
    if (!m || !cpu || !stack || depth != 4u || stack[0].callee != IRQ_DISPATCH ||
        stack[1].callee != IRQ_VBLANK || stack[2].callee != IRQ_GAME_VBLANK ||
        stack[3].callee != IRQ_SOUND_TICK || stack[3].address != 0x80010984u ||
        stack[3].sp != 0x8006cab4u || cpu->r[29] != 0x8006ca94u ||
        !musashi_boot_read16(m, 0x800a4e8eu, &flags) || !(flags & 8u) ||
        !music_data_equals(m, 0x800a4e8cu, 2u, 0x91u) ||
        !music_data_equals(m, 0x800a4e8au, 2u, 0u) ||
        !music_track_range(m, MUSIC_PLAYERS, &track) ||
        (at_entry && (cpu->r[31] != caller + 8u || cpu->pc != target))) return 0;

    if (caller == 0x8002cee8u) return target == 0x80037fc4u;
    return swept && caller == 0x8002cf2cu && target == 0x8003836cu &&
           (!at_entry || cpu->r[4] == 0u);
}

typedef struct { uint32_t parent, site, target; } OpeningMusicCall;
static const OpeningMusicCall music_calls[] = {
    {0x80037fc4u, 0x80037fe4u, 0x80038a58u},
    {0x80037fc4u, 0x80038030u, 0x8003d3b4u},
    {0x80037fc4u, 0x800380f0u, 0x8003b250u},
    {0x80037fc4u, 0x8003812cu, 0x8003c23cu},
    {0x80037fc4u, 0x80038160u, 0x8003be74u},
    {0x80038a58u, 0x80038bf4u, 0x80038ffcu},
    {0x80038a58u, 0x80038c0cu, 0x80038fc4u},
    {0x80038ffcu, 0x80039064u, 0x80039b20u},
    {0x80038ffcu, 0x80039074u, 0x80039308u},
    {0x80038ffcu, 0x80039094u, 0x80039f50u},
    {0x80038ffcu, 0x800390a4u, 0x8003a098u},
    {0x80038ffcu, 0x80039154u, 0x8003a234u},
    {0x80039c70u, 0x80039d74u, 0x8003b45cu},
    {0x80039f50u, 0x80039fe8u, 0x80039c70u},
    {0x80039f50u, 0x8003a080u, 0x80039decu},
};

/* A zero site validates a mounted table value before the edge is issued. */
static const TitleMixerSymbol *music_callee(uint32_t parent, uint32_t site, uint32_t target) {
    for (size_t i = 0; i < sizeof music_calls / sizeof music_calls[0]; ++i) {
        const OpeningMusicCall *call = &music_calls[i];
        if (call->parent != parent || call->target != target || (site && call->site != site))
            continue;
        const TitleMixerSymbol *game = music_game_symbol(target);
        if (game) return game;
        for (size_t j = 0; j < sizeof title_mixer_symbols / sizeof title_mixer_symbols[0]; ++j)
            if (title_mixer_symbols[j].start == target) return &title_mixer_symbols[j];
    }
    return NULL;
}

/* Emit the first entrance per function on this owner thread for ordinary
 * playback qualification. The game clock is charged exclusively by issue. */
static void music_observe_entry(const TitleMixerSymbol *symbol, uint32_t caller) {
    static _Thread_local uint32_t seen;
    if (symbol->family != MUSIC_SEQUENCE) return;
    size_t i = (size_t)(symbol - music_functions);
    uint32_t bit = 1u << i;
    if (!(seen & bit)) {
        seen |= bit;
        fprintf(stderr, "native_boot: OPENING_MUSIC_ENTER pc=%08x caller=%08x\n",
                (unsigned)symbol->start, (unsigned)caller);
    }
}

static int music_child_entry(MusashiBootMemory *m, const FormatterCpu *cpu, TitleMixerRun *run,
    const TitleMixerSymbol *child, uint32_t sp) {
    uint32_t voice=cpu->r[4], arg=cpu->r[5];
    if (cpu->r[29]!=sp || !child) return 0;
    if (child->start==0x80038a58u) {
        const uint8_t *busy=musashi_boot_ram_span(m,0x800a4f1du,1u);
        return busy && !*busy;
    }
    if (child->family==MUSIC_REQUEST) {
        run->gain_written=0;
        return voice==0x800a4eccu && music_data_equals(m,voice,4,6);
    }
    if (child->family==MUSIC_SEQUENCE) return 1;
    run->record=0; run->mask_cell=0; run->key_seen=run->emitted_key=run->reverb_seen=0;
    if (child->family==TITLE_KEYS || child->family==TITLE_REVERB_MASK) {
        uint32_t word;
        if (voice>1u || !arg || arg>0xffffu) return 0;
        run->mask_cell=child->family==TITLE_KEYS ? (voice ? 0x800c7d20u : 0x800a2b98u) :
                                                  (voice ? 0x800c7d2cu : 0x800a2ba0u);
        run->direction=voice;
        run->voices=arg;
        return musashi_boot_read32(m,run->mask_cell,&word) && (word&0xffffu)==arg;
    }
    if (voice>=16u) return 0;
    uint32_t base=0x800c6dd0u+0x60u*voice;
    const uint8_t *flags=musashi_boot_ram_span(m,base+0x5au,5u);
    if (!flags || !flags[0]) return 0;
    run->record=base+0x10u;
    run->voices=1u<<voice;
    if (child->family==TITLE_VOICE_ATTR) return flags[3] && arg==run->record;
    return child->family==MUSIC_RELEASE && !flags[3] && flags[4] && arg==8u;
}
