/* SPDX-License-Identifier: MIT */
#include "tower_bell_sound.h"
#include "bedroom_record.h"
#include "bedroom_score.h"
#include "palace_sound_bank.h"
#include "spu_envelope.h"
#include "spu_sample.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { MEMBER=84, LANDING=1202, IMPACT=1203, CLIMB=1213, STOP=4,
       SOURCE_RATE=44100, OUTPUT_RATE=37800, SEQUENCE_VOICES=8, NOTE_COUNT=8 };
static const uint32_t area_base = UINT32_C(0x800c7f08);

typedef struct {
    BfmDecodedSpuSample sample;
    BfmPalaceTone tone;
    unsigned bank, instrument;
} Instrument;

typedef struct {
    const Instrument *sound;
    unsigned pitch, level, gate, delay, interval, next;
} Note;

typedef struct {
    const Note *note;
    BfmSpuEnvelope envelope;
    uint64_t phase;
    unsigned gate, delay;
} Voice;

struct BfmTowerBellSound {
    Instrument instruments[5];
    unsigned instrument_count;
    Note sequence[NOTE_COUNT], direct[2];
    Voice sequence_voice[SEQUENCE_VOICES], direct_voice[2];
    unsigned field_clock, wait, next, gain, target, center_pan;
    int playing, previous, resample;
};

typedef struct {
    const MusashiDiscMedia *disc;
    const BfmPalaceBank *bank;
    const uint8_t *area;
    size_t bytes;
    uint8_t volumes[256], pitches[168];
} Sources;

static const uint8_t *area_at(const Sources *s, uint64_t address, size_t bytes)
{
    if (address < area_base || bytes > s->bytes || address-area_base > s->bytes-bytes) return NULL;
    return s->area+(size_t)(address-area_base);
}

static Instrument *instrument(BfmTowerBellSound *sound, const Sources *s, unsigned bank, unsigned number)
{
    for (unsigned i=0; i<sound->instrument_count; ++i) {
        Instrument *p = &sound->instruments[i];
        if (p->bank == bank && p->instrument == number) return p;
    }
    if (sound->instrument_count >= sizeof sound->instruments/sizeof sound->instruments[0])
        return NULL;
    uint8_t map[12], pair[2];
    Instrument *p = &sound->instruments[sound->instrument_count];
    BfmSpuSample encoded;
    if (!bfm_bedroom_resident_data(s->disc, 0x80064d44u+12*bank, map, sizeof map) ||
        number >= map[4] || u32le(map) > UINT32_MAX-2*number ||
        !bfm_bedroom_resident_data(s->disc, u32le(map)+2*number, pair, sizeof pair) ||
        !bfm_room_effect_tone(s->bank,pair[0],pair[1],&p->tone) ||
        !bfm_palace_sound_sample(s->bank,p->tone.sample,&encoded) ||
        !bfm_spu_sample_decode(encoded.data,encoded.bytes,&p->sample)) return NULL;
    p->bank=bank;
    p->instrument=number;
    ++sound->instrument_count;
    return p;
}

static int note_params(BfmTowerBellSound *sound, const Sources *s, Note *note,
                       unsigned bank, unsigned program, unsigned key, unsigned level)
{
    if (bank != 3 || level >= 128) return 0;
    const Instrument *p = instrument(sound,s,bank,program);
    if (!p) return 0;
    int index = (int)key + (60-(int)p->tone.center)*256 - 2*p->tone.tuning;
    if (index < 0 || index >= 83*256) return 0;
    const uint8_t *pitch = s->pitches+2*(index/256);
    unsigned low = u16le(pitch), high = u16le(pitch+2);
    unsigned fraction = (unsigned)index & 255;
    note->pitch = (low*(256-fraction)+high*fraction)/256;
    note->level = u16le(s->volumes+2*level)*127u >> 7;
    note->sound = p;
    return note->pitch && note->pitch <= 16383 && note->level <= 16383;
}

static int read_direct(BfmTowerBellSound *sound, const Sources *s, unsigned command,
                       const uint8_t *entry, const uint8_t *dir)
{
    if (entry[0] != 1 || entry[1] != u16le(dir)) return 0;
    const uint8_t *group = area_at(s,(uint64_t)u32le(dir+20)+4*entry[2],4);
    const uint8_t *row = group ? area_at(s,(uint64_t)u32le(group)+20*entry[3],20) : NULL;
    Note *n = &sound->direct[command-LANDING];
    if (!row || row[4] || row[7] || row[11] || u32le(row+12) ||
        !note_params(sound,s,n,row[8],row[9],row[5]*256u,row[6])) return 0;
    n->delay = u16le(row+2);
    n->level = n->level*127u >> 7;
    if (u32le(row+16)) {
        const uint8_t *control = area_at(s,u32le(row+16),4);
        /* The bell's delayed tone keeps its pitch and releases on this field. */
        if (!control || u16le(control)!=2 || !u16le(control+2)) return 0;
        n->gate = u16le(control+2);
    }
    return n->gate || n->sound->sample.frames == n->sound->sample.loop_start;
}

static int read_sequence(BfmTowerBellSound *sound, const Sources *s,
                         const uint8_t *entry, const uint8_t *dir)
{
    if (entry[0]!=5 || entry[1]!=u16le(dir) || entry[2]) return 0;
    const uint8_t *table=area_at(s,(uint64_t)u32le(dir+24)+12*entry[3],16);
    if (!table || table[8] || u32le(table+12) < u32le(table) ||
        u32le(table+12)-u32le(table)!=20*NOTE_COUNT) return 0;
    const uint8_t *rows=area_at(s,u32le(table),20*NOTE_COUNT);
    if (!rows) return 0;
    for (unsigned i=0;i<NOTE_COUNT;++i) {
        const uint8_t *r=rows+20*i;
        Note *n=&sound->sequence[i];
        if (!u16le(r) || !u16le(r+6) || r[9]!=64 || r[13]>NOTE_COUNT ||
            u32le(r+16) || !note_params(sound,s,n,r[10],r[11],u16le(r+6),r[8])) return 0;
        n->gate=u16le(r+2);
        n->interval=u16le(r);
        n->next=r[13] ? (unsigned)r[13]-1 : i+1;
        if (n->next>=NOTE_COUNT) return 0;
    }
    return 1;
}

BfmTowerBellSound *bfm_tower_bell_sound_load(const MusashiDiscMedia *disc)
{
    if (!disc) return NULL;
    Sources s={.disc=disc};
    BfmTowerBellSound *sound=calloc(1,sizeof *sound);
    if (!sound) return NULL;
    uint8_t *data=bfm_bedroom_effect_data(disc,MEMBER,&s.bytes);
    s.area=data;
    BfmPalaceBank *bank=bfm_archive_sound_bank_load(disc,"SC01.CD",MEMBER);
    s.bank=bank;
    uint8_t entry[4], pan[2];
    int ok=data && s.bytes>=28 && bank &&
        bfm_bedroom_resident_data(disc,0x8006aa30u,s.volumes,sizeof s.volumes) &&
        bfm_bedroom_resident_data(disc,0x8006ab30u,s.pitches,sizeof s.pitches) &&
        bfm_bedroom_resident_data(disc,0x8007321eu,pan,sizeof pan);
    if (ok) {
        const uint8_t *dir=data+s.bytes-28;
        sound->center_pan=u16le(pan);
        for (unsigned command=LANDING;ok && command<=IMPACT;++command)
            ok=bfm_bedroom_resident_data(disc,0x8006451cu+4*command,entry,sizeof entry) &&
               read_direct(sound,&s,command,entry,dir);
        ok=ok && sound->center_pan<=16384 &&
            bfm_bedroom_resident_data(disc,0x8006451cu+4*CLIMB,entry,sizeof entry) &&
            read_sequence(sound,&s,entry,dir);
    }
    bfm_palace_sound_bank_free(bank);
    free(data);
    if (ok) return sound;
    bfm_tower_bell_sound_free(sound);
    return NULL;
}

void bfm_tower_bell_sound_free(BfmTowerBellSound *sound)
{
    if (sound) {
        for (unsigned i=0;i<sound->instrument_count;++i)
            bfm_spu_sample_free(&sound->instruments[i].sample);
        free(sound);
    }
}

static void start(Voice *v, const Note *n)
{
    *v=(Voice){.note=n,.gate=n->gate,.delay=n->delay};
    bfm_spu_envelope_start(&v->envelope,n->sound->tone.adsr1,n->sound->tone.adsr2);
}

void bfm_tower_bell_sound_command(BfmTowerBellSound *s, unsigned command, unsigned argument)
{
    if (!s) return;
    if ((command==IMPACT || command==LANDING) && !argument) {
        start(&s->direct_voice[command-LANDING],&s->direct[command-LANDING]);
    } else if (command==CLIMB && (argument & ~127u)==0x1000) {
        s->target=(argument & 127u)*16383u >> 7;
        if (!s->playing) {
            s->gain=s->target;
            s->playing=1;
            s->next=s->wait=0;
        }
    } else if (command==STOP && argument==CLIMB) {
        s->playing=0;
        for (unsigned i=0;i<SEQUENCE_VOICES;++i)
            if (s->sequence_voice[i].note) bfm_spu_envelope_release(&s->sequence_voice[i].envelope);
    }
}

static void field(BfmTowerBellSound *s)
{
    for (unsigned i=0;i<SEQUENCE_VOICES+2;++i) {
        Voice *v=i<SEQUENCE_VOICES ? &s->sequence_voice[i] : &s->direct_voice[i-SEQUENCE_VOICES];
        if (!v->note) continue;
        if (v->delay) --v->delay;
        else if (v->gate && !--v->gate) bfm_spu_envelope_release(&v->envelope);
    }
    if (!s->playing) return;
    if (s->gain<s->target) s->gain+=s->target-s->gain>256 ? 256 : s->target-s->gain;
    else if (s->gain>s->target) s->gain-=s->gain-s->target>256 ? 256 : s->gain-s->target;
    if (s->wait && --s->wait) return;
    const Note *n=&s->sequence[s->next];
    for (unsigned i=0;i<SEQUENCE_VOICES;++i)
        if (!s->sequence_voice[i].note) { start(&s->sequence_voice[i],n); break; }
    s->next=n->next;
    s->wait=n->interval;
}

static int voice_sample(Voice *v, unsigned level)
{
    if (!v->note || v->delay) return 0;
    const Note *n=v->note;
    const BfmDecodedSpuSample *p=&n->sound->sample;
    size_t index=(size_t)(v->phase>>12);
    size_t next=index+1<p->frames ? index+1 : p->loop_start<p->frames ? p->loop_start : index;
    int point=p->pcm[index]+(p->pcm[next]-p->pcm[index])*(int)(v->phase & 4095)/4096;
    int env=bfm_spu_envelope_clock(&v->envelope);
    v->phase+=n->pitch;
    if (v->phase >= (uint64_t)p->frames*4096) {
        if (p->frames==p->loop_start) v->note=NULL;
        else v->phase=(uint64_t)p->loop_start*4096+
            (v->phase-(uint64_t)p->frames*4096)%((uint64_t)(p->frames-p->loop_start)*4096);
    }
    if (!env && v->envelope.stage==BFM_SPU_ENVELOPE_RELEASE) v->note=NULL;
    return (int)((int64_t)point*env*level/(32767*16384));
}

static int tick(BfmTowerBellSound *s)
{
    /* Advance the room sound fields independently of the 44.1 kHz voices. */
    s->field_clock+=60000;
    if (s->field_clock>=SOURCE_RATE*1001u) {
        s->field_clock-=SOURCE_RATE*1001u;
        field(s);
    }
    int mix=0;
    for (unsigned i=0;i<2;++i) {
        Voice *v=&s->direct_voice[i];
        if (v->note) mix+=voice_sample(v,v->note->level);
    }
    for (unsigned i=0;i<SEQUENCE_VOICES;++i) {
        Voice *v=&s->sequence_voice[i];
        if (v->note) {
            unsigned gain=v->note->level*s->gain >> 14;
            gain=(gain*16383 >> 14)*16383 >> 14;
            mix+=voice_sample(v,gain*s->center_pan >> 14);
        }
    }
    return mix;
}

void bfm_tower_bell_sound_mix(BfmTowerBellSound *s, int16_t *stereo, size_t frames)
{
    if (!s || !stereo) return;
    for (size_t i=0;i<frames;) {
        int current=tick(s);
        if (s->resample<=0) {
            int add=current+(int)((int64_t)(s->previous-current)*(-s->resample)/OUTPUT_RATE);
            for (unsigned k=0;k<2;++k) {
                int value=stereo[2*i+k]+add;
                stereo[2*i+k]=(int16_t)(value<INT16_MIN?INT16_MIN:value>INT16_MAX?INT16_MAX:value);
            }
            ++i;
            s->resample+=SOURCE_RATE;
        }
        s->previous=current;
        s->resample-=OUTPUT_RATE;
    }
}
