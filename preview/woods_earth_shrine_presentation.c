/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_presentation.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { AWARD = 60, RESTORE = 61, SECTOR = 2048, TILE = 32, MEMBER_LIMIT = 128 * SECTOR };
enum { IDLE, PLAYER_POLL, SET_MODE, MODE_READY, SETTLING, SEEK, SEEK_READY,
       READ_START, READING, DRAIN, BANK_READY, PAUSE_READY, ACKNOWLEDGE };

typedef struct { size_t offset, bytes; } Section;
struct BfmEarthPresentationMedia {
    size_t offset, length, received;
    uint8_t *incoming, *award, *restore;
    Section poses, header, samples;
};

void bfm_earth_presentation_clear(BfmEarthShrinePresentation *p)
{
    if (!p) return;
    if (p->media) {
        free(p->media->incoming);
        free(p->media->award);
        free(p->media->restore);
        free(p->media);
    }
    *p = (BfmEarthShrinePresentation){0};
}

static int acquire(BfmEarthShrinePresentation *p, unsigned member, int direct)
{
    if (!p->media) {
        p->media = calloc(1, sizeof *p->media);
        if (!p->media) return 0;
    }
    p->member = member;
    p->direct_return = direct;
    p->phase = direct ? SET_MODE : PLAYER_POLL;
    return 1;
}

int bfm_earth_presentation_request(BfmEarthShrinePresentation *p, unsigned member)
{
    if (!p || (member != AWARD && member != RESTORE)) return 0;
    if (p->phase != IDLE) return !p->direct_return && p->member == member;
    if (p->last_player_member == member) return 1;
    if ((member == AWARD && p->last_player_member != 0) ||
        (member == RESTORE && p->last_player_member != AWARD)) return 0;
    return acquire(p, member, 0);
}

int bfm_earth_presentation_player_ready(const BfmEarthShrinePresentation *p)
{
    return p && (p->phase == IDLE || p->direct_return);
}

int bfm_earth_presentation_wait_return(BfmEarthShrinePresentation *p)
{
    if (!p || p->last_player_member != RESTORE) return 0;
    if (p->returned) return 1;
    if (p->phase == IDLE) acquire(p, RESTORE, 1);
    return 0;
}

void bfm_earth_presentation_mode(BfmEarthShrinePresentation *p, int entering)
{
    if (p) p->dark = entering != 0;
}

int bfm_earth_presentation_dark(const BfmEarthShrinePresentation *p)
{
    return p && p->dark;
}

static int locate(BfmEarthPresentationMedia *m, const MusashiDiscMedia *disc, unsigned member)
{
    uint8_t directory[SECTOR];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SC03.CD", 0, directory, sizeof directory, &size))
        return 0;
    uint32_t count = u32le(directory);
    if (count > (SECTOR - 8) / 8 || member >= count) return 0;
    const uint8_t *row = directory + 8 + 8 * member;
    unsigned first = u32le(row);
    size_t bytes = u32le(row + 4);
    if (!first || first > size / SECTOR || !bytes || bytes % SECTOR || bytes > MEMBER_LIMIT ||
        bytes > size - (size_t)first * SECTOR) return 0;
    m->incoming = malloc(bytes);
    if (!m->incoming) return 0;
    m->offset = (size_t)first * SECTOR;
    m->length = bytes;
    m->received = 0;
    return 1;
}

static int valid_bank(const uint8_t *header, size_t header_bytes, size_t sample_bytes)
{
    if (!header || header_bytes < 32) return 0;
    unsigned programs = u16le(header + 18), count = u16le(header + 22);
    if (!programs || programs > 128 || !count || count > 255 ||
        u32le(header + 12) != header_bytes + sample_bytes) return 0;
    size_t table = 32u + 128u * 16 + programs * 16u * 32;
    if (table + 512 != header_bytes || u16le(header + table)) return 0;
    size_t total = 0;
    for (unsigned i = 1; i < 256; ++i) {
        size_t size = u16le(header + table + 2 * i) * 8u;
        if ((i > count && size) || (size & 15) || size > sample_bytes - total) return 0;
        total += size;
    }
    return total == sample_bytes;
}

/* These two US members share one sparse page. Validate the complete member
 * before replacing even one destination pixel or publishing a sound bank. */
static int install(BfmEarthPresentationMedia *m, unsigned member, uint16_t vram[512][1024])
{
    Section tiles = {0}, poses = {0}, header = {0}, samples = {0};
    const uint8_t *data = m->incoming;
    if (!data || !m->length || m->length % SECTOR || m->received != m->length) return 0;
    for (size_t offset = 0; offset < m->length;) {
        size_t remaining = m->length - offset;
        if (remaining < SECTOR) return 0;
        const uint8_t *packet = data + offset;
        if (memcmp(packet, "PAC", 4)) return 0;
        unsigned type = u32le(packet + 4), key = u32le(packet + 8);
        size_t extent = u32le(packet + 12);
        if (extent < SECTOR || extent > remaining) return 0;
        size_t sectors = (extent - 1) / SECTOR + 1;
        if (sectors > remaining / SECTOR) return 0;
        Section payload = {offset + SECTOR, extent - SECTOR};
        if (type == 0 && offset == 0) {
            unsigned seen = 0;
            for (unsigned y = 0; y < 16; ++y) {
                unsigned row = u32le(packet + 16 + 4 * y);
                if ((y < 8 && row) || (row & ~(3u << 20))) return 0;
                seen += (row >> 20 & 1u) + (row >> 21 & 1u);
            }
            if (!seen || seen != key || seen * SECTOR != payload.bytes) return 0;
            tiles = payload;
        } else if (type == 0x101 && member == AWARD && !key && !poses.bytes) {
            if (payload.bytes <= 4 || (payload.bytes - 4) % 256 ||
                u32le(data + payload.offset) != 106) return 0;
            poses = payload;
        } else if (type == 2 && member == RESTORE && !key && !header.bytes) {
            if (payload.bytes < 32 || memcmp(data + payload.offset, "pBAV", 4) ||
                u32le(data + payload.offset + 4) != 7) return 0;
            header = payload;
        } else if (type == 0x103 && member == RESTORE && !key && header.bytes && !samples.bytes) {
            samples = payload;
        } else return 0;
        offset += sectors * SECTOR;
    }
    if (!tiles.bytes || (member == AWARD ? !poses.bytes :
        (!header.bytes || !samples.bytes ||
         !valid_bank(data + header.offset, header.bytes, samples.bytes)))) return 0;
    size_t at = tiles.offset;
    for (unsigned ty = 8; ty < 16; ++ty) {
        unsigned row = u32le(data + 16 + 4 * ty);
        for (unsigned tx = 20; tx < 22; ++tx) {
            if (!(row & (1u << tx))) continue;
            for (unsigned y = 0; y < TILE; ++y) {
                for (unsigned x = 0; x < TILE; ++x) {
                    vram[TILE * ty + y][TILE * tx + x] = u16le(data + at);
                    at += sizeof(uint16_t);
                }
            }
        }
    }
    if (member == AWARD) {
        free(m->award);
        m->award = m->incoming;
        m->poses = poses;
    } else {
        free(m->restore);
        m->restore = m->incoming;
        m->header = header;
        m->samples = samples;
    }
    m->incoming = NULL;
    return 1;
}

int bfm_earth_presentation_step(BfmEarthShrinePresentation *p, const MusashiDiscMedia *disc,
    uint16_t vram[512][1024], unsigned *resident_member)
{
    if (!p || !disc || !vram || !resident_member) return 0;
    BfmEarthPresentationMedia *m = p->media;
    switch (p->phase) {
    case IDLE: return 1;
    case PLAYER_POLL: p->phase = SET_MODE; break;
    case SET_MODE: p->phase = MODE_READY; break;
    case MODE_READY: p->settle = 3; p->phase = SETTLING; break;
    case SETTLING: if (!--p->settle) p->phase = SEEK; break;
    case SEEK:
        if (!locate(m, disc, p->member)) goto failed;
        p->phase = SEEK_READY;
        break;
    case SEEK_READY: p->phase = READ_START; break;
    case READ_START: p->phase = READING; break;
    case READING: {
        /* US ReadN uses double speed: 150 data sectors per second, or five
         * sequential sectors per 30 Hz field. Read only the remaining extent. */
        size_t bytes = m->length - m->received, total;
        if (bytes > 5 * SECTOR) bytes = 5 * SECTOR;
        if (!musashi_disc_media_read_root_range(disc, "SC03.CD", m->offset + m->received,
                m->incoming + m->received, bytes, &total) ||
            m->offset > total || m->length > total - m->offset) goto failed;
        m->received += bytes;
        if (m->received == m->length) p->phase = DRAIN;
        break;
    }
    case DRAIN: p->phase = p->member == RESTORE ? BANK_READY : PAUSE_READY; break;
    case BANK_READY: p->phase = PAUSE_READY; break;
    case PAUSE_READY:
        if (!install(m, p->member, vram)) goto failed;
        *resident_member = p->member;
        p->phase = ACKNOWLEDGE;
        break;
    case ACKNOWLEDGE:
        if (p->direct_return) p->returned = 1;
        else p->last_player_member = p->member;
        p->phase = IDLE;
        break;
    default: return 0;
    }
    return 1;
failed:
    free(m->incoming);
    m->incoming = NULL;
    p->phase = SET_MODE;
    return 0;
}

const uint8_t *bfm_earth_presentation_body(const BfmEarthShrinePresentation *p, size_t *bytes)
{
    if (!bytes) return NULL;
    *bytes = 0;
    if (!p || !p->media || !p->media->award) return NULL;
    *bytes = p->media->poses.bytes;
    return p->media->award + p->media->poses.offset;
}

int bfm_earth_presentation_bank(const BfmEarthShrinePresentation *p,
    const uint8_t **header, size_t *header_bytes, const uint8_t **samples, size_t *sample_bytes)
{
    if (!p || !p->media || !p->media->restore || !header || !header_bytes ||
        !samples || !sample_bytes) return 0;
    *header = p->media->restore + p->media->header.offset;
    *header_bytes = p->media->header.bytes;
    *samples = p->media->restore + p->media->samples.offset;
    *sample_bytes = p->media->samples.bytes;
    return 1;
}
