/* SPDX-License-Identifier: MIT */
/* The room mixer owns these keyed layers, their mounted waveforms and SPU
 * release envelopes. The wheel sequencer only steers one note per group. */
static int prepare_mine_wheel(const Sources *s, Clip *clip)
{
    uint8_t entry[4], track[12], records[40];
    if (!select_command(s, BFM_MINE_WHEEL_LOOP, entry, NULL) ||
        (entry[0] & 0x7f) != 5 || !entry[1] || entry[2]) return 0;
    uint32_t tracks = word(s->data+s->bytes-4);
    if (!read_data(s, tracks+12*entry[3], track, sizeof track) || track[8] != 1 ||
        !read_data(s, word(track), records, sizeof records)) return 0;
    const uint8_t *note = records, *rest = records+20;
    unsigned key = half(note+6);
    /* A held note, followed by a silent repeating rest, drives the wheel. */
    if (!half(note) || half(note+2) || !key || key >= 127*256 || note[8] >= 128 ||
        note[9] || note[13] || word(note+16) ||
        !half(rest) || half(rest+6) || rest[13] != 2) return 0;
    Layer voice = {0};
    if (!instrument_open(s, note[10], note[11], &voice)) return 0;
    voice.key = (int)key;
    voice.pitch = pitch(s->pitch, &voice);
    voice.sequence_gain = half(s->volume+2*note[8])*127u >> 7;
    if (voice.pitch && voice.sample.loop_start < voice.sample.frames)
        clip->tone = calloc(1, sizeof *clip->tone);
    if (!clip->tone) {
        bfm_spu_sample_free(&voice.sample);
        return 0;
    }
    clip->tone->count = 1;
    clip->tone->initial[0] = voice;
    memcpy(clip->tone->pitch_table, s->pitch, sizeof s->pitch);
    clip->id = BFM_MINE_WHEEL_LOOP;
    return 1;
}

static void set_wheel_level(Playback *p, unsigned level)
{
    p->wheel_level = level;
    Layer *note = &p->layers[0];
    unsigned gain = note->sequence_gain * level >> 14;
    note->gain = ((gain * SEQUENCE_FULL_GAIN >> 14) * SEQUENCE_FULL_GAIN) >> 14;
}

static void step_wheel_level(Playback *p)
{
    if (p->wheel_slot && p->wheel_level != p->wheel_target)
        set_wheel_level(p, (unsigned)approach((int)p->wheel_level, (int)p->wheel_target, 256));
}

static void play_mine_wheel(BfmBedroomEffects *e, unsigned event)
{
    unsigned tag = event >> 16 & 3u;
    int stop = (event & BFM_MINE_WHEEL_KEY_OFF) != 0;
    if (tag >= BFM_MINE_WHEELS || (event & 0xffffu) != BFM_MINE_WHEEL_LOOP ||
        e->clips[2].id != BFM_MINE_WHEEL_LOOP) return;
    Playback *free_voice = NULL, *match = NULL;
    unsigned used = 0;
    for (unsigned i = 0; i < VOICES; ++i) {
        Playback *p = &e->playing[i];
        if (!p->clip && !free_voice) free_voice = p;
        if (!p->wheel_slot) continue;
        used |= 1u << p->wheel_slot;
        if (tag && p->command != tag) continue;
        if (stop) {
            /* The group becomes reusable while its note keys off. */
            p->wheel_slot = 0;
            bfm_spu_envelope_release(&p->layers[0].envelope);
        } else if (!match || p->wheel_slot < match->wheel_slot) {
            /* Untagged refresh addresses the first occupied sequence slot. */
            match = p;
        }
    }
    if (stop) return;
    unsigned target = ((event >> 19) & 127u) * SEQUENCE_FULL_GAIN >> 7;
    if (match) {
        match->wheel_target = target;
        return;
    }
    if (!free_voice) return;
    unsigned slot = 1;
    while (used & (1u << slot)) ++slot;
    start_voice(free_voice, &e->clips[2], tag);
    free_voice->wheel_slot = slot;
    free_voice->wheel_target = target;
    set_wheel_level(free_voice, target);
}
