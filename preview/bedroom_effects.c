/* SPDX-License-Identifier: MIT */
#include "bedroom_effects.h"
#include "../pc_port/forest_player_archive.h"
#include "bedroom_record.h"
#include "bedroom_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"
#include <stdlib.h>
#include <string.h>

enum { SPU_RATE = 44100, OUTPUT_RATE = 37800, VOICES = 24, MAX_LAYERS = 3 };
static const uint32_t effect_base = 0x800c7f08u;
static const unsigned woods_swings[] = {
    BFM_FOREST_SWING_SOUND, BFM_WOODS_FUSION_FOLLOWUP_SOUND, BFM_WOODS_LUMINA_SWING_SOUND
};
static const unsigned woods_actor_sounds[] = {
    BFM_WOODS_RING_SOUND, BFM_WOODS_PINK_BITE_SOUND, BFM_WOODS_LENO_BARK_SOUND,
    BFM_WOODS_LENO_YELP_SOUND, BFM_WOODS_SPLASH_SOUND,
    BFM_FOREST_HIT_SOUND, BFM_WOODS_FUSION_FINISH_HIT_SOUND, BFM_WOODS_CHARGED_SLASH_SOUND
};
enum {
    CLEFT_SWINGS = 9,
    CLEFT_ACTORS = CLEFT_SWINGS + sizeof woods_swings / sizeof *woods_swings,
    CLEFT_PICKUP = CLEFT_ACTORS + sizeof woods_actor_sounds / sizeof *woods_actor_sounds,
    CLEFT_RECOVERY, CLEFT_CHARGE, CLEFT_FULL_CHARGE, CLEFT_CLIPS
};
typedef struct {
    const MusashiDiscMedia *disc;
    const uint8_t *data;
    size_t bytes;
    unsigned key, group;
    BfmPalaceBank *common, *scene;
    uint8_t volume[256], pitch[256];
} Sources;
typedef struct {
    BfmDecodedSpuSample sample;
    BfmSpuEnvelope envelope;
    uint8_t control[20];
    unsigned delay, gain, center, pitch, field, sample_in_field, gate, gate_samples;
    int key, tuning, modulated, slide, done;
    unsigned slide_hold, slide_step[2];
    int slide_goal[2];
    size_t phase;
} Layer;
typedef struct {
    int previous, phase, done;
} Resampler;
typedef struct {
    Layer initial;
    uint8_t pitch_table[256];
} KeyedTone;
typedef struct {
    unsigned id;
    size_t frames, retrigger_frames;
    int16_t *mono;
    KeyedTone *tone;
} Clip;
typedef struct {
    const Clip *clip;
    unsigned command;
    size_t frame;
    Layer layer;
    Resampler resampler;
} Playback;
struct BfmBedroomEffects {
    Clip clips[CLEFT_CLIPS];
    Playback playing[VOICES];
    int replace_matching;
    unsigned terrain_aliases;
};

static unsigned half(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t word(const uint8_t *p) { return half(p) | (uint32_t)half(p+2) << 16; }
static int read_data(const Sources *s, uint32_t address, uint8_t *out, size_t count)
{
    if (address < effect_base) return bfm_bedroom_resident_data(s->disc, address, out, count);
    size_t offset = address-effect_base;
    if (offset > s->bytes || count > s->bytes-offset) return 0;
    memcpy(out, s->data+offset, count);
    return 1;
}

/* Area-directed sounds first select a column for the loaded room bank.
 * Common button sounds already point to a resident effect table. */
static int description(const Sources *s, unsigned command, uint32_t *address, unsigned *resolved)
{
    uint8_t entry[4], pointer[4], value[2];
    const uint8_t *header = NULL;
    if (!read_data(s, 0x8006451cu+4*command, entry, sizeof entry)) return 0;
    if (s->data || entry[0] == 6 || entry[1]) {
        /* Each area sound payload ends with its 28-byte selector directory. */
        if (!s->data || s->bytes < 28) return 0;
        header = s->data + s->bytes - 28;
        if (half(header) != s->group) return 0;
    }
    if (entry[0] == 6) {
        unsigned first = half(header+4), columns = half(header+14), row = entry[3];
        if (entry[2] != 3 || s->key < first || s->key-first >= half(header+2) ||
            row >= half(header+12) || !read_data(s, word(header+8)+2*(s->key-first), value, 2)) return 0;
        unsigned column = half(value);
        if (!column || column > columns ||
            !read_data(s, word(header+16)+2*(row*columns+column-1), value, 2)) return 0;
        command = half(value);
        if (!read_data(s, 0x8006451cu+4*command, entry, 4)) return 0;
    }
    /* Direct common menu effects preserve their high service-mode flag
     * in the resident slot. The lower six bits select the instrument
     * descriptor and run the same envelope. */
    if ((entry[0] != 1 && entry[0] != 0x41) || (entry[1] && entry[1] != s->group)) return 0;
    uint32_t tables = entry[1] ? word(header+20) : 0x80068304u;
    if (!read_data(s, tables+4*entry[2], pointer, 4)) return 0;
    *address = word(pointer)+20u*entry[3];
    if (resolved) *resolved = command;
    return 1;
}

static unsigned pitch(const uint8_t table[256], const Layer *l)
{
    int key = l->key + (60-(int)l->center)*256 - 2*l->tuning;
    if (key < 0 || key >= 127*256) return 0;
    const uint8_t *p = table+2*(key>>8);
    unsigned low = half(p), high = half(p+2);
    return low + ((high-low)*(unsigned)(key&255) >> 8);
}

static int instrument_open(const Sources *s, unsigned bank, unsigned instrument, Layer *l)
{
    uint8_t map[12], pair[2];
    if ((bank && bank != s->key) || !read_data(s, 0x80064d44u+12*bank, map, sizeof map) ||
        instrument >= map[4] || !read_data(s, word(map)+2*instrument, pair, 2)) return 0;
    const BfmPalaceBank *vab = bank ? s->scene : s->common;
    BfmPalaceTone tone;
    BfmSpuSample encoded;
    if (!bfm_room_effect_tone(vab, pair[0], pair[1], &tone) ||
        !bfm_palace_sound_sample(vab, tone.sample, &encoded) ||
        !bfm_spu_sample_decode(encoded.data, encoded.bytes, &l->sample)) return 0;
    l->center = tone.center;
    l->tuning = tone.tuning;
    bfm_spu_envelope_start(&l->envelope,(uint16_t)tone.adsr1,(uint16_t)tone.adsr2);
    return 1;
}

static int layer_open(const Sources *s, const uint8_t d[20], Layer *l)
{
    if (d[4] || d[7] || d[5]>=128 || d[6]>=128 || !instrument_open(s,d[8],d[9],l)) return 0;
    l->delay = half(d+2) * (SPU_RATE/60);
    l->gain = ((half(s->volume+2*d[6])*127 >> 7)*127) >> 7;
    l->key = d[5]*256;
    l->pitch = pitch(s->pitch,l);
    if (word(d+16)) {
        uint32_t command = word(d+16);
        if (!read_data(s, command, l->control, 4)) return 0;
        unsigned control = half(l->control);
        if (control == 0 || control == 8) {
            /* Both slides have a note target and a release target. Type 8
             * holds the starting key first and measures both goals from it. */
            unsigned relative = control == 8;
            if (!read_data(s, command, l->control, relative ? 14 : 12)) return 0;
            const uint8_t *points = l->control + (relative ? 4 : 2);
            l->slide = 1;
            l->slide_hold = relative ? half(l->control+2) : 0;
            l->slide_step[0] = half(points);
            l->slide_step[1] = half(points+6);
            for (unsigned i = 0; i < 2; ++i) {
                unsigned goal = half(points+2+2*i);
                l->slide_goal[i] = relative ? l->key + (int16_t)goal : (int)goal;
                if (l->slide_goal[i] < 0 || l->slide_goal[i] >= 127*256) return 0;
            }
            l->gate = half(points+8);
        } else if (control == 2) {
            /* A fixed-pitch gate is just two halfwords, independent of the
             * adjacent controls in the resident effect table. */
            l->gate = half(l->control+2);
            if (!l->gate) return 0;
        } else {
            if (half(l->control) != 6 || !read_data(s, command, l->control, sizeof l->control) ||
                !half(l->control+4) || !half(l->control+8) || !half(l->control+16)) return 0;
            l->modulated = 1;
        }
    }
    return l->pitch && (l->gate || l->modulated || l->sample.loop_start == l->sample.frames);
}

static int approach(int value, int target, int step)
{
    if (value < target) return target-value < step ? target : value+step;
    return value-target < step ? target : value-step;
}

static void sound_field(const uint8_t table[256], Layer *l)
{
    const uint8_t *c = l->control;
    unsigned t = ++l->field;
    if (l->slide) {
        int releasing = l->gate && t > l->gate;
        if (releasing || t > l->slide_hold) {
            l->key = approach(l->key,l->slide_goal[releasing],(int)l->slide_step[releasing]);
            l->pitch = pitch(table,l);
        }
    }
    if (l->gate == t) bfm_spu_envelope_release(&l->envelope);
    if (!l->modulated || t <= half(c+2)) return;
    if (t == half(c+16)) bfm_spu_envelope_release(&l->envelope);
    if (t > half(c+16)) l->key = approach(l->key,c[13]*256,(int)half(c+14));
    else if (l->modulated == 1) {
        l->key = approach(l->key,c[7]*256,(int)half(c+4));
        if (l->key == c[7]*256) l->modulated = 2;
    } else l->key = approach(l->key,c[11]*256,(int)half(c+8));
    l->pitch = pitch(table,l);
}

static int next_sample(const uint8_t table[256], Layer *l)
{
    if (l->done) return 0;
    if (l->delay) { --l->delay; return 0; }
    size_t index = l->phase >> 12, end = l->sample.frames;
    size_t next = index+1 < end ? index+1 : l->sample.loop_start < end ? l->sample.loop_start : index;
    int v = l->sample.pcm[index];
    v += (l->sample.pcm[next]-v)*(int)(l->phase&4095)/4096;
    int level = bfm_spu_envelope_clock(&l->envelope);
    l->phase += l->pitch;
    if (l->phase >= end*4096) {
        size_t span = end-l->sample.loop_start;
        if (!span) l->done = 1;
        else l->phase = l->sample.loop_start*4096 + (l->phase-end*4096)%(span*4096);
    }
    if (!level && l->envelope.stage == BFM_SPU_ENVELOPE_RELEASE) l->done = 1;
    if (l->gate_samples && !--l->gate_samples) bfm_spu_envelope_release(&l->envelope);
    if (++l->sample_in_field == SPU_RATE/60) {
        l->sample_in_field = 0;
        sound_field(table,l);
    }
    return (int)((int64_t)v*level*l->gain / (32767*16384));
}

static int16_t limited(int value)
{
    return (int16_t)(value > INT16_MAX ? INT16_MAX : value < INT16_MIN ? INT16_MIN : value);
}

/* Keep the SPU clocks and fractional output position when a playing note
 * changes from its mounted envelope to key-off between two mixer calls. */
static int next_frame(const uint8_t table[256], Layer layers[], unsigned count,
                      Resampler *rate, int16_t *out)
{
    while (!rate->done) {
        int current = 0, active = 0;
        for (unsigned i = 0; i < count; ++i) {
            current += next_sample(table, &layers[i]);
            active |= !layers[i].done;
        }
        int emit = rate->phase <= 0;
        if (emit) {
            *out = limited(current + (int)((int64_t)(rate->previous-current)*(-rate->phase)/OUTPUT_RATE));
            rate->phase += SPU_RATE;
        }
        /* A final SPU sample can still interpolate into the next output. */
        rate->done = !active && (!current || rate->phase >= OUTPUT_RATE);
        rate->previous = current;
        rate->phase -= OUTPUT_RATE;
        if (emit) return 1;
    }
    return 0;
}

static int render_layers(const Sources *s, Layer layers[], unsigned count,
                         unsigned command, Clip *clip)
{
    size_t capacity = OUTPUT_RATE;
    clip->mono = malloc(capacity * sizeof *clip->mono);
    if (!clip->mono) return 0;
    Resampler rate = {0};
    int16_t sample;
    while (next_frame(s->pitch, layers, count, &rate, &sample)) {
        if (clip->frames == capacity) {
            if (capacity >= OUTPUT_RATE*8) return 0;
            capacity *= 2;
            int16_t *p = realloc(clip->mono, capacity*sizeof *p);
            if (!p) return 0;
            clip->mono = p;
        }
        clip->mono[clip->frames++] = sample;
    }
    clip->id = command;
    return 1;
}

/* Field counts in the resident note sequence use the NTSC sound service. */
static unsigned field_sample(unsigned field)
{
    return (unsigned)((uint64_t)field*SPU_RATE*1001u/60000u);
}

static int prepare(const Sources *s, unsigned command, Clip *clip)
{
    uint32_t address;
    Layer layers[MAX_LAYERS] = {0};
    unsigned count = 0;
    int result = 0;
    if (!description(s, command, &address, NULL)) return 0;
    for (;;) {
        uint8_t d[20];
        if (count == MAX_LAYERS || !read_data(s,address,d,sizeof d)) goto done;
        if (word(d+12)) {
            if (command != BFM_WOODS_GUARD_SOUND || count || half(d+14)) goto done;
            /* This is the same-command protection interval, not a delay
             * before the blow sounds or a key-off for the instrument. */
            clip->retrigger_frames = ((uint64_t)half(d+12)*OUTPUT_RATE*1001u+59999u)/60000u;
        }
        if (!layer_open(s,d,&layers[count])) goto done;
        if (command == BFM_WOODS_FULL_CHARGE_SOUND)
            layers[count].delay = field_sample(half(d+2));
        ++count;
        if (!d[11]) break;
        address += sizeof d;
    }
    if (command == BFM_WOODS_CHARGE_SOUND) {
        if (count != 1 || !layers[0].gate || layers[0].slide || layers[0].modulated) goto done;
        clip->tone = malloc(sizeof *clip->tone);
        if (!clip->tone) goto done;
        clip->tone->initial = layers[0];
        clip->tone->initial.gate_samples = field_sample(layers[0].gate);
        clip->tone->initial.gate = 0;
        memcpy(clip->tone->pitch_table, s->pitch, sizeof s->pitch);
        layers[0].sample = (BfmDecodedSpuSample){0}; /* The clip owns the decoded note. */
        clip->id = command;
        result = 1;
    } else {
        result = render_layers(s,layers,count,command,clip);
    }
done:
    for (unsigned i=0; i<MAX_LAYERS; ++i) bfm_spu_sample_free(&layers[i].sample);
    return result;
}

static int prepare_sequence(const Sources *s, unsigned command, Clip *clip)
{
    enum { MAX_NOTES = 32 };
    uint8_t entry[4], track[12];
    if (!read_data(s, 0x8006451cu+4*command, entry, sizeof entry) ||
        (entry[0] & 0x7f) != 5 || entry[1] || entry[2] ||
        !read_data(s, 0x80068a54u+12*entry[3], track, sizeof track) || track[8] != 0x10)
        return 0;
    uint32_t cursor = word(track), repeat_start = 0;
    unsigned repeats = 0, count = 0, field = 0, attenuation = 0;
    Layer timbre = {0}, notes[MAX_NOTES];
    unsigned instrument = 0;
    int result = 0;
    for (unsigned steps=0; steps<128; ++steps) {
        uint8_t note[20];
        if (cursor >= effect_base || !read_data(s,cursor,note,4)) break;
        unsigned op = note[3];
        if (op == 0x87) {
            if (count && !repeats && !note[0] && !note[1] && !note[2])
                result = render_layers(s,notes,count,command,clip);
            break;
        }
        if (op == 0x82) {
            if (repeats || !note[0] || note[1] || note[2]) break;
            repeats = note[0];
            cursor += 4;
            repeat_start = cursor;
            continue;
        }
        if (op == 0x85) {
            /* This phrase repeats at the same pitch with a softer echo. */
            if (!repeats || note[0] || note[1] >= 128 || note[2]) break;
            if (--repeats) {
                attenuation += note[1];
                cursor = repeat_start;
            } else {
                attenuation = 0;
                cursor += 4;
            }
            continue;
        }
        if (op != 0x80 && op != 0x81) break;
        unsigned bytes = op == 0x80 ? 20 : 8;
        if (count == MAX_NOTES || !read_data(s,cursor,note,bytes) ||
            note[2] >= 128 || note[2] < attenuation) break;
        if (op == 0x80) {
            /* Both supported phrases keep one common-bank instrument.
             * The pickup repeats its definition and changes voice priority. */
            if (note[8] > 1 || note[10] || note[11] || word(note+16)) break;
            if (timbre.sample.pcm) {
                if (note[9] != instrument) break;
            } else {
                instrument = note[9];
                if (!instrument_open(s,0,instrument,&timbre)) break;
            }
        }
        if (!timbre.sample.pcm) break;
        Layer *voice = &notes[count++];
        *voice = timbre;
        voice->key = (int)half(note);
        voice->pitch = pitch(s->pitch,voice);
        unsigned gate = half(note+4), interval = half(note+6);
        if (!voice->pitch || (!gate && voice->sample.loop_start < voice->sample.frames) ||
            gate > 480-field || interval > 480-field) break;
        voice->delay = field_sample(field);
        voice->gate_samples = field_sample(field+gate)-voice->delay;
        unsigned gain = half(s->volume+2*(note[2]-attenuation))*127u >> 7;
        /* The sequence's three volume controls start at full scale. */
        for (unsigned i=0; i<3; ++i) gain = gain*16383u >> 14;
        voice->gain = gain;
        field += interval;
        cursor += bytes;
    }
    bfm_spu_sample_free(&timbre.sample);
    return result;
}

static int load_tables(Sources *s)
{
    return read_data(s,0x8006aa30u,s->volume,sizeof s->volume) &&
           read_data(s,0x8006ab30u,s->pitch,sizeof s->pitch);
}

static BfmBedroomEffects *town_effects_load(const MusashiDiscMedia *disc, unsigned member,
                                           const unsigned commands[], size_t count)
{
    Sources sources = {.disc = disc};
    uint8_t *data = bfm_archive_effect_data(disc, "SC03.CD", member, &sources.bytes);
    if (!data || sources.bytes < 28) {
        free(data);
        return NULL;
    }

    /* These rooms each select a single bank and a single area column. */
    const uint8_t *directory = data + sources.bytes - 28;
    BfmBedroomEffects *effects = NULL;
    if (count <= CLEFT_CLIPS && half(directory + 2) == 1 && half(directory + 14) == 1) {
        sources.data = data;
        sources.group = half(directory);
        sources.key = half(directory + 4);
        sources.scene = bfm_archive_sound_bank_load(disc, "SC03.CD", member);
        if (sources.scene && load_tables(&sources)) {
            effects = calloc(1, sizeof *effects);
            for (size_t i = 0; effects && i < count; ++i) {
                if (!prepare(&sources, commands[i], &effects->clips[i])) {
                    bfm_bedroom_effects_free(effects);
                    effects = NULL;
                }
            }
        }
    }
    bfm_palace_sound_bank_free(sources.scene);
    free(data);
    return effects;
}

BfmBedroomEffects *bfm_village_knock_sound_load(const MusashiDiscMedia *disc)
{
    const unsigned commands[] = { BFM_VILLAGE_KNOCK_SOUND };
    return town_effects_load(disc, 2, commands, sizeof commands / sizeof *commands);
}

BfmBedroomEffects *bfm_nightmare_sound_load(const MusashiDiscMedia *disc)
{
    const unsigned commands[] = {
        BFM_NIGHTMARE_WAKE_SOUND, BFM_NIGHTMARE_BED_LANDING_SOUND, BFM_NIGHTMARE_FLOOR_LANDING_SOUND
    };
    return town_effects_load(disc, 3, commands, sizeof commands / sizeof *commands);
}

BfmBedroomEffects *bfm_forest_swing_sound_load(const MusashiDiscMedia *disc)
{
    Sources s = {.disc=disc};
    s.common = bfm_archive_sound_bank_load(disc,"MAIN.CD",2);
    BfmBedroomEffects *e = calloc(1,sizeof *e);
    if (!e || !s.common || !load_tables(&s) || !prepare(&s,BFM_FOREST_SWING_SOUND,&e->clips[0])) {
        bfm_bedroom_effects_free(e);
        e = NULL;
    }
    if (e) {
        /* Giant Forest uses area group 1 and the first room sound bank. */
        s.key = 1;
        s.group = 1;
        uint8_t *area = bfm_bedroom_effect_data(disc,77,&s.bytes);
        s.data = area;
        s.scene = bfm_archive_sound_bank_load(disc,"SC01.CD",77);
        if (area && s.scene) prepare(&s,BFM_FOREST_HIT_SOUND,&e->clips[1]);
        bfm_palace_sound_bank_free(s.scene);
        free(area);
        /* The direct firing command selects the equipped skill's bank and
         * uses the resident pitch and volume tables already read above. */
        s.data = NULL;
        s.bytes = 0;
        s.group = 0;
        s.key = 5;
        s.scene = bfm_archive_sound_bank_load(disc,"MAIN.CD",15);
        if (s.scene) prepare(&s,BFM_FOREST_GUNSHOT_SOUND,&e->clips[2]);
        bfm_palace_sound_bank_free(s.scene);
    }
    bfm_palace_sound_bank_free(s.common);
    return e;
}

BfmBedroomEffects *bfm_bedroom_effects_load(const MusashiDiscMedia *disc, unsigned member, uint32_t room)
{
    Sources s = {.disc=disc,.key=room>>16,.group=(room>>12)&15};
    uint8_t *data = bfm_bedroom_effect_data(disc,member,&s.bytes);
    s.data = data;
    s.common = bfm_archive_sound_bank_load(disc,"MAIN.CD",2);
    s.scene = bfm_archive_sound_bank_load(disc,"SC01.CD",member);
    BfmBedroomEffects *e = calloc(1,sizeof *e);
    if (!e || !data || !s.common || !s.scene ||
        !load_tables(&s) ||
        !prepare(&s,BFM_ROOM_AWARD_SOUND,&e->clips[0]) ||
        !prepare(&s,BFM_ROOM_MOVE_SOUND,&e->clips[1]) ||
        !prepare(&s,BFM_ROOM_ACCEPT_SOUND,&e->clips[2]) ||
        !prepare(&s,0x45a,&e->clips[3]) || !prepare(&s,0x45d,&e->clips[4]) ||
        !prepare(&s,0x473,&e->clips[5]) || !prepare(&s,0x474,&e->clips[6])) {
        bfm_bedroom_effects_free(e);
        e = NULL;
    }
    bfm_palace_sound_bank_free(s.scene);
    bfm_palace_sound_bank_free(s.common);
    free(data);
    return e;
}

static BfmBedroomEffects *common_effects_load(const MusashiDiscMedia *disc,
                                              const unsigned commands[], size_t count)
{
    Sources s = {.disc=disc};
    s.common = bfm_archive_sound_bank_load(disc,"MAIN.CD",2);
    BfmBedroomEffects *e = NULL;
    if (s.common && load_tables(&s)) e = calloc(1, sizeof *e);
    for (size_t i = 0; e && i < count; ++i) {
        if (!prepare(&s, commands[i], &e->clips[i])) {
            bfm_bedroom_effects_free(e);
            e = NULL;
        }
    }
    bfm_palace_sound_bank_free(s.common);
    return e;
}

BfmBedroomEffects *bfm_menu_effects_load(const MusashiDiscMedia *disc)
{
    const unsigned commands[] = {BFM_ROOM_MOVE_SOUND, BFM_ROOM_ACCEPT_SOUND};
    return common_effects_load(disc, commands, sizeof commands / sizeof *commands);
}

/* Cleft's eight surface contacts select the same mounted effect as guarding.
 * Keep one decoded clip for every selector which actually resolves to it. */
static void prepare_terrain_hits(const Sources *s, BfmBedroomEffects *effects)
{
    uint32_t address;
    unsigned guard, surface;
    if (!prepare(s, BFM_WOODS_GUARD_SOUND, &effects->clips[6]) ||
        !description(s, BFM_WOODS_GUARD_SOUND, &address, &guard)) return;
    for (unsigned id = BFM_WOODS_TERRAIN_HIT_FIRST; id <= BFM_WOODS_TERRAIN_HIT_LAST; ++id) {
        if (description(s, id, &address, &surface) && surface == guard)
            effects->terrain_aliases |= 1u << (id - BFM_WOODS_TERRAIN_HIT_FIRST);
    }
}

BfmBedroomEffects *bfm_woods_shrink_sound_load(const MusashiDiscMedia *disc)
{
    Sources s = {.disc = disc};
    s.common = bfm_archive_sound_bank_load(disc, "MAIN.CD", 2);
    BfmBedroomEffects *effects = NULL;
    if (s.common && load_tables(&s)) effects = calloc(1, sizeof *effects);
    if (!effects || !prepare(&s,BFM_WOODS_SHRINK_SOUND,&effects->clips[0])) {
        bfm_bedroom_effects_free(effects);
        bfm_palace_sound_bank_free(s.common);
        return NULL;
    }
    effects->replace_matching = 1;
    if (!prepare(&s, BFM_WOODS_CHARGE_SOUND, &effects->clips[CLEFT_CHARGE]) ||
        !prepare(&s, BFM_WOODS_FULL_CHARGE_SOUND, &effects->clips[CLEFT_FULL_CHARGE])) {
        bfm_bedroom_effects_free(effects);
        bfm_palace_sound_bank_free(s.common);
        return NULL;
    }
    prepare_sequence(&s, BFM_BODY_LEVEL_SOUND, &effects->clips[2]);
    prepare_sequence(&s, BFM_WOODS_PICKUP_SOUND, &effects->clips[CLEFT_PICKUP]);
    for (unsigned i=0; i<sizeof woods_swings/sizeof *woods_swings; ++i)
        prepare(&s, woods_swings[i], &effects->clips[CLEFT_SWINGS+i]);
    prepare(&s, BFM_WOODS_CARRY_THROW_SOUND, &effects->clips[8]);
    prepare(&s, BFM_WOODS_HURT_SOUND, &effects->clips[5]);

    /* The award uses the common bank. The cast uses the mounted skill's
     * own bank, selected by its resident player resource record. */
    ForestPlayerArchive player;
    uint8_t row[8];
    enum { CLEFT_WOODS = 29 };
    uint8_t *area = bfm_archive_effect_data(disc, "SC03.CD", CLEFT_WOODS, &s.bytes);
    int has_player_bank = forest_player_archive_open(disc, &player);
    if (has_player_bank &&
        forest_player_archive_read(&player, UINT32_C(0x800d379c), row, sizeof row)) {
        s.key = word(row+4);
        s.scene = bfm_archive_sound_bank_load(disc, "MAIN.CD", word(row));
        if (s.scene) prepare(&s, BFM_WOODS_SATIATE_RECOVERY_SOUND, &effects->clips[CLEFT_RECOVERY]);
        bfm_palace_sound_bank_free(s.scene);
        s.scene = NULL;
    }
    if (has_player_bank &&
        forest_player_archive_read(&player, UINT32_C(0x800d3784), row, sizeof row)) {
        s.key = word(row + 4);
        s.scene = bfm_archive_sound_bank_load(disc, "MAIN.CD", word(row));
        if (s.scene) prepare(&s, BFM_WOODS_SHRINK_CAST_SOUND, &effects->clips[1]);
    }
    if (area && s.bytes >= 28) {
        /* The flattened King sounds through the equipped Shrink bank.
         * Its bite uses the room bank, even while it is small. */
        const uint8_t *dir = area+s.bytes-28;
        s.data = area;
        s.group = half(dir);
        if (s.scene) prepare(&s, BFM_WOODS_KING_STOMP_SOUND, &effects->clips[3]);
        bfm_palace_sound_bank_free(s.scene);
        s.scene = bfm_archive_sound_bank_load(disc, "SC03.CD", CLEFT_WOODS);
        uint32_t descriptor;
        uint8_t key;
        if (s.scene && description(&s, BFM_WOODS_KING_BITE_SOUND, &descriptor, NULL) &&
            read_data(&s, descriptor+8, &key, 1) && key >= half(dir+4) &&
            key-half(dir+4) < half(dir+2)) {
            s.key = key;
            prepare(&s, BFM_WOODS_KING_BITE_SOUND, &effects->clips[4]);
            prepare_terrain_hits(&s, effects);
            prepare(&s, BFM_WOODS_WATER_ENTRY_SOUND, &effects->clips[7]);
            for (unsigned i=0; i<sizeof woods_actor_sounds/sizeof *woods_actor_sounds; ++i)
                prepare(&s, woods_actor_sounds[i], &effects->clips[CLEFT_ACTORS+i]);
        }
    }
    bfm_palace_sound_bank_free(s.scene);
    free(area);
    bfm_palace_sound_bank_free(s.common);
    return effects;
}

void bfm_bedroom_effects_free(BfmBedroomEffects *e)
{
    if (!e) return;
    for (unsigned i=0; i<sizeof e->clips/sizeof *e->clips; ++i) {
        Clip *clip = &e->clips[i];
        if (clip->tone) {
            bfm_spu_sample_free(&clip->tone->initial.sample);
            free(clip->tone);
        }
        free(clip->mono);
    }
    free(e);
}

static void start_voice(Playback *p, const Clip *clip, unsigned command)
{
    *p = (Playback){.clip=clip, .command=command};
    if (clip->tone) p->layer = clip->tone->initial;
}

void bfm_bedroom_effects_stop(BfmBedroomEffects *effects)
{
    if (effects) memset(effects->playing, 0, sizeof effects->playing);
}

void bfm_bedroom_effects_start(BfmBedroomEffects *e, unsigned command)
{
    if (!e || !command) return;
    unsigned clip_command = command;
    if (command >= BFM_WOODS_TERRAIN_HIT_FIRST && command <= BFM_WOODS_TERRAIN_HIT_LAST &&
        (e->terrain_aliases & (1u << (command - BFM_WOODS_TERRAIN_HIT_FIRST))))
        clip_command = BFM_WOODS_GUARD_SOUND;
    if (command == BFM_WOODS_STOP_CHARGE_SOUND) {
        for (unsigned i = 0; i < VOICES; ++i) {
            Playback *p = &e->playing[i];
            if (p->clip && p->clip->id == BFM_WOODS_CHARGE_SOUND)
                bfm_spu_envelope_release(&p->layer.envelope);
        }
        return;
    }
    for (unsigned i=0; i<sizeof e->clips/sizeof *e->clips; ++i) if (e->clips[i].id == clip_command) {
        /* Repeated menu commands and rapid Gunshot fire restart the active
         * instance of that command. Surface aliases keep separate cursors
         * even when the mounted sample is the same. */
        if (e->replace_matching || i >= 3 || command == BFM_FOREST_GUNSHOT_SOUND) {
            for (unsigned j=0; j<VOICES; ++j) {
                Playback *p = &e->playing[j];
                if (p->clip && p->command == command &&
                    !(p->clip->tone && p->layer.envelope.stage == BFM_SPU_ENVELOPE_RELEASE)) {
                    if (p->frame >= e->clips[i].retrigger_frames)
                        start_voice(p, &e->clips[i], command);
                    return;
                }
            }
        }
        for (unsigned j=0; j<VOICES; ++j) if (!e->playing[j].clip) {
            start_voice(&e->playing[j], &e->clips[i], command);
            return;
        }
    }
}

static void mix_sample(int16_t stereo[2], int value)
{
    stereo[0] = limited(stereo[0]+value);
    stereo[1] = limited(stereo[1]+value);
}

void bfm_bedroom_effects_mix(BfmBedroomEffects *e, int16_t *stereo, size_t frames)
{
    if (!e || !stereo) return;
    for (unsigned j=0; j<VOICES; ++j) {
        Playback *p = &e->playing[j];
        if (!p->clip) continue;
        if (p->clip->tone) {
            for (size_t i = 0; i < frames; ++i) {
                int16_t value;
                if (!next_frame(p->clip->tone->pitch_table, &p->layer, 1, &p->resampler, &value)) break;
                mix_sample(stereo+2*i,value);
            }
            if (p->resampler.done) p->clip = NULL;
        } else {
            size_t left = p->clip->frames-p->frame;
            size_t n = left < frames ? left : frames;
            for (size_t i=0; i<n; ++i)
                mix_sample(stereo+2*i,p->clip->mono[p->frame++]);
            if (p->frame == p->clip->frames) p->clip = NULL;
        }
    }
}
