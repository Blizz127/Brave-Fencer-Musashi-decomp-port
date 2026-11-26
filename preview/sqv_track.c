/* SPDX-License-Identifier: MIT */
#include "sqv_track.h"

#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, PALACE_MEMBER = 2, FOREST_MEMBER = 78, MAX_TRACKS = 3, MIDI_HEADER_BYTES = 22 };

static uint32_t little32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t big32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

typedef struct {
    size_t offset, bytes;
    uint16_t division;
} TrackRange;

static int read_score(const MusashiDiscMedia *disc, const char *archive, size_t offset, uint8_t *p, size_t bytes)
{
    size_t root_size;
    return musashi_disc_media_read_root_range(disc, archive, offset, p, bytes, &root_size);
}

static BfmSqvResult locate(const MusashiDiscMedia *disc, const char *archive, unsigned member, unsigned track,
                           unsigned tracks, TrackRange *range)
{
    uint8_t directory[8], entry[8], wrapper[16], midi[MIDI_HEADER_BYTES];
    size_t root_size;
    if (!musashi_disc_media_read_root_range(disc, archive, 0, directory, sizeof(directory),
                                           &root_size)) return BFM_SQV_DISC_ERROR;
    uint32_t count = little32(directory);
    if (count <= member || count > (root_size - 8) / 8) return BFM_SQV_INVALID;
    if (!read_score(disc, archive, 8 + (size_t)member * 8, entry, sizeof(entry))) return BFM_SQV_DISC_ERROR;
    uint32_t sector = little32(entry);
    size_t member_bytes = little32(entry + 4);
    if (sector > root_size / SECTOR)
        return BFM_SQV_INVALID;
    size_t base = (size_t)sector * SECTOR;
    if (base < 8 + 8 * (size_t)count || member_bytes < sizeof(wrapper) ||
        member_bytes > root_size - base) return BFM_SQV_INVALID;
    if (!read_score(disc, archive, base, wrapper, sizeof(wrapper))) return BFM_SQV_DISC_ERROR;
    if (memcmp(wrapper, ".sqv", 4)) return BFM_SQV_INVALID;
    /* Offsets follow the signature. The palace has two offsets, the
     * forest none; all unused words in either wrapper must be zero. */
    for (unsigned i = tracks; i <= MAX_TRACKS; ++i)
        if (little32(wrapper + 4 * i)) return BFM_SQV_UNSUPPORTED;
    size_t starts[MAX_TRACKS + 1] = {sizeof(wrapper)};
    starts[tracks] = member_bytes;
    for (unsigned i = 1; i < tracks; ++i) {
        size_t relative = little32(wrapper + 4 * i);
        if (relative > member_bytes - 4) return BFM_SQV_INVALID;
        starts[i] = relative + 4;
    }
    for (unsigned i = 0; i < tracks; ++i)
        if (starts[i] > starts[i + 1] || starts[i + 1] - starts[i] < sizeof(midi))
            return BFM_SQV_INVALID;

    size_t start = starts[track];
    if (!read_score(disc, archive, base + start, midi, sizeof(midi))) return BFM_SQV_DISC_ERROR;
    if (memcmp(midi, "MThd", 4) || memcmp(midi + 14, "MTrk", 4)) return BFM_SQV_INVALID;
    if (big32(midi + 4) != 6 || midi[8] || midi[9] || midi[10] || midi[11] != 1 ||
        (midi[12] & 128)) return BFM_SQV_UNSUPPORTED;
    range->division = (uint16_t)((unsigned)midi[12] << 8 | midi[13]);
    size_t body = start + sizeof(midi);
    size_t declared = big32(midi + 18);
    if (!range->division || !declared || declared > member_bytes - body) return BFM_SQV_INVALID;
    /* The logical MTrk length can exceed the space before the next header.
     * Both it and the physical envelope bound reads. End-of-track fixes
     * the exact number of compact bytes played inside those bounds. */
    range->offset = base + body;
    range->bytes = starts[track + 1] - body;
    if (range->bytes > declared) range->bytes = declared;
    if (track == tracks - 1) {
        /* A sector-aligned VAB follows the final score in this member. A
         * malformed length must never feed its instrument data to the parser. */
        size_t distance = (SECTOR - body % SECTOR) % SECTOR;
        while (distance < range->bytes && range->bytes - distance >= 4) {
            uint8_t signature[4];
            if (!read_score(disc, archive, range->offset + distance, signature, sizeof(signature)))
                return BFM_SQV_DISC_ERROR;
            if (!memcmp(signature, "pBAV", 4)) {
                range->bytes = distance;
                break;
            }
            if (range->bytes - distance <= SECTOR) break;
            distance += SECTOR;
        }
    }
    return BFM_SQV_OK;
}

typedef struct {
    const uint8_t *next, *end;
} Cursor;

static int varlen(Cursor *in, uint32_t *out)
{
    uint32_t value = 0;
    for (unsigned i = 0; i < 4 && in->next < in->end; ++i) {
        unsigned byte = *in->next++;
        value = (value << 7) | (byte & 127);
        if (!(byte & 128)) {
            *out = value;
            return 1;
        }
    }
    return 0;
}

static BfmSqvResult meta_event(Cursor *in, BfmSqvEvent *event)
{
    if (in->next == in->end) return BFM_SQV_INVALID;
    unsigned type = *in->next++;
    uint32_t n;
    if (!varlen(in, &n) || n > (size_t)(in->end - in->next)) return BFM_SQV_INVALID;
    if (type == 0x2f) {
        if (n) return BFM_SQV_INVALID;
        event->kind = BFM_SQV_END;
    } else if (type == 0x51) {
        if (n != 3) return BFM_SQV_INVALID;
        event->kind = BFM_SQV_TEMPO;
        event->tempo_us_per_quarter = (uint32_t)in->next[0] << 16 |
                                      (uint32_t)in->next[1] << 8 | in->next[2];
        if (!event->tempo_us_per_quarter) return BFM_SQV_INVALID;
    } else {
        return BFM_SQV_UNSUPPORTED;
    }
    in->next += n;
    return BFM_SQV_OK;
}

static BfmSqvResult channel_event(Cursor *in, unsigned status, unsigned member, BfmSqvEvent *event)
{
    unsigned command = status >> 4;
    if (command != 8 && command != 9 && command != 11 && command != 12 &&
        !((member == FOREST_MEMBER || member == 75) && command == 14))
        return BFM_SQV_UNSUPPORTED;
    if (in->next == in->end || *in->next >= 128) return BFM_SQV_INVALID;
    event->channel = (uint8_t)(status & 15);
    event->number = *in->next++;
    if (command == 8) {
        /* Compact note-off has a key and no velocity byte. */
        event->kind = BFM_SQV_NOTE_OFF;
    } else if (command == 12) {
        event->kind = BFM_SQV_PROGRAM;
    } else if (command == 14) {
        /* The mounted field scores encode a single wheel byte. */
        event->kind = BFM_SQV_PITCH_BEND;
    } else {
        if (in->next == in->end || *in->next >= 128) return BFM_SQV_INVALID;
        event->value = *in->next++;
        if (command == 9) event->kind = event->value ? BFM_SQV_NOTE_ON : BFM_SQV_NOTE_OFF;
        else event->kind = event->number == 10 ? BFM_SQV_PAN : BFM_SQV_CONTROL;
    }
    return BFM_SQV_OK;
}

static BfmSqvResult parse(const uint8_t *data, size_t bytes, unsigned member, BfmSqvEvent *events,
                          size_t *count, uint32_t *duration)
{
    Cursor in = {data, data + bytes};
    uint32_t tick = 0;
    unsigned running = 0;
    size_t index = 0;
    while (in.next < in.end) {
        uint32_t delta;
        if (!varlen(&in, &delta) || delta > UINT32_MAX - tick || in.next == in.end)
            return BFM_SQV_INVALID;
        tick += delta;
        unsigned status = running;
        if (*in.next >= 128) status = *in.next++;
        if (!status) return BFM_SQV_INVALID;
        BfmSqvEvent event = {.tick = tick, .channel = 0xff};
        BfmSqvResult result;
        if (status == 0xff) {
            running = 0;
            result = meta_event(&in, &event);
        } else {
            running = status;
            result = channel_event(&in, status, member, &event);
        }
        if (result != BFM_SQV_OK) return result;
        if (events) events[index] = event;
        ++index;
        if (event.kind == BFM_SQV_END) {
            *count = index;
            *duration = tick;
            return BFM_SQV_OK;
        }
    }
    return BFM_SQV_INVALID;
}

BfmSqvResult bfm_sqv_archive_track_load(const MusashiDiscMedia *disc, const char *archive, unsigned member,
                                      unsigned track, BfmSqvTrack *out)
{
    if (!out) return BFM_SQV_INVALID;
    *out = (BfmSqvTrack){0};
    if (!disc) return BFM_SQV_INVALID;
    if (!archive) return BFM_SQV_INVALID;
    int sc01 = !strcmp(archive, "SC01.CD"), sc03 = !strcmp(archive, "SC03.CD");
    int single = (sc01 && member == FOREST_MEMBER) || (sc03 && member == 67);
    int triple = (sc01 && (member == PALACE_MEMBER || member == 75)) || (sc03 && member == 68);
    if (!single && !triple) return BFM_SQV_UNSUPPORTED;
    unsigned tracks = single ? 1 : MAX_TRACKS;
    if (track >= tracks) return BFM_SQV_INVALID;
    TrackRange range;
    BfmSqvResult result = locate(disc, archive, member, track, tracks, &range);
    if (result != BFM_SQV_OK) return result;
    if (!range.bytes) return BFM_SQV_INVALID;
    uint8_t *data = malloc(range.bytes);
    if (!data) return BFM_SQV_NO_MEMORY;
    BfmSqvTrack score = {.ticks_per_quarter = range.division};
    if (!read_score(disc, archive, range.offset, data, range.bytes)) {
        result = BFM_SQV_DISC_ERROR;
    } else {
        /* Count and validate the immutable stream before allocating its
         * event list. Both passes stop at the same first end-of-track. */
        result = parse(data, range.bytes, member, NULL, &score.event_count, &score.duration_ticks);
        if (result == BFM_SQV_OK) {
            if (score.event_count <= SIZE_MAX / sizeof(*score.events))
                score.events = malloc(score.event_count * sizeof(*score.events));
            if (!score.events) result = BFM_SQV_NO_MEMORY;
            else result = parse(data, range.bytes, member, score.events, &score.event_count, &score.duration_ticks);
        }
    }
    free(data);
    if (result == BFM_SQV_OK) *out = score;
    else free(score.events);
    return result;
}

BfmSqvResult bfm_sqv_member_track_load(const MusashiDiscMedia *disc, unsigned member,
                                      unsigned track, BfmSqvTrack *out)
{
    return bfm_sqv_archive_track_load(disc, "SC01.CD", member, track, out);
}

BfmSqvResult bfm_sqv_track_load(const MusashiDiscMedia *disc, unsigned track, BfmSqvTrack *out)
{
    return bfm_sqv_member_track_load(disc, PALACE_MEMBER, track, out);
}

void bfm_sqv_track_free(BfmSqvTrack *track)
{
    if (track) {
        free(track->events);
        *track = (BfmSqvTrack){0};
    }
}
