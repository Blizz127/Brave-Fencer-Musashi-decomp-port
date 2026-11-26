/* SPDX-License-Identifier: MIT */
#include "palace_sound_bank.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, MAX_SAMPLES = 255, SPU_RAM_BYTES = 512 * 1024 };
struct BfmPalaceBank {
    uint8_t *body, *header;
    unsigned programs;
    unsigned count;
    size_t boundaries[MAX_SAMPLES + 1];
};

static unsigned le16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p + 2) << 16;
}

static int read_scene(const MusashiDiscMedia *disc, size_t offset, uint8_t *out, size_t size)
{
    size_t file_size;
    return musashi_disc_media_read_root_range(disc, "SC01.CD", offset, out, size, &file_size);
}

/* The program table has 128 entries; each present program owns 16 tone slots.
 * The final 256 unsigned lengths count eight-byte units, starting with zero. */
static BfmPalaceBank *read_instruments(const MusashiDiscMedia *disc,
                                      size_t header, size_t header_bytes,
                                      size_t body, size_t body_bytes)
{
    uint8_t info[32], lengths[512];
    if (header_bytes < sizeof(info) || !body_bytes || body_bytes > SPU_RAM_BYTES ||
        !read_scene(disc, header, info, sizeof(info)) ||
        memcmp(info, "pBAV", 4) || le32(info + 4) != 7)
        return NULL;
    unsigned programs = le16(info + 18), count = le16(info + 22);
    if (!programs || programs > 128 || !count || count > MAX_SAMPLES)
        return NULL;
    size_t table = sizeof(info) + 128 * 16 + programs * 16 * 32;
    if (table + sizeof(lengths) != header_bytes || le32(info + 12) != header_bytes + body_bytes ||
        !read_scene(disc, header + table, lengths, sizeof(lengths)) || le16(lengths))
        return NULL;
    BfmPalaceBank *bank = calloc(1, sizeof(*bank));
    if (!bank)
        return NULL;
    size_t total = 0;
    for (unsigned i = 1; i <= MAX_SAMPLES; ++i) {
        size_t bytes = 8u * le16(lengths + 2 * i);
        if (bytes > body_bytes - total || (bytes & 15) || (i > count && bytes))
            goto fail;
        total += bytes;
        bank->boundaries[i] = total;
    }
    if (total != body_bytes)
        goto fail;
    bank->body = malloc(body_bytes);
    if (!bank->body || !read_scene(disc, body, bank->body, body_bytes))
        goto fail;
    bank->header = malloc(header_bytes);
    if (!bank->header || !read_scene(disc, header, bank->header, header_bytes))
        goto fail;
    bank->count = count;
    bank->programs = programs;
    return bank;
fail:
    bfm_palace_sound_bank_free(bank);
    return NULL;
}

/* The embedded bank header and body are sector-aligned. The two words
 * just before the body give its sector count and bytes in the final sector. */
static BfmPalaceBank *read_score_bank(const MusashiDiscMedia *disc, size_t base, size_t bytes,
                                      unsigned member, const uint8_t wrapper[16])
{
    if (bytes % SECTOR || le32(wrapper + 12)) return NULL;
    size_t middle = le32(wrapper + 4), last = le32(wrapper + 8);
    size_t final_track = 16;
    if (member == 2) {
        if (middle < 12 || last <= middle || last > bytes - 8) return NULL;
        final_track = 4 + last;
    } else if (member != 78 || middle || last) {
        return NULL;
    }
    uint8_t signature[4];
    if (!read_scene(disc, base + final_track, signature, sizeof(signature)) ||
        memcmp(signature, "MThd", 4)) return NULL;
    size_t first_sector = final_track / SECTOR + 1;
    for (size_t sector = first_sector; sector < bytes / SECTOR; ++sector) {
        size_t relative = sector * SECTOR;
        uint8_t info[32], trailer[4];
        if (!read_scene(disc, base + relative, info, sizeof(info))) return NULL;
        if (memcmp(info, "pBAV", 4) || le32(info + 4) != 7) continue;
        unsigned programs = le16(info + 18);
        if (!programs || programs > 128) return NULL;
        size_t header_bytes = 32 + 128 * 16 + programs * 16 * 32 + 512;
        size_t total = le32(info + 12);
        if (total <= header_bytes || total - header_bytes > SPU_RAM_BYTES) return NULL;
        size_t body_bytes = total - header_bytes;
        size_t sectors = (body_bytes - 1) / SECTOR + 1;
        if (sectors >= bytes / SECTOR) return NULL;
        size_t body = bytes - sectors * SECTOR;
        if (relative >= body || header_bytes + 4 > body - relative ||
            !read_scene(disc, base + body - 4, trailer, sizeof(trailer)) ||
            le16(trailer) != sectors || le16(trailer + 2) != (body_bytes - 1) % SECTOR + 1)
            return NULL;
        return read_instruments(disc, base + relative, header_bytes, base + body, body_bytes);
    }
    return NULL;
}

BfmPalaceBank *bfm_palace_sound_bank_load(const MusashiDiscMedia *disc, unsigned member)
{
    uint8_t sector_header[SECTOR], block[16];
    size_t file_size;
    if (!disc || !musashi_disc_media_read_root_range(disc, "SC01.CD", 0, sector_header,
                                                       sizeof(sector_header), &file_size))
        return NULL;
    unsigned count = le32(sector_header);
    if (count > (SECTOR - 8) / 8 || member >= count)
        return NULL;
    size_t sector = le32(sector_header + 8 + member * 8);
    size_t left = le32(sector_header + 12 + member * 8);
    if (!sector || sector > file_size / SECTOR)
        return NULL;
    size_t offset = sector * SECTOR;
    if (left > file_size - offset)
        return NULL;
    size_t header = 0, header_bytes = 0;
    unsigned header_key = 0;
    while (left >= SECTOR) {
        if (!read_scene(disc, offset, block, sizeof(block))) return NULL;
        if (offset == sector * SECTOR && !memcmp(block, ".sqv", 4))
            return read_score_bank(disc, offset, left, member, block);
        if (memcmp(block, "PAC", 4)) return NULL;
        unsigned type = le32(block + 4), key = le32(block + 8);
        size_t bytes = le32(block + 12);
        if (bytes < SECTOR || bytes > left)
            return NULL;
        if (header_bytes) {
            if (type != 3 || key != header_key)
                return NULL;
            return read_instruments(disc, header, header_bytes, offset + SECTOR, bytes - SECTOR);
        }
        if (type == 2) {
            header = offset + SECTOR;
            header_bytes = bytes - SECTOR;
            header_key = key;
            if (!header_bytes)
                return NULL;
        }
        size_t sectors = 1 + (bytes - 1) / SECTOR;
        if (sectors > left / SECTOR)
            return NULL;
        offset += sectors * SECTOR;
        left -= sectors * SECTOR;
    }
    return NULL;
}

int bfm_palace_sound_sample(const BfmPalaceBank *bank, unsigned index, BfmSpuSample *sample)
{
    if (!bank || !sample || !index || index > bank->count)
        return 0;
    size_t start = bank->boundaries[index - 1], end = bank->boundaries[index];
    if (start == end)
        return 0;
    *sample = (BfmSpuSample){bank->body + start, end - start};
    return 1;
}

void bfm_palace_sound_bank_free(BfmPalaceBank *bank)
{
    if (bank) {
        free(bank->header);
        free(bank->body);
    }
    free(bank);
}

unsigned bfm_palace_sound_tones(const BfmPalaceBank *bank, unsigned program, unsigned key,
                                BfmPalaceTone tones[BFM_PALACE_MAX_LAYERS])
{
    if (!bank || !tones || program >= 128 || key >= 128) return 0;
    const uint8_t *programs = bank->header + 32;
    const uint8_t *selected = programs + 16 * program;
    unsigned layers = selected[0];
    if (!layers || layers > BFM_PALACE_MAX_LAYERS) return 0;
    unsigned group = 0;
    for (unsigned p = 0; p < program; ++p) {
        if (programs[16 * p]) ++group;
    }
    if (group >= bank->programs) return 0;
    const uint8_t *source = programs + 128 * 16 + group * 16 * 32;
    BfmPalaceTone matches[BFM_PALACE_MAX_LAYERS];
    unsigned count = 0;
    for (unsigned layer = 0; layer < layers; ++layer, source += 32) {
        if (source[6] > key || source[7] < key) continue;
        unsigned sample = le16(source + 22);
        if (le16(source + 20) != program || !sample || sample > bank->count) return 0;
        matches[count++] = (BfmPalaceTone){
            .sample = sample, .key_low = source[6], .key_high = source[7], .center = source[4],
            .tuning = source[5] < 128 ? source[5] : (int)source[5] - 256,
            .bank_gain = bank->header[24], .bank_pan = bank->header[25],
            .program_gain = selected[1], .program_pan = selected[4],
            .tone_gain = source[2], .tone_pan = source[3], .priority = source[0], .mode = source[1],
            .adsr1 = le16(source + 16), .adsr2 = le16(source + 18),
            .bend_range = {source[12], source[13]}
        };
    }
    memcpy(tones, matches, count * sizeof(*tones));
    return count;
}
