/* SPDX-License-Identifier: MIT */
#include "palace_sound_bank.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, MAX_SAMPLES = 255, SPU_RAM_BYTES = 512 * 1024 };
struct BfmPalaceBank {
    uint8_t *body;
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
    bank->count = count;
    return bank;
fail:
    bfm_palace_sound_bank_free(bank);
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
        if (!read_scene(disc, offset, block, sizeof(block)) || memcmp(block, "PAC", 4))
            return NULL;
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
    if (bank)
        free(bank->body);
    free(bank);
}
