/* SPDX-License-Identifier: MIT */
/* The fixed /109 programs each schedule seven notes. The start program
 * sustains a loop under later notes until the actor keys off the group. */
static int keep_fan_notes(const Sources *s, unsigned command, Layer notes[], unsigned count, Clip *clip)
{
    if (count > MAX_KEYED_NOTES) return 0;
    KeyedTone *tone = calloc(1, sizeof *tone);
    if (!tone) return 0;
    tone->count = count;
    for (unsigned i = 0; i < count; ++i) {
        tone->initial[i] = notes[i];
        notes[i].sample = (BfmDecodedSpuSample){0};
    }
    memcpy(tone->pitch_table, s->pitch, sizeof s->pitch);
    clip->tone = tone;
    clip->id = command;
    return 1;
}

static int prepare_mine_shaft(Sources *s, BfmBedroomEffects *effects)
{
    static const unsigned direct[] = {
        BFM_WOODS_BOX_OPEN_SOUND, BFM_WOODS_BOX_CLOSE_SOUND, BFM_ROOM_MOVE_SOUND,
        BFM_ROOM_ACCEPT_SOUND, BFM_WALL_CLIMB_FIRST_SOUND, BFM_WALL_CLIMB_SECOND_SOUND
    };
    s->common = bfm_archive_sound_bank_load(s->disc, "MAIN.CD", 2);
    if (!s->common) return 0;
    effects->replace_matching = 1;
    size_t count = sizeof direct / sizeof *direct;
    for (size_t i = 0; i < count; ++i)
        if (!prepare(s, direct[i], &effects->clips[i])) return 0;
    return prepare_sequence(s, BFM_MINE_FAN_START, &effects->clips[count]) &&
           prepare_sequence(s, BFM_MINE_FAN_DECELERATE, &effects->clips[count+1]) &&
           prepare_sequence(s, BFM_WOODS_BOX_ACCEPT_SOUND, &effects->clips[count+2]);
}

static int is_fan(unsigned command)
{
    return command == BFM_MINE_FAN_START || command == BFM_MINE_FAN_DECELERATE;
}

static void release_fan(BfmBedroomEffects *e, unsigned command)
{
    for (unsigned i = 0; i < VOICES; ++i) {
        Playback *p = &e->playing[i];
        if (!p->clip || p->command != command) continue;
        p->command = 0;
        if (p->fan_pending) {
            p->clip = NULL;
            continue;
        }
        /* The resident frees the scheduler, cancels future notes and leaves
         * already sounding instruments on their individual SPU releases. */
        for (unsigned j = 0; j < p->clip->tone->count; ++j) {
            Layer *layer = &p->layers[j];
            if (layer->delay) layer->done = 1;
            else bfm_spu_envelope_release(&layer->envelope);
        }
    }
}
