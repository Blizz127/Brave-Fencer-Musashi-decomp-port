#include "musashi_disc_media.h"
#include "main12_internal.h"

#include "musashi_sha256_digest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PINNED_CUE_SIZE = 250,
    PINNED_BIN_SIZE = 416021760,
    PINNED_TOTAL_FRAMES = 176880,
    PINNED_TRACK_COUNT = 4,
    READ_CHUNK = 1024 * 1024
};

struct MusashiDiscMedia {
    unsigned char *image;
    MusashiDiscMediaInfo info;
};

static int hex_value(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

static int sha256_matches(const unsigned char *data, size_t size,
                          const char *expected_hex) {
    unsigned char digest[32];
    size_t i;

    if (!data || !expected_hex || strlen(expected_hex) != 64u ||
        !musashi_sha256_digest(data, size, digest))
        return 0;
    for (i = 0; i < 32u; ++i) {
        int high = hex_value(expected_hex[i * 2u]);
        int low = hex_value(expected_hex[i * 2u + 1u]);
        if (high < 0 || low < 0 || digest[i] != (unsigned char)((high << 4) | low))
            return 0;
    }
    return 1;
}

static int read_file_bytes(const char *path, unsigned char *destination,
                           size_t expected_size) {
    FILE *file;
    long length;
    size_t offset = 0;
    int close_ok;

    if (!path || !destination) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (unsigned long)length != (unsigned long)expected_size ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    while (offset < expected_size) {
        size_t request = expected_size - offset;
        size_t count = request > READ_CHUNK ? READ_CHUNK : request;
        size_t got = fread(destination + offset, 1, count, file);
        offset += got;
        if (got != count) break;
    }
    close_ok = fclose(file) == 0;
    return close_ok && offset == expected_size;
}

static int file_has_exact_size(const char *path, size_t expected_size) {
    FILE *file;
    long length;
    int close_ok;

    if (!path) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0)
        close_ok = 0;
    else
        close_ok = (unsigned long)length == (unsigned long)expected_size;
    if (fclose(file) != 0) close_ok = 0;
    return close_ok;
}

static int validate_cue(const char *cue_path) {
    unsigned char cue[PINNED_CUE_SIZE];
    return read_file_bytes(cue_path, cue, sizeof(cue)) &&
           sha256_matches(cue, sizeof(cue),
                          "85326d3f735d603a9d37f0e0754a37a52cc9d0bf6b8b4ce566c4fa54f6fdee5c");
}

static void fill_info(MusashiDiscMediaInfo *info) {
    memset(info, 0, sizeof(*info));
    info->sector_size = MUSASHI_DISC_RAW_SECTOR_SIZE;
    info->total_frames = PINNED_TOTAL_FRAMES;
    info->track_count = PINNED_TRACK_COUNT;

    info->tracks[0].number = 1;
    info->tracks[0].index00_frame = 0;
    info->tracks[0].index01_frame = 0;
    info->tracks[0].first_frame = 0;
    info->tracks[0].end_frame = 155122;
    info->tracks[0].mode = MUSASHI_DISC_TRACK_MODE2_2352;

    info->tracks[1].number = 2;
    info->tracks[1].index00_frame = 155122;
    info->tracks[1].index01_frame = 155272;
    info->tracks[1].first_frame = 155272;
    info->tracks[1].end_frame = 159662;
    info->tracks[1].mode = MUSASHI_DISC_TRACK_AUDIO;

    info->tracks[2].number = 3;
    info->tracks[2].index00_frame = 159662;
    info->tracks[2].index01_frame = 159812;
    info->tracks[2].first_frame = 159812;
    info->tracks[2].end_frame = 162778;
    info->tracks[2].mode = MUSASHI_DISC_TRACK_AUDIO;

    info->tracks[3].number = 4;
    info->tracks[3].index00_frame = 162778;
    info->tracks[3].index01_frame = 162928;
    info->tracks[3].first_frame = 162928;
    info->tracks[3].end_frame = PINNED_TOTAL_FRAMES;
    info->tracks[3].mode = MUSASHI_DISC_TRACK_AUDIO;
}

MusashiDiscMedia *musashi_disc_media_open_pinned(const char *cue_path,
                                                 const char *bin_path) {
    unsigned char *image;
    MusashiDiscMedia *media;

    if (!validate_cue(cue_path)) return NULL;
    if (!file_has_exact_size(bin_path, PINNED_BIN_SIZE)) return NULL;
    image = malloc(PINNED_BIN_SIZE);
    if (!image) return NULL;
    if (!read_file_bytes(bin_path, image, PINNED_BIN_SIZE) ||
        !sha256_matches(image, PINNED_BIN_SIZE,
                        "0a53702937d74e20d99fee9a29a80f7e91da762e66958819d531173939a50879")) {
        free(image);
        return NULL;
    }
    media = calloc(1, sizeof(*media));
    if (!media) {
        free(image);
        return NULL;
    }
    media->image = image;
    fill_info(&media->info);
    return media;
}

static int ends_with(const char *path, const char *ext) {
    size_t n = strlen(path), m = strlen(ext), i;
    if (n < m) return 0;
    for (i = 0; i < m; ++i) {
        char c = path[n - m + i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c != ext[i]) return 0;
    }
    return 1;
}

static long file_size(const char *path) {
    FILE *file = fopen(path, "rb");
    long length = -1;
    if (!file) return -1;
    if (fseek(file, 0, SEEK_END) == 0) length = ftell(file);
    fclose(file);
    return length;
}

static int cue_file_is_absolute(const char *name) {
#ifdef _WIN32
    if (name[0] == '\\' || (name[0] && name[1] == ':')) return 1;
#endif
    return name[0] == '/';
}

/* Up to eight FILE entries of a cue sheet, resolved against its folder. */
static int cue_files(const char *cue_path, char files[][4096], int max) {
    FILE *cue = fopen(cue_path, "r");
    char line[4096];
    const char *slash = strrchr(cue_path, '/');
    int count = 0;
#ifdef _WIN32
    /* Windows paths may use either separator (and "C:disc.cue" names a folder by drive). */
    {
        const char *back = strrchr(cue_path, '\\');
        if (back && (!slash || back > slash)) slash = back;
        if (!slash && cue_path[0] && cue_path[1] == ':') slash = cue_path + 1;
    }
#endif
    if (!cue) return -1;
    while (fgets(line, sizeof line, cue)) {
        char *p = line, *name, *end;
        size_t prefix, name_length;
        while (*p == ' ' || *p == '\t') ++p;
        if (strncmp(p, "FILE", 4) != 0 || (p[4] != ' ' && p[4] != '\t')) continue;
        p += 5;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '"') {
            name = ++p;
            end = strchr(p, '"');
        } else {
            name = p;
            end = strpbrk(p, " \t\r\n");
        }
        if (!end || end == name || count >= max) { fclose(cue); return -1; }
        *end = '\0';
        prefix = slash && !cue_file_is_absolute(name) ? (size_t)(slash + 1 - cue_path) : 0;
        name_length = (size_t)(end - name);
        if (prefix >= sizeof(files[0]) || name_length >= sizeof(files[0]) - prefix) {
            fclose(cue);
            return -1;
        }
        memcpy(files[count], cue_path, prefix);
        memcpy(files[count] + prefix, name, name_length + 1);
        ++count;
    }
    fclose(cue);
    return count;
}

MusashiDiscMedia *musashi_disc_media_open_image(const char *path, char *why,
                                                size_t why_size) {
    char files[8][4096];
    size_t track_sizes[8];
    int count, i;
    size_t offset = 0;
    unsigned char *image;
    MusashiDiscMedia *media;
#define REFUSE(...) do { if (why && why_size) snprintf(why, why_size, __VA_ARGS__); return NULL; } while (0)
    if (!path || !path[0]) REFUSE("No disc image was given.");
    if (ends_with(path, ".iso"))
        REFUSE("%s is an .iso (2048-byte sectors). PlayStation games need the raw "
               "2352-byte dump: use the .cue/.bin of your disc.", path);
    if (ends_with(path, ".cue")) {
        count = cue_files(path, files, 8);
        if (count <= 0) REFUSE("%s could not be read as a cue sheet.", path);
    } else {
        if (strlen(path) >= sizeof files[0]) REFUSE("The disc image path is too long.");
        memcpy(files[0], path, strlen(path) + 1u);
        count = 1;
    }
    for (i = 0; i < count; ++i) {
        long length = file_size(files[i]);
        if (length < 0) REFUSE("The disc image file %s is missing.", files[i]);
        if ((unsigned long)length > PINNED_BIN_SIZE - offset)
            REFUSE("%s is not a complete image of Brave Fencer Musashi (USA, SLUS-00726): "
                   "the size does not match.", path);
        track_sizes[i] = (size_t)length;
        offset += track_sizes[i];
    }
    if (offset != PINNED_BIN_SIZE)
        REFUSE("%s is not a complete image of Brave Fencer Musashi (USA, SLUS-00726): "
               "the size does not match.", path);
    image = malloc(PINNED_BIN_SIZE);
    if (!image) REFUSE("Not enough memory to load the disc image.");
    for (offset = 0, i = 0; i < count; ++i) {
        if (!read_file_bytes(files[i], image + offset, track_sizes[i])) {
            free(image);
            REFUSE("The disc image file %s could not be read.", files[i]);
        }
        offset += track_sizes[i];
    }
    if (!sha256_matches(image, PINNED_BIN_SIZE,
                        "0a53702937d74e20d99fee9a29a80f7e91da762e66958819d531173939a50879")) {
        free(image);
        REFUSE("%s is not an unmodified image of Brave Fencer Musashi (USA, SLUS-00726).", path);
    }
    media = calloc(1, sizeof(*media));
    if (!media) {
        free(image);
        REFUSE("Not enough memory to load the disc image.");
    }
#undef REFUSE
    media->image = image;
    fill_info(&media->info);
    return media;
}

/* ISO 9660 in Mode 2 Form 1 sectors: user data starts 24 bytes in. */
static const unsigned char *iso_sector(const MusashiDiscMedia *media, uint32_t lba) {
    if (lba >= media->info.tracks[0].end_frame) return NULL;
    return media->image + (size_t)lba * MUSASHI_DISC_RAW_SECTOR_SIZE + 24u;
}

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

enum { ISO_BLOCK = 2048 };
typedef struct IsoExtent {
    uint32_t lba, size;
} IsoExtent;

static int iso_extent_fits(const MusashiDiscMedia *media, const IsoExtent *extent) {
    uint32_t end = media->info.tracks[0].end_frame;
    uint32_t blocks = extent->size / ISO_BLOCK + (extent->size % ISO_BLOCK != 0);
    return end <= PINNED_TOTAL_FRAMES && extent->lba <= end && blocks <= end - extent->lba;
}

static int root_file_extent(const MusashiDiscMedia *media, const char *name, IsoExtent *out) {
    if (!media || !media->image || !name || !name[0] || !out) return 0;
    const size_t name_len = strlen(name);
    const uint8_t *pvd = iso_sector(media, 16);
    if (!pvd || pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5)) return 0;
    const uint8_t *root = pvd + 156;
    if (root[0] < 34 || root[1] || !(root[25] & 2) || (root[25] & 0x80) ||
        root[26] || root[27]) return 0;
    IsoExtent dir = {le32(root + 2), le32(root + 10)};
    if (!iso_extent_fits(media, &dir)) return 0;
    for (uint32_t pos = 0; pos < dir.size;) {
        uint32_t within = pos % ISO_BLOCK;
        uint32_t available = ISO_BLOCK - within;
        if (available > dir.size - pos) available = dir.size - pos;
        const uint8_t *rec = iso_sector(media, dir.lba + pos / ISO_BLOCK) + within;
        if (rec[0] == 0) { pos += available; continue; }
        if (rec[0] < 34 || rec[0] > available || rec[32] > rec[0] - 33u) return 0;
        if (rec[32] >= name_len && !memcmp(rec + 33, name, name_len) &&
            (rec[32] == name_len || rec[33 + name_len] == ';')) {
            if (rec[1] || (rec[25] & 0x82) || rec[26] || rec[27]) return 0;
            IsoExtent found = {le32(rec + 2), le32(rec + 10)};
            if (!iso_extent_fits(media, &found)) return 0;
            *out = found;
            return 1;
        }
        pos += rec[0];
    }
    return 0;
}

static void copy_root_range(const MusashiDiscMedia *media, IsoExtent file,
                            size_t offset, uint8_t *out, size_t count) {
    while (count) {
        size_t within = offset % ISO_BLOCK;
        size_t chunk = ISO_BLOCK - within;
        if (chunk > count) chunk = count;
        const uint8_t *sector = iso_sector(media, file.lba + (uint32_t)(offset / ISO_BLOCK));
        memcpy(out, sector + within, chunk);
        out += chunk;
        offset += chunk;
        count -= chunk;
    }
}

int musashi_disc_media_read_root_range(const MusashiDiscMedia *media, const char *name,
                                       size_t offset, uint8_t *out, size_t count,
                                       size_t *file_size) {
    IsoExtent file;
    if (!file_size || (!out && count) || !root_file_extent(media, name, &file) ||
        offset > file.size || count > file.size - offset) return 0;
    copy_root_range(media, file, offset, out, count);
    *file_size = file.size;
    return 1;
}

int musashi_disc_media_read_root_file(const MusashiDiscMedia *media, const char *name,
                                      uint8_t *out, size_t max, size_t *size) {
    IsoExtent file;
    if (!out || !size || !root_file_extent(media, name, &file) || file.size > max) return 0;
    copy_root_range(media, file, 0, out, file.size);
    *size = file.size;
    return 1;
}

enum {
    RESIDENT_TEXT_SIZE = 0x64800,
    MAIN12_BYTES = 0x5db28,
    MAIN12_MENU_WORD = 0x80182638u - 0x80128158u,
    MAIN12_STATE = 0x801853f0u - 0x80128158u,
    MAIN12_MAILBOX = 0x80185adcu - 0x80128158u
};

/* The same cached guest span and file layout accepted by musashi_boot_map_exe. */
static const uint32_t resident_text_start = 0x80010000u;

static const uint8_t *mounted_resident_text(const MusashiDiscMedia *media) {
    IsoExtent exe;
    if (!root_file_extent(media, "SLUS_007.26", &exe) ||
        exe.size != ISO_BLOCK + RESIDENT_TEXT_SIZE) return NULL;
    const uint8_t *header = iso_sector(media, exe.lba);
    if (memcmp(header, "PS-X EXE", 8) || le32(header + 0x18) != resident_text_start ||
        le32(header + 0x1c) != RESIDENT_TEXT_SIZE) return NULL;
    return iso_sector(media, exe.lba + 1u);
}

/* A logical range is bounded during opening. The 2048-byte data payloads
 * lie 2352 bytes apart in the raw disc image. */
static const uint8_t *transfer_byte(const uint8_t *transfer, size_t offset) {
    return transfer + offset / ISO_BLOCK * MUSASHI_DISC_RAW_SECTOR_SIZE + offset % ISO_BLOCK;
}

static const uint8_t *mounted_main12_transfer(const MusashiDiscMedia *media) {
    IsoExtent archive;
    if (!root_file_extent(media, "MAIN.CD", &archive) || archive.size < ISO_BLOCK) return NULL;
    const uint8_t *directory = iso_sector(media, archive.lba);
    const uint32_t member = 12u;
    uint32_t count = le32(directory);
    if (count <= member || count > (ISO_BLOCK - 8u) / 8u) return NULL;
    const uint8_t *row = directory + 8u + 8u * member;
    uint32_t sector = le32(row), size = le32(row + 4);
    if (!sector || sector > archive.size / ISO_BLOCK || size != 0x86000u ||
        size > archive.size - sector * ISO_BLOCK) return NULL;

    uint32_t lba = archive.lba + sector;
    const uint8_t *first = iso_sector(media, lba);
    if (!first || memcmp(first, "PAC", 4) || first[4] || first[5] ||
        le32(first + 12) != 0x27800u) return NULL;
    uint32_t next = le32(first + 12);
    const uint8_t *program = iso_sector(media, lba + next / ISO_BLOCK);
    if (!program || memcmp(program, "PAC", 4) || program[4] != 1 || program[5] != 1) return NULL;
    uint32_t length = le32(program + 12);
    if (length != 0x5e327u || length > size - next) return NULL;
    uint32_t start = next + ISO_BLOCK;
    if (((length - ISO_BLOCK + 3u) & ~3u) != MAIN12_BYTES || MAIN12_BYTES > size - start)
        return NULL;
    IsoExtent body = {lba + start / ISO_BLOCK, MAIN12_BYTES};
    if (!iso_extent_fits(media, &body)) return NULL;
    const uint8_t *data = iso_sector(media, body.lba);
    const uint8_t *state = transfer_byte(data, MAIN12_STATE);
    if (state[0] || state[1]) return NULL;
    return data;
}

static int transfer_range_matches(const uint8_t *source, const uint8_t *live,
                                  size_t first, size_t end) {
    while (first < end) {
        size_t chunk = ISO_BLOCK - first % ISO_BLOCK;
        if (chunk > end - first) chunk = end - first;
        if (memcmp(live + first, transfer_byte(source, first), chunk)) return 0;
        first += chunk;
    }
    return 1;
}

static int mounted_owner_live(const MusashiMain12Owner *owner) {
    return owner && owner->image && owner->memory && owner->image->identity == owner->image &&
        owner->image->memory == owner->memory && owner->image->main12_owner == owner;
}

static int mounted_resident_matches(const MusashiMain12Owner *owner, uint32_t start,
                                    uint32_t size) {
    if (!mounted_owner_live(owner) || !owner->exe_text || !size || ((start | size) & 3u) ||
        start < resident_text_start) return 0;
    uint32_t offset = start - resident_text_start;
    if (offset >= RESIDENT_TEXT_SIZE || size > RESIDENT_TEXT_SIZE - offset) return 0;
    const uint8_t *live = owner->memory->bytes + (resident_text_start - 0x80000000u);
    return transfer_range_matches(owner->exe_text, live, offset, offset + size);
}

static int musashi_main12_transfer_matches(const MusashiMain12Owner *owner, const uint8_t *live,
                                           int installed, const uint8_t mailbox[16]) {
    if (!mounted_owner_live(owner) || !owner->transfer || !live ||
        (installed != 0 && installed != 1)) return 0;
    if (live[MAIN12_STATE] != (uint8_t)installed ||
        (mailbox && memcmp(live + MAIN12_MAILBOX, mailbox, 16))) return 0;
    const uint8_t *source = owner->transfer;
    if (installed && owner->menu_word_changed) {
        if (le32(live + MAIN12_MENU_WORD) != 10u ||
            !transfer_range_matches(source, live, 0, MAIN12_MENU_WORD) ||
            !transfer_range_matches(source, live, MAIN12_MENU_WORD + 4u, MAIN12_STATE))
            return 0;
    } else if (!transfer_range_matches(source, live, 0, MAIN12_STATE)) return 0;
    if (!mailbox) return transfer_range_matches(source, live, MAIN12_STATE + 1u, MAIN12_BYTES);
    if (!transfer_range_matches(source, live, MAIN12_STATE + 1u, MAIN12_MAILBOX)) return 0;
    if (!installed || !owner->row_pointer_written)
        return transfer_range_matches(source, live, MAIN12_MAILBOX + 16u, MAIN12_BYTES);
    const uint32_t cell = MUSASHI_MAIN12_ROW_CELL - 0x80128158u;
    return le32(live + cell) == MUSASHI_MAIN12_ROW_DATA &&
        transfer_range_matches(source, live, MAIN12_MAILBOX + 16u, cell) &&
        transfer_range_matches(source, live, cell + 4u, MAIN12_BYTES);
}

MusashiMain12Owner *musashi_main12_open(MusashiGuestIrqImage *image,
    MusashiBootMemory *memory, const MusashiDiscMedia *media) {
    if (!media || !media->image || !image || !memory || image->identity != image ||
        image->memory != memory || image->main12_owner) return NULL;
    const uint8_t *transfer = mounted_main12_transfer(media);
    const uint8_t *exe_text = mounted_resident_text(media);
    if (!transfer || !exe_text) return NULL;
    MusashiMain12Owner *owner = calloc(1, sizeof *owner);
    if (owner) {
        owner->image = image;
        owner->memory = memory;
        owner->transfer = transfer;
        owner->matches = musashi_main12_transfer_matches;
        owner->exe_text = exe_text;
        owner->resident_matches = mounted_resident_matches;
        image->main12_owner = owner;
    }
    return owner;
}

void musashi_main12_close(MusashiMain12Owner *owner) {
    if (!owner) return;
    if (owner->image->main12_owner == owner) owner->image->main12_owner = NULL;
    free(owner);
}

int musashi_disc_media_get_info(const MusashiDiscMedia *media,
                                MusashiDiscMediaInfo *info) {
    if (!media || !media->image || !info) return 0;
    *info = media->info;
    return 1;
}

int musashi_disc_media_read_sector(const MusashiDiscMedia *media,
                                   uint32_t file_frame, void *output,
                                   size_t output_size) {
    size_t offset;

    if (!media || !media->image || !output ||
        output_size < MUSASHI_DISC_RAW_SECTOR_SIZE ||
        file_frame >= media->info.total_frames)
        return 0;
    offset = (size_t)file_frame * MUSASHI_DISC_RAW_SECTOR_SIZE;
    memcpy(output, media->image + offset, MUSASHI_DISC_RAW_SECTOR_SIZE);
    return 1;
}

void musashi_disc_media_close(MusashiDiscMedia *media) {
    if (!media) return;
    free(media->image);
    media->image = NULL;
    free(media);
}
