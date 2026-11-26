/* SPDX-License-Identifier: MIT */
#include "bedroom_effects.h"
#include "bedroom_record.h"
#include "bedroom_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"
#include <stdlib.h>
#include <string.h>

enum { SPU_RATE = 44100, OUTPUT_RATE = 37800, VOICES = 24, MAX_LAYERS = 3 };
static const uint32_t effect_base = 0x800c7f08u;
typedef struct {
    unsigned id;
    size_t frames;
    int16_t *mono;
} Clip;
typedef struct {
    const Clip *clip;
    size_t frame;
} Playback;
struct BfmBedroomEffects {
    Clip clips[7];
    Playback playing[VOICES];
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
    unsigned delay, gain, center, pitch, field, sample_in_field;
    int key, modulated, done;
    size_t phase;
} Layer;

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
static int description(const Sources *s, unsigned command, uint32_t *address)
{
    uint8_t entry[4], header[28], pointer[4], value[2];
    if (!read_data(s, 0x8006451cu+4*command, entry, sizeof entry) ||
        !read_data(s, 0x800c8500u, header, sizeof header) || half(header) != s->group) return 0;
    if (entry[0] == 6) {
        unsigned first = half(header+4), columns = half(header+14), row = entry[3];
        if (entry[2] != 3 || s->key < first || s->key-first >= half(header+2) ||
            row >= half(header+12) || !read_data(s, word(header+8)+2*(s->key-first), value, 2)) return 0;
        unsigned column = half(value);
        if (!column || column > columns ||
            !read_data(s, word(header+16)+2*(row*columns+column-1), value, 2) ||
            !read_data(s, 0x8006451cu+4*half(value), entry, 4)) return 0;
    }
    /* Direct common menu effects preserve their high service-mode flag
     * in the resident slot. The lower six bits select the instrument
     * descriptor and run the same envelope. */
    if ((entry[0] != 1 && entry[0] != 0x41) || (entry[1] && entry[1] != s->group)) return 0;
    uint32_t tables = entry[1] ? word(header+20) : 0x80068304u;
    if (!read_data(s, tables+4*entry[2], pointer, 4)) return 0;
    *address = word(pointer)+20u*entry[3];
    return 1;
}

static unsigned pitch(const Sources *s, int key, unsigned center)
{
    key += (60-(int)center)*256;
    if (key < 0 || key >= 127*256) return 0;
    const uint8_t *p = s->pitch+2*(key>>8);
    unsigned low = half(p), high = half(p+2);
    return low + ((high-low)*(unsigned)(key&255) >> 8);
}

static int layer_open(const Sources *s, const uint8_t d[20], Layer *l)
{
    uint8_t map[12], pair[2];
    unsigned bank = d[8], instrument = d[9];
    if ((bank && bank != s->key) || d[4] || d[7] || d[5]>=128 || d[6]>=128 ||
        !read_data(s, 0x80064d44u+12*bank, map, sizeof map) || instrument >= map[4] ||
        !read_data(s, word(map)+2*instrument, pair, 2)) return 0;
    const BfmPalaceBank *vab = bank ? s->scene : s->common;
    BfmPalaceTone tone;
    BfmSpuSample encoded;
    if (!bfm_room_effect_tone(vab, pair[0], pair[1], &tone) || tone.tuning ||
        !bfm_palace_sound_sample(vab, tone.sample, &encoded) ||
        !bfm_spu_sample_decode(encoded.data, encoded.bytes, &l->sample)) return 0;
    l->delay = half(d+2) * (SPU_RATE/60);
    l->gain = ((half(s->volume+2*d[6])*127 >> 7)*127) >> 7;
    l->center = tone.center;
    l->key = d[5]*256;
    l->pitch = pitch(s,l->key,l->center);
    bfm_spu_envelope_start(&l->envelope,(uint16_t)tone.adsr1,(uint16_t)tone.adsr2);
    if (word(d+12)) return 0;
    if (word(d+16)) {
        if (!read_data(s, word(d+16), l->control, sizeof l->control) || half(l->control)!=6 ||
            !half(l->control+4) || !half(l->control+8) || !half(l->control+16)) return 0;
        l->modulated = 1;
    }
    return l->pitch && (l->modulated || l->sample.loop_start == l->sample.frames);
}

static int approach(int value, int target, int step)
{
    if (value < target) return target-value < step ? target : value+step;
    return value-target < step ? target : value-step;
}

static void sound_field(const Sources *s, Layer *l)
{
    const uint8_t *c = l->control;
    unsigned t = ++l->field;
    if (!l->modulated || t <= half(c+2)) return;
    if (t == half(c+16)) bfm_spu_envelope_release(&l->envelope);
    if (t > half(c+16)) l->key = approach(l->key,c[13]*256,(int)half(c+14));
    else if (l->modulated == 1) {
        l->key = approach(l->key,c[7]*256,(int)half(c+4));
        if (l->key == c[7]*256) l->modulated = 2;
    } else l->key = approach(l->key,c[11]*256,(int)half(c+8));
    l->pitch = pitch(s,l->key,l->center);
}

static int next_sample(const Sources *s, Layer *l)
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
    if (++l->sample_in_field == SPU_RATE/60) {
        l->sample_in_field = 0;
        sound_field(s,l);
    }
    return (int)((int64_t)v*level*l->gain / (32767*16384));
}

static int16_t limited(int value)
{
    return (int16_t)(value > INT16_MAX ? INT16_MAX : value < INT16_MIN ? INT16_MIN : value);
}

static int prepare(const Sources *s, unsigned command, Clip *clip)
{
    uint32_t address;
    Layer layers[MAX_LAYERS] = {0};
    unsigned count = 0;
    int result = 0;
    if (!description(s, command, &address)) return 0;
    for (;;) {
        uint8_t d[20];
        if (count == MAX_LAYERS || !read_data(s,address,d,sizeof d) ||
            !layer_open(s,d,&layers[count])) goto done;
        ++count;
        if (!d[11]) break;
        address += sizeof d;
    }
    size_t capacity = OUTPUT_RATE;
    clip->mono = malloc(capacity * sizeof *clip->mono);
    if (!clip->mono) goto done;
    int previous = 0, phase = 0;
    for (;;) {
        int current = 0, active = 0;
        for (unsigned i = 0; i < count; ++i) {
            current += next_sample(s, &layers[i]);
            active |= !layers[i].done;
        }
        if (phase <= 0) {
            if (clip->frames == capacity) {
                if (capacity >= OUTPUT_RATE*8) goto done;
                capacity *= 2;
                int16_t *p = realloc(clip->mono, capacity*sizeof *p);
                if (!p) goto done;
                clip->mono = p;
            }
            clip->mono[clip->frames++] = limited(current + (previous-current)*(-phase)/OUTPUT_RATE);
            phase += SPU_RATE;
        }
        if (!active) break;
        previous = current;
        phase -= OUTPUT_RATE;
    }
    clip->id = command;
    result = 1;
done:
    for (unsigned i=0; i<MAX_LAYERS; ++i) bfm_spu_sample_free(&layers[i].sample);
    return result;
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
        !read_data(&s,0x8006aa30u,s.volume,sizeof s.volume) ||
        !read_data(&s,0x8006ab30u,s.pitch,sizeof s.pitch) ||
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

void bfm_bedroom_effects_free(BfmBedroomEffects *e)
{
    if (!e) return;
    for (unsigned i=0; i<sizeof e->clips/sizeof *e->clips; ++i) free(e->clips[i].mono);
    free(e);
}

void bfm_bedroom_effects_start(BfmBedroomEffects *e, unsigned command)
{
    if (!e || !command) return;
    for (unsigned i=0; i<sizeof e->clips/sizeof *e->clips; ++i) if (e->clips[i].id == command) {
        /* The resident menu reissues an identical command in its own voice
         * slot. Distinct button commands may overlap until their releases. */
        if (i >= 3) {
            for (unsigned j=0; j<VOICES; ++j) {
                if (e->playing[j].clip == &e->clips[i]) {
                    e->playing[j].frame = 0;
                    return;
                }
            }
        }
        for (unsigned j=0; j<VOICES; ++j) if (!e->playing[j].clip) {
            e->playing[j] = (Playback){&e->clips[i],0};
            return;
        }
    }
}

void bfm_bedroom_effects_mix(BfmBedroomEffects *e, int16_t *stereo, size_t frames)
{
    if (!e || !stereo) return;
    for (unsigned j=0; j<VOICES; ++j) {
        Playback *p = &e->playing[j];
        if (!p->clip) continue;
        size_t left = p->clip->frames-p->frame;
        size_t n = left < frames ? left : frames;
        for (size_t i=0; i<n; ++i) {
            int value = p->clip->mono[p->frame++];
            stereo[2*i] = limited(stereo[2*i]+value);
            stereo[2*i+1] = limited(stereo[2*i+1]+value);
        }
        if (p->frame == p->clip->frames) p->clip = NULL;
    }
}
