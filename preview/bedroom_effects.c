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

enum { SPU_RATE = 44100, OUTPUT_RATE = 37800, VOICES = 24, MAX_LAYERS = 4 };
enum { SEQUENCE_FULL_GAIN = 16383, MENU_RAMP_STEP = 1023 };
enum { SEQUENCE_FIELDS = 480, RESCUE_BP_STRIKE = 2822, HEAL_ARPEGGIO = 0xb07,
       RESCUE_BP_ARPEGGIO = 3168 };
enum {
    SHRINE_HIT_ACCEPTED = 0x5eb, SHRINE_RESTORED = 0x5ec,
    SHRINE_SHAKE = 0x635, SHRINE_APPEAR = 0x636, SHRINE_RAYS = 0x637,
    SHRINE_FLASH = 0x638, SHRINE_FINISH = 0x63b, SCROLL_BLADE_RETURN = 0x445
};
static const uint32_t effect_base = 0x800c7f08u;
static const unsigned woods_swings[] = {
    BFM_FOREST_SWING_SOUND, BFM_WOODS_FUSION_FOLLOWUP_SOUND, BFM_WOODS_LUMINA_SWING_SOUND
};
static const unsigned woods_actor_sounds[] = {
    BFM_WOODS_RING_SOUND, BFM_WOODS_PINK_BITE_SOUND, BFM_WOODS_LENO_BARK_SOUND,
    BFM_WOODS_LENO_YELP_SOUND, BFM_WOODS_SPLASH_SOUND,
    BFM_FOREST_HIT_SOUND, BFM_WOODS_FUSION_FINISH_HIT_SOUND, BFM_WOODS_CHARGED_SLASH_SOUND,
    BFM_WOODS_CRYSTAL_HIT_SOUND, BFM_WOODS_CRYSTAL_BREAK_SOUND,
    BFM_WOODS_RESCUE_DEPART_SOUND, BFM_WOODS_RESCUE_VANISH_SOUND
};
enum {
    CLEFT_SWINGS = 9,
    CLEFT_ACTORS = CLEFT_SWINGS + sizeof woods_swings / sizeof *woods_swings,
    CLEFT_PICKUP = CLEFT_ACTORS + sizeof woods_actor_sounds / sizeof *woods_actor_sounds,
    CLEFT_RECOVERY, CLEFT_CHARGE, CLEFT_FULL_CHARGE, CLEFT_HOP,
    CLEFT_BP_STRIKE, CLEFT_BP_ARPEGGIO, CLEFT_CLIPS
};
typedef struct {
    const MusashiDiscMedia *disc;
    const uint8_t *data;
    size_t bytes;
    unsigned key, group;
    int menu_sequences;
    BfmPalaceBank *common, *scene, *score;
    uint8_t volume[256], pitch[256];
} Sources;
typedef struct {
    BfmDecodedSpuSample sample;
    BfmSpuEnvelope envelope;
    uint8_t control[20];
    unsigned delay, gain, center, pitch, field, sample_in_field, gate, gate_samples;
    int key, tuning, modulated, slide, done, ntsc_fields;
    int sequence_base;
    unsigned sequence_gain, sequence_start, menu_attenuation;
    int menu_gain_latched;
    const uint16_t *sequence_levels;
    unsigned slide_hold, slide_step[2];
    int slide_goal[2];
    size_t phase;
} Layer;
typedef struct {
    int previous, phase, done;
} Resampler;
typedef struct {
    Layer initial[MAX_LAYERS];
    unsigned count;
    uint8_t pitch_table[256];
} KeyedTone;
/* The first half holds the prepared sequence and the second its live notes.
 * Each lower-village command restarts its own instance on retrigger. */
typedef struct {
    unsigned count, owned_samples;
    uint8_t pitch_table[256];
    uint16_t levels[SEQUENCE_FIELDS+1];
    Layer notes[];
} MenuSequence;
typedef struct {
    unsigned id;
    size_t frames, retrigger_frames;
    int16_t *mono;
    KeyedTone *tone;
    MenuSequence *menu_sequence;
} Clip;
typedef struct {
    const Clip *clip;
    unsigned command, menu_target, menu_staged;
    size_t frame;
    Layer layers[MAX_LAYERS];
    Resampler resampler;
} Playback;
struct BfmBedroomEffects {
    Clip clips[CLEFT_CLIPS];
    Playback playing[VOICES];
    int replace_matching, menu_sequences;
    unsigned sound_phase;
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
static int select_command(const Sources *s, unsigned command, uint8_t entry[4], unsigned *resolved)
{
    uint8_t value[2];
    const uint8_t *header = NULL;
    if (!read_data(s, 0x8006451cu+4*command, entry, 4)) return 0;
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
    if (entry[1] && entry[1] != s->group) return 0;
    if (resolved) *resolved = command;
    return 1;
}

static int description(const Sources *s, unsigned command, uint32_t *address, unsigned *resolved)
{
    uint8_t entry[4], pointer[4];
    if (!select_command(s, command, entry, resolved)) return 0;
    /* Direct common menu effects preserve their high service-mode flag
     * in the resident slot. The lower six bits select the instrument
     * descriptor and run the same envelope. */
    if (entry[0] != 1 && entry[0] != 0x41) return 0;
    uint32_t tables = entry[1] ? word(s->data+s->bytes-8) : 0x80068304u;
    if (!read_data(s, tables+4*entry[2], pointer, 4)) return 0;
    *address = word(pointer)+20u*entry[3];
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
    const BfmPalaceBank *vab;
    unsigned program, layer;
    if (bank & 0x80u) {
        /* These notes name a program and tone in the currently mounted score bank. */
        vab = s->score;
        program = bank & 0x7fu;
        layer = instrument;
    } else {
        uint8_t map[12], pair[2];
        if ((bank && bank != s->key) || !read_data(s, 0x80064d44u+12*bank, map, sizeof map) ||
            instrument >= map[4] || !read_data(s, word(map)+2*instrument, pair, 2)) return 0;
        vab = bank ? s->scene : s->common;
        program = pair[0];
        layer = pair[1];
    }
    BfmPalaceTone tone;
    BfmSpuSample encoded;
    if (!bfm_room_effect_tone(vab, program, layer, &tone) ||
        !bfm_palace_sound_sample(vab, tone.sample, &encoded) ||
        !bfm_spu_sample_decode(encoded.data, encoded.bytes, &l->sample)) return 0;
    l->center = tone.center;
    l->tuning = bank & 0x80u ? 0 : tone.tuning;
    bfm_spu_envelope_start(&l->envelope,(uint16_t)tone.adsr1,(uint16_t)tone.adsr2);
    return 1;
}

/* Types 7 and 8 measure both goals from the starting key. Only type 8
 * has an initial hold. Type 0 supplies absolute target keys. */
static int pitch_slide(Layer *l)
{
    unsigned type = half(l->control);
    const uint8_t *points = l->control + (type == 8 ? 4 : 2);
    l->slide = 1;
    l->slide_hold = type == 8 ? half(l->control+2) : 0;
    l->slide_step[0] = half(points);
    l->slide_step[1] = half(points+6);
    for (unsigned i=0; i<2; ++i) {
        unsigned goal = half(points+2+2*i);
        l->slide_goal[i] = type == 0 ? (int)goal : l->key + (int16_t)goal;
        if (l->slide_goal[i] < 0 || l->slide_goal[i] >= 127*256) return 0;
    }
    l->gate = half(points+8);
    return 1;
}

/* Field counts in resident note sequences use the NTSC sound service. */
static unsigned field_sample(unsigned field)
{
    return (unsigned)((uint64_t)field*SPU_RATE*1001u/60000u);
}

static int layer_open(const Sources *s, const uint8_t d[20], Layer *l, int sustained)
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
        if (control == 0 || control == 7 || control == 8) {
            if (!read_data(s, command, l->control, control == 8 ? 14 : 12) || !pitch_slide(l))
                return 0;
        } else if (control == 1) {
            /* Two successive pitch targets, with a separate glide on release. */
            if (!read_data(s, command, l->control, 16) ||
                half(l->control+4) >= 127*256 || half(l->control+8) >= 127*256 ||
                half(l->control+10) >= 127*256) return 0;
            l->modulated = l->key != (int)half(l->control+4);
            l->gate = half(l->control+14);
            if (!l->gate && l->sample.loop_start < l->sample.frames) return 0;
        } else if (control == 10) {
            /* A direct tone can carry the same two-stage bend and release
             * as a sequence note. Its gate belongs to the tone itself. */
            if (!read_data(s, command, l->control, sizeof l->control) || !half(l->control+16))
                return 0;
            l->sequence_base = l->key;
            l->modulated = 1;
            l->gate_samples = field_sample(half(l->control+16));
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
    return l->pitch && (sustained || l->gate || l->modulated || l->sample.loop_start == l->sample.frames);
}

static int approach(int value, int target, int step)
{
    if (value < target) return target-value < step ? target : value+step;
    return value-target < step ? target : value-step;
}

/* The first of the three sequence gain controls can change on the field clock. */
static unsigned sequence_level(const Layer *voice, unsigned field)
{
    unsigned index = field < SEQUENCE_FIELDS ? field : SEQUENCE_FIELDS;
    unsigned level = voice->sequence_levels ? voice->sequence_levels[index] : 16383;
    unsigned gain = voice->sequence_gain * level >> 14;
    gain = gain * (SEQUENCE_FULL_GAIN - voice->menu_attenuation) >> 14;
    return gain * 16383u >> 14;
}

static void sound_field(const uint8_t table[256], Layer *l)
{
    const uint8_t *c = l->control;
    unsigned t = ++l->field;
    if (l->sequence_levels) l->gain = sequence_level(l, l->sequence_start + t);
    if (half(c) == 1) {
        if (l->gate && t == l->gate) bfm_spu_envelope_release(&l->envelope);
        if (l->envelope.stage == BFM_SPU_ENVELOPE_RELEASE) {
            l->key = approach(l->key, (int)half(c+10), (int)half(c+12));
        } else if (l->modulated == 1) {
            l->key = approach(l->key, (int)half(c+4), (int)half(c+2));
            if (l->key == (int)half(c+4)) l->modulated = 2;
        } else if (l->modulated == 2) {
            l->key = approach(l->key, (int)half(c+8), (int)half(c+6));
        }
        l->pitch = pitch(table, l);
        return;
    }
    if (half(c) == 10) {
        /* Two pitch targets precede key-off. All three targets are offsets
         * from the original note, including the release bend. */
        if (t > half(c+2)) {
            unsigned stage = t > half(c+16) ? 2 : l->modulated - 1;
            static const unsigned targets[] = {6, 10, 12};
            static const unsigned steps[] = {4, 8, 14};
            int goal = l->sequence_base + (int16_t)half(c+targets[stage]);
            l->key = approach(l->key, goal, (int)half(c+steps[stage]));
            if (stage == 0 && l->key == goal) l->modulated = 2;
            l->pitch = pitch(table, l);
        }
        return;
    }
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
    int output = (int)((int64_t)v*level*l->gain / (32767*16384));
    unsigned field = l->field + (l->sequence_levels ? l->sequence_start : 0);
    unsigned period = l->ntsc_fields || half(l->control) == 1 || half(l->control) == 10 || l->sequence_levels
        ? field_sample(field+1)-field_sample(field) : SPU_RATE/60;
    if (++l->sample_in_field == period) {
        l->sample_in_field = 0;
        sound_field(table,l);
    }
    return output;
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

/* The field transition keys off these three selectors during preparation. */
static int transition_key_off_target(unsigned command)
{
    return command == 1096 || command == 1097 || command == 2285;
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
            if ((command != BFM_WOODS_GUARD_SOUND && command != BFM_WOODS_TERRAIN_HIT_FIRST &&
                 command != SHRINE_HIT_ACCEPTED && command != BFM_WOODS_CRYSTAL_HIT_SOUND &&
                 command != BFM_VALLEY_HINGE_IMPACT_SOUND) || half(d+14)) goto done;
            /* This is the same-command protection interval, not a delay
             * before the blow sounds or a key-off for the instrument. */
            size_t guard = ((uint64_t)half(d+12)*OUTPUT_RATE*1001u+59999u)/60000u;
            /* All three voices of the hinged rock share one protection interval. */
            if (count && (command != BFM_VALLEY_HINGE_IMPACT_SOUND ||
                          guard != clip->retrigger_frames)) goto done;
            clip->retrigger_frames = guard;
        }
        int rings = command == BFM_EARTH_SCROLL_RINGS_SOUND;
        if (!layer_open(s,d,&layers[count],rings)) goto done;
        layers[count].ntsc_fields = s->key == 38 || d[8] == 32 || (d[8] & 0x80) ||
            command == BFM_VALLEY_GATE_SLIDE_SOUND || half(layers[count].control) == 7 ||
            command == BFM_WOODS_CRYSTAL_BREAK_SOUND || command == BFM_WOODS_RESCUE_DEPART_SOUND ||
            command == BFM_WOODS_RESCUE_VANISH_SOUND ||
            (s->group == 4 && command == BFM_WOODS_FUSION_FOLLOWUP_SOUND);
        if (command == BFM_WOODS_FULL_CHARGE_SOUND || command == BFM_VALLEY_EARTH_RING_SOUND ||
            layers[count].ntsc_fields)
            layers[count].delay = field_sample(half(d+2));
        ++count;
        if (!d[11]) break;
        address += sizeof d;
    }
    if (command == BFM_WOODS_CHARGE_SOUND || command == SHRINE_APPEAR ||
        command == BFM_EARTH_SCROLL_RINGS_SOUND || transition_key_off_target(command)) {
        if (command == BFM_WOODS_CHARGE_SOUND) {
            if (count != 1 || !layers[0].gate || layers[0].slide || layers[0].modulated) goto done;
            layers[0].gate_samples = field_sample(layers[0].gate);
            layers[0].gate = 0;
        }
        /* The appearance plays beyond eight seconds; two ring voices loop
         * until the player closes the named scroll page. Keep their envelopes
         * and (for the rings) the staggered key-ons live. */
        clip->tone = calloc(1, sizeof *clip->tone);
        if (!clip->tone) goto done;
        clip->tone->count = count;
        for (unsigned i = 0; i < count; ++i) {
            clip->tone->initial[i] = layers[i];
            layers[i].sample = (BfmDecodedSpuSample){0};
        }
        memcpy(clip->tone->pitch_table, s->pitch, sizeof s->pitch);
        clip->id = command;
        result = 1;
    } else {
        result = render_layers(s,layers,count,command,clip);
    }
done:
    for (unsigned i=0; i<MAX_LAYERS; ++i) bfm_spu_sample_free(&layers[i].sample);
    return result;
}

static int finish_sequence(const Sources *s, Layer notes[], unsigned count,
                           unsigned command, Clip *clip, BfmDecodedSpuSample *shared_instrument)
{
    if (!s->menu_sequences && !transition_key_off_target(command))
        return render_layers(s, notes, count, command, clip);

    MenuSequence *sequence = calloc(1, sizeof *sequence + 2 * count * sizeof *notes);
    if (!sequence) return 0;
    sequence->count = count;
    memcpy(sequence->pitch_table, s->pitch, sizeof sequence->pitch_table);
    memcpy(sequence->notes, notes, count * sizeof *notes);
    if (notes[0].sequence_levels) {
        memcpy(sequence->levels, notes[0].sequence_levels, sizeof sequence->levels);
        for (unsigned i = 0; i < count; ++i) sequence->notes[i].sequence_levels = sequence->levels;
    }
    /* Compact phrases share their decoded instrument. Fixed records can
     * name a different bank and instrument on each note. */
    sequence->owned_samples = shared_instrument ? 1 : count;
    if (shared_instrument) {
        *shared_instrument = (BfmDecodedSpuSample){0};
    } else {
        for (unsigned i = 0; i < count; ++i) notes[i].sample = (BfmDecodedSpuSample){0};
    }
    clip->id = command;
    clip->menu_sequence = sequence;
    return 1;
}

/* Fixed records hold an interval, an independent gate and a complete
 * instrument. A zero interval ends scheduling with the current note. */
static int prepare_fixed_sequence(const Sources *s, unsigned command, const uint8_t track[12], Clip *clip)
{
    enum { MAX_NOTES = 66 };
    Layer notes[MAX_NOTES] = {0};
    uint32_t cursor = word(track);
    unsigned field = 0;
    int result = 0;
    for (unsigned i = 0; i < MAX_NOTES; ++i, cursor += 20) {
        uint8_t note[20];
        if (!read_data(s, cursor, note, sizeof note)) break;
        unsigned interval = half(note), gate = half(note+2), key = half(note+6);
        if (!key || key >= 127*256 || note[8] >= 128 || note[9] || note[13] ||
            interval > SEQUENCE_FIELDS - field || gate > SEQUENCE_FIELDS - field) break;
        Layer *voice = &notes[i];
        if (word(note+16)) {
            if (!read_data(s, word(note+16), voice->control, sizeof voice->control) ||
                half(voice->control) != 6 || !gate || half(voice->control+16) != gate) break;
            voice->modulated = 1;
        }
        if (!instrument_open(s, note[10], note[11], voice) ||
            (!gate && voice->sample.loop_start < voice->sample.frames)) break;
        voice->key = (int)key;
        voice->pitch = pitch(s->pitch, voice);
        if (!voice->pitch) break;
        voice->sequence_start = field;
        voice->sequence_gain = half(s->volume+2*note[8])*127u >> 7;
        voice->gain = sequence_level(voice, field);
        voice->ntsc_fields = 1;
        voice->delay = field_sample(field);
        voice->gate_samples = field_sample(field + gate) - voice->delay;
        if (!interval) {
            result = finish_sequence(s, notes, i+1, command, clip, NULL);
            break;
        }
        field += interval;
    }
    for (unsigned i = 0; i < MAX_NOTES; ++i) bfm_spu_sample_free(&notes[i].sample);
    return result;
}

static int prepare_sequence(const Sources *s, unsigned command, Clip *clip)
{
    /* The two HP refill phrases require 66 voices across their repeats. */
    enum { MAX_NOTES = 66 };
    uint8_t entry[4], track[12];
    if (!select_command(s, command, entry, NULL) || (entry[0] & 0x7f) != 5 || entry[2]) return 0;
    uint32_t tracks = entry[1] ? word(s->data+s->bytes-4) : 0x80068a54u;
    if (!read_data(s, tracks+12*entry[3], track, sizeof track)) return 0;
    if (!track[8] && (command == BFM_ROOM_AWARD_SOUND || command == BFM_VILLAGE_MINE_UNLOCK_SOUND))
        return prepare_fixed_sequence(s, command, track, clip);
    if (track[8] != 0x10 && track[8] != 0x12) return 0;
    uint32_t cursor = word(track), repeat_start = 0;
    unsigned repeats = 0, count = 0, field = 0, attenuation = 0;
    uint16_t levels[SEQUENCE_FIELDS+1];
    for (unsigned i=0; i<=SEQUENCE_FIELDS; ++i) levels[i] = 16383;
    const uint16_t *active_levels = NULL;
    int transpose = 0;
    Layer timbre = {0}, notes[MAX_NOTES];
    unsigned instrument = 0, bank = 0;
    int result = 0;
    for (unsigned steps=0; steps<128; ++steps) {
        uint8_t note[20];
        if ((!entry[1] && cursor >= effect_base) || !read_data(s,cursor,note,4)) break;
        unsigned op = note[3];
        if (op == 0x87) {
            if (count && !repeats && !note[0] && !note[1] && !note[2])
                result = finish_sequence(s,notes,count,command,clip,&timbre.sample);
            break;
        }
        if (op == 0x82) {
            if (repeats || !note[0] || note[1] || note[2]) break;
            repeats = note[0];
            cursor += 4;
            repeat_start = cursor;
            continue;
        }
        if (op == 0x84 || op == 0x86) {
            unsigned step = half(note+1);
            if (note[0] >= 128 || (op == 0x84 ? !step : step != 0)) break;
            int target = note[0]*16383 >> 7;
            if (op == 0x86) levels[field] = (uint16_t)target;
            for (unsigned i=field+1; i<=SEQUENCE_FIELDS; ++i)
                levels[i] = (uint16_t)approach(levels[i-1], target, (int)step);
            /* Notes from the previous phrase can still be ringing. */
            active_levels = levels;
            for (unsigned i=0; i<count; ++i) notes[i].sequence_levels = levels;
            cursor += 4;
            continue;
        }
        if (op == 0x85) {
            if (!repeats || note[1] >= 128 || note[2]) break;
            if (--repeats) {
                transpose += (note[0] < 128 ? (int)note[0] : (int)note[0]-256) * 16;
                attenuation += note[1];
                cursor = repeat_start;
            } else {
                transpose = 0;
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
            /* A phrase may redefine its voice priority without changing
             * instrument. The village well uses its mounted area bank. */
            if (note[8] > 2 || note[11]) break;
            if (timbre.sample.pcm) {
                if (note[9] != instrument || note[10] != bank) break;
            } else {
                bank = note[10];
                instrument = note[9];
                if (!instrument_open(s,bank,instrument,&timbre)) break;
            }
        }
        if (!timbre.sample.pcm) break;
        Layer *voice = &notes[count++];
        *voice = timbre;
        voice->key = (int)half(note) + transpose;
        voice->pitch = pitch(s->pitch,voice);
        unsigned gate = half(note+4), interval = half(note+6);
        if (op == 0x80 && word(note+16)) {
            if (!read_data(s,word(note+16),voice->control,14)) break;
            if (half(voice->control) == 8) {
                if (!pitch_slide(voice)) break;
                gate = voice->gate;
                voice->ntsc_fields = 1;
            } else {
                if (half(voice->control) != 10 ||
                    !read_data(s,word(note+16),voice->control,sizeof voice->control) ||
                    !half(voice->control+16)) break;
                gate = half(voice->control+16);
                voice->sequence_base = voice->key;
                voice->modulated = 1;
            }
        }
        if (!voice->pitch || (!gate && voice->sample.loop_start < voice->sample.frames) ||
            gate > SEQUENCE_FIELDS-field || interval > SEQUENCE_FIELDS-field) break;
        voice->delay = field_sample(field);
        voice->gate_samples = field_sample(field+gate)-voice->delay;
        voice->sequence_gain = half(s->volume+2*(note[2]-attenuation))*127u >> 7;
        voice->sequence_start = field;
        voice->sequence_levels = active_levels;
        voice->gain = sequence_level(voice,field);
        if (!interval) {
            /* The final note can end scheduling directly and ring out. */
            result = finish_sequence(s,notes,count,command,clip,&timbre.sample);
            break;
        }
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
                                           const unsigned commands[], size_t count, int with_common)
{
    Sources sources = {.disc = disc, .menu_sequences = member == 124};
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
        if (with_common) sources.common = bfm_archive_sound_bank_load(disc, "MAIN.CD", 2);
        if (sources.scene && (!with_common || sources.common) && load_tables(&sources)) {
            effects = calloc(1, sizeof *effects);
            for (size_t i = 0; effects && i < count; ++i) {
                unsigned command = commands[i];
                uint8_t entry[4];
                int prepared = select_command(&sources, command, entry, NULL) &&
                    ((entry[0] & 0x7f) == 5
                        ? prepare_sequence(&sources, command, &effects->clips[i])
                        : prepare(&sources, command, &effects->clips[i]));
                if (!prepared) {
                    bfm_bedroom_effects_free(effects);
                    effects = NULL;
                }
            }
        }
    }
    bfm_palace_sound_bank_free(sources.common);
    bfm_palace_sound_bank_free(sources.scene);
    free(data);
    return effects;
}

BfmBedroomEffects *bfm_village_knock_sound_load(const MusashiDiscMedia *disc)
{
    const unsigned commands[] = {
        BFM_VILLAGE_KNOCK_SOUND, BFM_VILLAGE_WELL_SOUND, BFM_WOODS_LENO_BARK_SOUND
    };
    return town_effects_load(disc, 2, commands, sizeof commands / sizeof *commands, 0);
}

BfmBedroomEffects *bfm_village_illness_effects_load(const MusashiDiscMedia *disc)
{
    /* SC03/124 selects the two door tones and the key presentation in its
     * own bank. The inventory uses MAIN/2, including both voices of Heal. */
    static const unsigned commands[] = {
        1300, 1301, BFM_ROOM_AWARD_SOUND,
        BFM_ROOM_MOVE_SOUND, BFM_ROOM_ACCEPT_SOUND, BFM_VILLAGE_MINE_STRIKE_SOUND,
        BFM_VILLAGE_MINE_UNLOCK_SOUND, 0x467, BFM_FIELD_MENU_ROW_SOUND, 0x46d, 0x46b, 0x46c, 0x469,
        BFM_FIELD_MENU_COMMIT_SOUND, BFM_FIELD_MENU_ERROR_SOUND,
        0x463, 0x464, 0x465, RESCUE_BP_STRIKE, HEAL_ARPEGGIO
    };
    BfmBedroomEffects *effects = town_effects_load(disc, 124, commands,
                                                  sizeof commands / sizeof *commands, 1);
    if (effects) {
        effects->replace_matching = 1;
        effects->menu_sequences = 1;
    }
    return effects;
}

BfmBedroomEffects *bfm_woods_box_sound_load(const MusashiDiscMedia *disc)
{
    enum { BOX_MEMBER = 11, BOX_RESOURCE = 146 };
    ForestPlayerArchive player;
    uint8_t row[4];
    if (!forest_player_archive_open(disc, &player) ||
        !forest_player_archive_read(&player, UINT32_C(0x800d3544), row, sizeof row)) return NULL;
    uint32_t room = word(row);
    if ((room & 4095u) != BOX_RESOURCE) return NULL;

    /* Mount the effects for Twinpeak. Terrain and climbing contacts use its
     * selector column; Rootrick's landing names an instrument in this area. */
    static const unsigned contacts[] = {
        BFM_WOODS_WATER_ENTRY_SOUND,
        BFM_ROOTRICK_IMPACT_SOUND, BFM_WALL_CLIMB_FIRST_SOUND, BFM_WALL_CLIMB_SECOND_SOUND,
        BFM_ROOTRICK_STUMBLE_LANDING_SOUND, BFM_WOODS_HURT_SOUND, BFM_WOODS_LUMINA_SWING_SOUND,
        BFM_FOREST_SWING_SOUND, BFM_WOODS_FUSION_FOLLOWUP_SOUND, BFM_WOODS_TERRAIN_HIT_FIRST,
        SHRINE_HIT_ACCEPTED, SCROLL_BLADE_RETURN
    };
    static const unsigned ceremony[] = {
        SHRINE_SHAKE, SHRINE_APPEAR, SHRINE_RAYS, SHRINE_FLASH, BFM_EARTH_SCROLL_RINGS_SOUND, SHRINE_FINISH
    };
    _Static_assert(5 + sizeof contacts / sizeof *contacts + sizeof ceremony / sizeof *ceremony <= CLEFT_CLIPS,
                   "Twinpeak clip capacity");
    Sources s = {.disc = disc, .group = (room >> 12) & 15u, .key = room >> 16};
    uint8_t *area = bfm_archive_effect_data(disc, "SC02.CD", BOX_MEMBER, &s.bytes);
    s.data = area;
    s.scene = bfm_archive_sound_bank_load(disc, "SC02.CD", BOX_MEMBER);
    s.common = bfm_archive_sound_bank_load(disc, "MAIN.CD", 2);
    BfmBedroomEffects *effects = NULL;
    if (area && s.scene && s.common && load_tables(&s))
        effects = calloc(1, sizeof *effects);
    const size_t count = sizeof contacts / sizeof *contacts;
    int prepared = effects && prepare(&s, BFM_WOODS_BOX_OPEN_SOUND, &effects->clips[0]) &&
                   prepare(&s, BFM_WOODS_BOX_CLOSE_SOUND, &effects->clips[1]) &&
                   prepare_sequence(&s, BFM_WOODS_BOX_ACCEPT_SOUND, &effects->clips[2]);
    for (size_t i = 0; prepared && i < count; ++i)
        prepared = prepare(&s, contacts[i], &effects->clips[3+i]);
    if (effects && (!prepared ||
        !prepare_sequence(&s, BFM_WOODS_PICKUP_SOUND, &effects->clips[3+count]))) {
        bfm_bedroom_effects_free(effects);
        effects = NULL;
    }
    if (effects) {
        /* Upon the accepted blow the shrine mounts the ceremony's bank 32.
         * The late restored-shrine cue uses the current score's instruments.
         * The ordinary room contacts above keep their original bank 24. */
        s.score = bfm_archive_sound_bank_load(disc, "SC03.CD", 45);
        if (s.score) prepare(&s, SHRINE_RESTORED, &effects->clips[4+count]);
        bfm_palace_sound_bank_free(s.score);
        s.score = NULL;
        bfm_palace_sound_bank_free(s.scene);
        s.scene = bfm_archive_sound_bank_load(disc, "SC03.CD", 59);
        s.key = 32;
        if (s.scene) {
            for (size_t i = 0; i < sizeof ceremony / sizeof *ceremony; ++i)
                prepare(&s, ceremony[i], &effects->clips[5+count+i]);
        }
    }
    bfm_palace_sound_bank_free(s.common);
    bfm_palace_sound_bank_free(s.scene);
    free(area);
    return effects;
}

static int prepare_equipment_effect(Sources *s, const ForestPlayerArchive *player,
                                    uint32_t resource, unsigned command, Clip *clip)
{
    uint8_t bank[8];
    if (!forest_player_archive_read(player, resource, bank, sizeof bank)) return 0;
    bfm_palace_sound_bank_free(s->scene);
    s->scene = bfm_archive_sound_bank_load(s->disc, "MAIN.CD", word(bank));
    s->key = word(bank+4);
    return s->scene && prepare(s, command, clip);
}

static int prepare_bank_field(Sources *s, const ForestPlayerArchive *player, BfmBedroomEffects *effects)
{
    /* The bank's defense sound uses the active field VAB. The hurt and
     * award use common instruments. Hop sounds through its equipment bank. */
    s->common = bfm_archive_sound_bank_load(s->disc, "MAIN.CD", 2);
    if (!s->common || !prepare(s, BFM_WOODS_GUARD_SOUND, &effects->clips[0]) ||
        !prepare(s, BFM_WOODS_HURT_SOUND, &effects->clips[1]) ||
        !prepare(s, BFM_WOODS_SHRINK_SOUND, &effects->clips[2])) return 0;
    effects->replace_matching = 1;
    return prepare_equipment_effect(s, player, UINT32_C(0x800d3794),
                                    BFM_WOODS_HOP_SOUND, &effects->clips[3]);
}

static int prepare_valley_field(Sources *s, const ForestPlayerArchive *player, BfmBedroomEffects *effects)
{
    /* Keep the canyon bank mounted through the four-layer gate leaf, the
     * crystal and the captive's departure. The blade and BP sequences use
     * the common bank. Only the two Earth sounds switch to equipment. */
    static const unsigned field_sounds[] = {
        BFM_VALLEY_GATE_SLIDE_SOUND, BFM_VALLEY_SOLDIER_LAND_SOUND,
        BFM_WOODS_CRYSTAL_HIT_SOUND, BFM_WOODS_CRYSTAL_BREAK_SOUND,
        BFM_VALLEY_KNITTER_DEPART_SOUND, BFM_VALLEY_KNITTER_VANISH_SOUND,
        BFM_VALLEY_AWARD_RAISE_SOUND, BFM_WOODS_HURT_SOUND
    };
    size_t slot = 5;
    _Static_assert(5 + sizeof field_sounds / sizeof *field_sounds +
                   sizeof woods_swings / sizeof *woods_swings + 2 <= CLEFT_CLIPS,
                   "Valley effects must fit the prepared bank");
    s->common = bfm_archive_sound_bank_load(s->disc, "MAIN.CD", 2);
    if (!s->common || !prepare(s, BFM_VALLEY_BOULDER_START_SOUND, &effects->clips[0]) ||
        !prepare(s, BFM_VALLEY_BOULDER_IMPACT_SOUND, &effects->clips[1]) ||
        !prepare(s, BFM_VALLEY_HINGE_IMPACT_SOUND, &effects->clips[4])) return 0;
    for (size_t i = 0; i < sizeof field_sounds / sizeof *field_sounds; ++i)
        if (!prepare(s, field_sounds[i], &effects->clips[slot++])) return 0;
    for (size_t i = 0; i < sizeof woods_swings / sizeof *woods_swings; ++i)
        if (!prepare(s, woods_swings[i], &effects->clips[slot++])) return 0;
    return prepare_sequence(s, RESCUE_BP_STRIKE, &effects->clips[slot]) &&
           prepare_sequence(s, RESCUE_BP_ARPEGGIO, &effects->clips[slot+1]) &&
           prepare_equipment_effect(s, player, UINT32_C(0x800d3854),
                                    BFM_VALLEY_EARTH_LANDING_SOUND, &effects->clips[2]) &&
           prepare(s, BFM_VALLEY_EARTH_RING_SOUND, &effects->clips[3]);
}

static BfmBedroomEffects *forest_field_effects(const MusashiDiscMedia *disc, unsigned room,
                                              unsigned member)
{
    /* The resident room descriptor supplies the area selector and VAB key.
     * The archive member supplies its remap directory, programs and samples. */
    ForestPlayerArchive player;
    uint8_t field[4];
    if (!forest_player_archive_open(disc, &player) ||
        !forest_player_archive_read(&player, UINT32_C(0x800d34ac) + 4*(room & 4095u),
                                    field, sizeof field)) return NULL;
    Sources s = {.disc = disc, .group = half(field) >> 12, .key = half(field+2)};
    uint8_t *area = bfm_archive_effect_data(disc, "SC03.CD", member, &s.bytes);
    s.data = area;
    s.scene = bfm_archive_sound_bank_load(disc, "SC03.CD", member);
    BfmBedroomEffects *effects = NULL;
    if (area && s.scene && load_tables(&s)) effects = calloc(1, sizeof *effects);
    if (effects) {
        int valid;
        if (room == 0x3014) {
            valid = prepare_bank_field(&s, &player, effects);
        } else if (room == 0x302b) {
            valid = prepare_valley_field(&s, &player, effects);
        } else {
            valid = prepare(&s, BFM_WOODS_LENO_BARK_SOUND, &effects->clips[0]) &&
                    prepare(&s, BFM_MAZE_GUIDE_SNIFF_SOUND, &effects->clips[1]) &&
                    prepare_sequence(&s, BFM_MAZE_GUIDE_ABSENT_SOUND, &effects->clips[2]);
        }
        if (!valid) {
            bfm_bedroom_effects_free(effects);
            effects = NULL;
        }
    }
    bfm_palace_sound_bank_free(s.common);
    bfm_palace_sound_bank_free(s.scene);
    free(area);
    return effects;
}

BfmBedroomEffects *bfm_forest_bank_effects_load(const MusashiDiscMedia *disc)
{
    return forest_field_effects(disc, 0x3014, 6);
}

BfmBedroomEffects *bfm_maze_guide_effects_load(const MusashiDiscMedia *disc)
{
    return forest_field_effects(disc, 0x301b, 7);
}

BfmBedroomEffects *bfm_valley_effects_load(const MusashiDiscMedia *disc)
{
    return forest_field_effects(disc, 0x302b, 31);
}

BfmBedroomEffects *bfm_nightmare_sound_load(const MusashiDiscMedia *disc)
{
    const unsigned commands[] = {
        BFM_NIGHTMARE_WAKE_SOUND, BFM_NIGHTMARE_BED_LANDING_SOUND, BFM_NIGHTMARE_FLOOR_LANDING_SOUND
    };
    return town_effects_load(disc, 3, commands, sizeof commands / sizeof *commands, 0);
}

BfmBedroomEffects *bfm_bakery_effects_load(const MusashiDiscMedia *disc)
{
    /* The menu uses the common instruments. Completing a trade uses the
     * effects bank already mounted for the lower village. The insufficient
     * funds request's argument 3 carries no volume, pan or pitch override. */
    const unsigned commands[] = {1518, 1114, 1135, 1139, 1140, 1119, 1138};
    BfmBedroomEffects *effects = town_effects_load(disc, 1, commands, sizeof commands / sizeof *commands, 1);
    if (effects) effects->replace_matching = 1;
    return effects;
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

BfmBedroomEffects *bfm_castle_menu_effects_load(const MusashiDiscMedia *disc)
{
    static const unsigned commands[] = {
        BFM_ROOM_MOVE_SOUND, BFM_ROOM_ACCEPT_SOUND, 0x45a, 0x45d, 0x45f, 0x473, 0x474
    };
    BfmBedroomEffects *effects = common_effects_load(disc, commands, sizeof commands / sizeof *commands);
    if (effects) effects->replace_matching = 1;
    return effects;
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
    prepare_sequence(&s, RESCUE_BP_STRIKE, &effects->clips[CLEFT_BP_STRIKE]);
    prepare_sequence(&s, RESCUE_BP_ARPEGGIO, &effects->clips[CLEFT_BP_ARPEGGIO]);
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
    if (has_player_bank) {
        static const struct { uint32_t row; unsigned command, slot; } skills[] = {
            { UINT32_C(0x800d3794), BFM_WOODS_HOP_SOUND, CLEFT_HOP },
            { UINT32_C(0x800d379c), BFM_WOODS_SATIATE_RECOVERY_SOUND, CLEFT_RECOVERY }
        };
        for (unsigned i = 0; i < sizeof skills / sizeof *skills; ++i) {
            if (!forest_player_archive_read(&player, skills[i].row, row, sizeof row)) continue;
            s.key = word(row + 4);
            s.scene = bfm_archive_sound_bank_load(disc, "MAIN.CD", word(row));
            if (s.scene) prepare(&s, skills[i].command, &effects->clips[skills[i].slot]);
            bfm_palace_sound_bank_free(s.scene);
            s.scene = NULL;
        }
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
            for (unsigned j = 0; j < clip->tone->count; ++j)
                bfm_spu_sample_free(&clip->tone->initial[j].sample);
            free(clip->tone);
        }
        if (clip->menu_sequence) {
            for (unsigned j = 0; j < clip->menu_sequence->owned_samples; ++j)
                bfm_spu_sample_free(&clip->menu_sequence->notes[j].sample);
            free(clip->menu_sequence);
        }
        free(clip->mono);
    }
    free(e);
}

static void start_voice(Playback *p, const Clip *clip, unsigned command)
{
    *p = (Playback){.clip=clip, .command=command};
    if (clip->menu_sequence) {
        MenuSequence *sequence = clip->menu_sequence;
        memcpy(sequence->notes + sequence->count, sequence->notes,
               sequence->count * sizeof *sequence->notes);
    }
    if (clip->tone) memcpy(p->layers, clip->tone->initial, clip->tone->count * sizeof *p->layers);
}

void bfm_bedroom_effects_stop(BfmBedroomEffects *effects)
{
    if (effects) memset(effects->playing, 0, sizeof effects->playing);
}

void bfm_bedroom_effects_start(BfmBedroomEffects *e, unsigned command)
{
    if (!e || !command) return;
    if (command == BFM_ROOM_MENU_DUCK || command == BFM_ROOM_MENU_RESTORE) {
        for (unsigned i = 0; i < VOICES; ++i) {
            Playback *p = &e->playing[i];
            if (p->clip && p->clip->menu_sequence)
                p->menu_target = command == BFM_ROOM_MENU_DUCK ? SEQUENCE_FULL_GAIN : 0;
        }
        return;
    }
    if (command == BFM_FIELD_MENU_HEAL_SOUND && e->menu_sequences) {
        /* The item service starts both mounted sequences on the same tick. */
        bfm_bedroom_effects_start(e, RESCUE_BP_STRIKE);
        bfm_bedroom_effects_start(e, HEAL_ARPEGGIO);
        return;
    }
    if (command == BFM_WOODS_RESCUE_BP_SOUND) {
        /* The BP refill starts two common sequences on the same sound tick. */
        bfm_bedroom_effects_start(e, RESCUE_BP_STRIKE);
        bfm_bedroom_effects_start(e, RESCUE_BP_ARPEGGIO);
        return;
    }
    unsigned clip_command = command;
    if (command >= BFM_WOODS_TERRAIN_HIT_FIRST && command <= BFM_WOODS_TERRAIN_HIT_LAST &&
        (e->terrain_aliases & (1u << (command - BFM_WOODS_TERRAIN_HIT_FIRST))))
        clip_command = BFM_WOODS_GUARD_SOUND;
    if (command == BFM_WOODS_STOP_CHARGE_SOUND || command == BFM_EARTH_SCROLL_STOP_RINGS_SOUND ||
        ((command >> 16) == 1 && transition_key_off_target(command & 0xffffu))) {
        for (unsigned i = 0; i < VOICES; ++i) {
            Playback *p = &e->playing[i];
            if (!p->clip || p->command != (command & 0xffffu)) continue;
            MenuSequence *sequence = p->clip->menu_sequence;
            if (!sequence && !p->clip->tone) {
                p->clip = NULL;
                continue;
            }
            unsigned count = sequence ? sequence->count : p->clip->tone->count;
            Layer *notes = sequence ? sequence->notes + count : p->layers;
            for (unsigned j = 0; j < count; ++j) {
                if (notes[j].delay) notes[j].done = 1;
                else bfm_spu_envelope_release(&notes[j].envelope);
            }
        }
        return;
    }
    for (unsigned i=0; i<sizeof e->clips/sizeof *e->clips; ++i) if (e->clips[i].id == clip_command) {
        /* Repeated menu commands and rapid Gunshot fire restart the active
         * instance of that command. Surface aliases keep separate cursors
         * even when the mounted sample is the same. */
        if (e->replace_matching || i >= 3 || e->clips[i].menu_sequence ||
            command == BFM_FOREST_GUNSHOT_SOUND) {
            for (unsigned j=0; j<VOICES; ++j) {
                Playback *p = &e->playing[j];
                if (p->clip && p->command == command &&
                    !(p->clip->tone && p->layers[0].envelope.stage == BFM_SPU_ENVELOPE_RELEASE)) {
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

static void mix_block(BfmBedroomEffects *e, int16_t *stereo, size_t frames)
{
    for (unsigned j=0; j<VOICES; ++j) {
        Playback *p = &e->playing[j];
        if (!p->clip) continue;
        if (p->clip->tone || p->clip->menu_sequence) {
            KeyedTone *tone = p->clip->tone;
            MenuSequence *sequence = p->clip->menu_sequence;
            unsigned count = sequence ? sequence->count : tone->count;
            Layer *notes = sequence ? sequence->notes + count : p->layers;
            const uint8_t *table = sequence ? sequence->pitch_table : tone->pitch_table;
            for (size_t i = 0; i < frames; ++i) {
                int16_t value;
                if (!next_frame(table, notes, count, &p->resampler, &value)) break;
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

static void advance_menu_gain(Playback *p)
{
    if (!p->clip || !p->clip->menu_sequence) return;
    MenuSequence *sequence = p->clip->menu_sequence;
    Layer *notes = sequence->notes + sequence->count;
    for (unsigned i = 0; i < sequence->count; ++i) {
        Layer *note = &notes[i];
        if (note->done || note->menu_gain_latched) continue;
        if (note->menu_attenuation != p->menu_staged) {
            note->menu_attenuation = p->menu_staged;
            note->gain = sequence_level(note, note->sequence_start + note->field);
        }
        if (note->envelope.stage == BFM_SPU_ENVELOPE_RELEASE) note->menu_gain_latched = 1;
    }
    /* The previous service's gain reaches the notes before the next step. */
    p->menu_staged = (unsigned)approach((int)p->menu_staged, (int)p->menu_target, MENU_RAMP_STEP);
}

void bfm_bedroom_effects_mix(BfmBedroomEffects *e, int16_t *stereo, size_t frames)
{
    if (!e || !stereo) return;
    if (!e->menu_sequences) {
        mix_block(e, stereo, frames);
        return;
    }
    /* Convert 37.8 kHz output frames to 60000/1001 sound services per second. */
    enum { FIELD_STEP = 100, FIELD_PERIOD = 63063 };
    while (frames) {
        size_t n = (FIELD_PERIOD - e->sound_phase + FIELD_STEP - 1) / FIELD_STEP;
        if (n > frames) n = frames;
        mix_block(e, stereo, n);
        stereo += 2*n;
        frames -= n;
        e->sound_phase += (unsigned)n * FIELD_STEP;
        if (e->sound_phase >= FIELD_PERIOD) {
            e->sound_phase -= FIELD_PERIOD;
            for (unsigned i = 0; i < VOICES; ++i) advance_menu_gain(&e->playing[i]);
        }
    }
}
