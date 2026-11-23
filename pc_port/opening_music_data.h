#ifndef BFM_OPENING_MUSIC_DATA_H
#define BFM_OPENING_MUSIC_DATA_H

#include "musashi_boot_memory.h"
#include <stdint.h>

/* Authorization only. The caller authenticates the particular game call,
 * root SP and return chain, and applies the checked access itself. The owner
 * serializes access to memory for the entire call. No device transactions
 * are accepted by this RAM predicate. */
enum {
    MUSIC_PLAYERS = 0x800b9cd8u, MUSIC_PLAYER_BYTES = 0x1fcu,
    MUSIC_VOICES = 0x800c6dd0u, MUSIC_VOICE_BYTES = 0x60u,
    MUSIC_BANK = 0x800ba320u, MUSIC_INSTRUMENTS = 0x800c1320u,
    MUSIC_EVENTS = 0x800be45eu, MUSIC_EVENTS_END = 0x800bea97u
};

static int music_data_read(MusashiBootMemory *mem, uint32_t address,
                           unsigned width, uint32_t *out)
{
    uint8_t *p = musashi_boot_ram_span(mem, address, width);
    if (!p) return 0;
    uint32_t v = 0;
    for (unsigned i = 0; i < width; ++i)
        v |= (uint32_t)p[i] << (8u * i);
    *out = v;
    return 1;
}

static int music_data_equals(MusashiBootMemory *mem, uint32_t address,
                             unsigned width, uint32_t expected)
{
    uint32_t actual;
    return music_data_read(mem, address, width, &actual) && actual == expected;
}

/* 800373d0 passes the sequence buffer and buf+0x7000 to distinct services.
 * 8002d904 selects the track's bank and MIDI object; 80038210/800387c0
 * place the event payload start into the selected player's cursor and start.
 * The one admitted ordinary opening track has the following structural
 * metadata. It is checked independently of mutable cursor/loop pointers.
 * Scalar comparisons of MIDI header bytes below use little-endian RAM reads. */
typedef struct MusicDataTrack {
    uint32_t start, end;
} MusicDataTrack;

static int music_track_range(MusashiBootMemory *mem, uint32_t player, MusicDataTrack *track)
{
    uint32_t id, bank_slot, sequence, bank_offset, bank_handle, midi, size, raw, saved;
    uint32_t player_index = (player - MUSIC_PLAYERS) / MUSIC_PLAYER_BYTES;
    /* 0x91 is a documented, exercised index into the contiguous 16-byte
     * track table (8002d904). It supplies a conservative witnessed high
     * water mark. Index 0x8e uses the separate special pointer. */
    if ((player != MUSIC_PLAYERS && player != MUSIC_PLAYERS+MUSIC_PLAYER_BYTES) ||
        !music_data_equals(mem,player+0x1fb,1,1) ||
        !music_data_equals(mem,0x800a4e86u,2,player_index) ||
        !music_data_equals(mem,0x800a4e7cu,4,MUSIC_BANK) ||
        !music_data_read(mem,0x800a4e8cu,2,&id) || id > 0x91u || id == 0x8eu)
        return 0;
    uint32_t row = 0x80068b5cu + 16u * id;
    if (!music_data_read(mem,row,2,&bank_slot) || bank_slot >= 5u ||
        !music_data_read(mem,0x800a4642u+24u*bank_slot,2,&bank_handle) ||
        bank_handle >= 0x8000u || !music_data_equals(mem,player+0x1ec,2,bank_handle) ||
        !music_data_read(mem,row+6,2,&sequence) || sequence > 3u)
        return 0;
    midi = MUSIC_BANK + 0x10u;
    if (sequence) {
        if (!music_data_read(mem,MUSIC_BANK+4u*sequence,4,&bank_offset) ||
            bank_offset < 0x10u || bank_offset > 0x7000u-26u)
            return 0;
        midi = MUSIC_BANK + bank_offset + 4u;
    }
    /* A type-0, single-track MIDI chunk. Header scalars are big-endian.
     * 800387c0 consumes MTrk and its length, 80038210 saves its first
     * payload byte. Neither the starting pointer nor the extent can select
     * unrelated mapped RAM. Check the independently known opening size too. */
    uint32_t start = midi + 22u;
    if (!music_data_equals(mem,midi,4,0x6468544du) ||
        !music_data_equals(mem,midi+4,4,0x06000000u) ||
        !music_data_equals(mem,midi+8,2,0) || !music_data_equals(mem,midi+10,2,0x0100u) ||
        !music_data_read(mem,midi+12,2,&raw) || !raw ||
        !music_data_equals(mem,midi+14,4,0x6b72544du) ||
        !music_data_read(mem,midi+18,4,&raw) ||
        !music_data_read(mem,player+4,4,&saved) || saved != start)
        return 0;
    size = (raw << 24) | ((raw & 0xff00u) << 8) | ((raw >> 8) & 0xff00u) | (raw >> 24);
    if (!size || size > MUSIC_INSTRUMENTS-start ||
        (id == 0x91u && (start != MUSIC_EVENTS || size != MUSIC_EVENTS_END-MUSIC_EVENTS)))
        return 0;
    track->start = start;
    track->end = start+size;
    return 1;
}

/* Consumers may advance their cursor before or after the byte load. */
static int music_event_byte(MusashiBootMemory *mem, uint32_t address)
{
    uint32_t index, cursor;
    MusicDataTrack track;
    if (!music_data_read(mem,0x800a4e86u,2,&index) || index >= 2) return 0;
    uint32_t player = MUSIC_PLAYERS+MUSIC_PLAYER_BYTES*index;
    if (!music_track_range(mem,player,&track) || !music_data_read(mem, player, 4, &cursor) ||
        address < track.start || address >= track.end || cursor < track.start || cursor > track.end)
        return 0;
    return address == cursor || address + 1u == cursor || address + 2u == cursor;
}

static int music_cursor_store(MusashiBootMemory *mem, uint32_t player,
                              unsigned field, uint32_t next)
{
    uint32_t cursor;
    MusicDataTrack track;
    if (!music_track_range(mem,player,&track) ||
        !music_data_read(mem,player,4,&cursor) || cursor < track.start || cursor > track.end ||
        next < track.start || next > track.end)
        return 0;
    if (field == 0x0c) return next == cursor && next < track.end;
    return (next >= cursor && next - cursor <= 2u) || next == track.start ||
           (next < track.end && music_data_equals(mem,player+0x0c,4,next));
}

/* SeqPlayer and SeqChannel field widths from 80038210, 80038838, 80038908,
 * 80039c70, 80039dec, 80039f50, 8003a0e4. The allocator's bank pointers,
 * stream start, allocation flag and identity are read-only during a sweep. */
static int music_player_field(MusashiBootMemory *mem, uint32_t player,
                              unsigned off, unsigned width, int write, uint32_t value)
{
    if (off == 0x1fa || off == 0x1fb)
        return width == 1 && (!write || (value == 0 &&
               music_data_equals(mem,player+0x1fb,1,1)));
    if (!music_data_equals(mem, player+0x1fb, 1, 1)) return 0;
    /* The mounted sequencer may read any aligned scalar in a live player.
     * The program also keeps per-player timing between the published fields. */
    if (!write) return width <= MUSIC_PLAYER_BYTES-off;
    if (off < 0x1a) {
        if (off == 0 || off == 0x0c)
            return width == 4 && (!write || music_cursor_store(mem,player,off,value));
        if (off == 4) return width == 4 && !write;
        if (off == 8) return width == 4;
        if (off == 0x10 || off == 0x12) return width == 2;
        return off == 0x18 && width == 1;
    }
    if (off < 0x1ba) {
        unsigned ch = (off - 0x1a) % 0x1a;
        if (ch == 2) return width == 2;
        if (ch == 6 && width == 2) return !write; /* 80039c70 reads combined flags */
        if (ch == 0) return width == 1 && value <= 127u;
        if (ch == 1 || (ch >= 4 && ch <= 8)) return width == 1;
        if (ch >= 9 && ch < 25) return width == 1 && (!write || value <= 1u);
        return 0;
    }
    if (off < 0x1ca) return width == 1 && !write;
    if (off == 0x1cc || off == 0x1d0 || off == 0x1d4) return width == 4;
    if (off == 0x1d8 || off == 0x1dc || off == 0x1e0 || off == 0x1e8)
        return width == 4 && !write;
    if (off == 0x1ec || off == 0x1f0 || off == 0x1f2) return width == 2 && !write;
    if (off == 0x1e4) return width == 2;
    return off >= 0x1f4 && off <= 0x1f9 && width == 1;
}

/* 80037d98, 80037fc4, 80038958, 8003916c and 8003a0e4 describe the
 * game voices. An initialization store targets a documented field of an
 * idle slot, or a slot already bound to the current sequence player. */
static int music_voice_field(MusashiBootMemory *mem, uint32_t voice, unsigned off,
                             unsigned width, int write, uint32_t value)
{
    uint32_t owner_index, owner=0, flags;
    int selected=music_data_read(mem,0x800a4e86u,2,&owner_index) && owner_index < 2;
    if (selected) {
        owner=MUSIC_PLAYERS+owner_index*MUSIC_PLAYER_BYTES;
        selected=music_data_equals(mem,owner+0x1fb,1,1) &&
                 music_data_equals(mem,owner+0x1fa,1,1);
    }
    unsigned slot = (voice-MUSIC_VOICES)/MUSIC_VOICE_BYTES;
    int can_setup=selected && slot < 16u && music_data_equals(mem,voice+0x10,4,1u<<slot) &&
                             (music_data_equals(mem,voice+0x5a,1,0) ||
                              music_data_equals(mem,voice+0x50,4,owner));
    /* The game initializes its per-note scalar parameters before handing
     * its embedded device attribute record to the checked voice setter. */
    if ((off < 0x10u && off != 4 && off != 6) ||
        (off >= 0x18u && off < 0x50u && off != 0x1cu && off != 0x1eu && off != 0x24u))
        return (!write || can_setup) && off+width <= (off < 0x10u ? 0x10u : 0x50u);
    switch (off) {
    case 0x04: return width == 2 && (!write || (can_setup && value <= 127u));
    case 0x06: return width == 2 && (!write || (can_setup && value < 16u));
    case 0x10: return width == 4 && !write;
    case 0x50: return width == 4 && (!write || (can_setup && value == owner));
    case 0x54: return width == 4 && (!write || can_setup);
    case 0x14:
        if (width != 4 || !write) return width == 4;
        if (!music_data_read(mem,voice+off,4,&flags)) return 0;
        return value == 0 || value == (flags & 0xfff9ffffu) ||
               (can_setup && (value == (flags | 0x10u) || value == 0x6009fu));
    case 0x1c: case 0x1e: return width == 2 && (!write || can_setup);
    case 0x24: return width == 2 && (!write || (can_setup && value <= 0x3fffu));
    case 0x58: case 0x59: return width == 1 && (!write || (can_setup && value <= 127u));
    case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e:
        return width == 1 && (!write || value == 0 || (can_setup && value == 1));
    default: return 0;
    }
}

static int music_pending_mask(MusashiBootMemory *mem, uint32_t address,
                              unsigned width, int write, uint32_t value)
{
    uint32_t old;
    if (!write) return width == 2 || width == 4;
    if (width != 4 || !musashi_boot_read32(mem,address,&old)) return 0;
    /* Sweep owns only the sixteen low voices. Keep unrelated upper
     * bits during a bit update; a low-half flush preserves 16..23 only. */
    return ((old ^ value) & 0xffff0000u) == 0 ||
           ((old & 0xffffu) != 0 && value == (old & 0x00ff0000u));
}

static int music_data_access(MusashiBootMemory *mem, uint32_t root_sp, uint32_t sp,
                             uint32_t address, unsigned width, int write, uint32_t value)
{
    if (!mem || (width != 1 && width != 2 && width != 4) || (address & (width-1u)) ||
        (write != 0 && write != 1) || !musashi_boot_ram_span(mem,address,width))
        return 0;
    /* The enclosing IRQ gate supplies the entry stack for this call. The
     * sweep and all suspended callees share a bounded downward-growing
     * region and the O32 caller's 16-byte incoming argument home. */
    if (root_sp != 0x8006ca94u || sp > root_sp || root_sp-sp > 0x400u || (sp & 3u))
        return 0;
    if (address >= sp && address <= root_sp+16u-width) return 1;
    if (width == 1) value &= 0xffu;
    if (width == 2) value &= 0xffffu;
    if (address >= MUSIC_PLAYERS && address < MUSIC_PLAYERS+2u*MUSIC_PLAYER_BYTES) {
        unsigned off = (address - MUSIC_PLAYERS) % MUSIC_PLAYER_BYTES;
        return music_player_field(mem,address-off,off,width,write,value);
    }
    if (address >= MUSIC_VOICES && address < MUSIC_VOICES+16u*MUSIC_VOICE_BYTES) {
        unsigned off = (address - MUSIC_VOICES) % MUSIC_VOICE_BYTES;
        return music_voice_field(mem,address-off,off,width,write,value);
    }
    if (address >= MUSIC_BANK && address < MUSIC_INSTRUMENTS)
        return !write && width == 1 && music_event_byte(mem,address);

    /* 80038210 binds a VAB header, 128 program records, then the
     * 16 tone records allocated per program. The next game object
     * (800c5320) caps the bank even if its header is damaged. */
    if (address >= MUSIC_INSTRUMENTS && address < MUSIC_INSTRUMENTS+0x4000u) {
        uint32_t ix;
        if (write || !music_data_read(mem,0x800a4e86u,2,&ix) || ix >= 2) return 0;
        uint32_t player=MUSIC_PLAYERS+ix*MUSIC_PLAYER_BYTES;
        if (!music_data_equals(mem,player+0x1fb,1,1) ||
            !music_data_equals(mem,player+0x1d8,4,MUSIC_INSTRUMENTS) ||
            !music_data_equals(mem,player+0x1dc,4,MUSIC_INSTRUMENTS+0x20u) ||
            !music_data_equals(mem,player+0x1e0,4,MUSIC_INSTRUMENTS+0x820u)) return 0;
        if (address < MUSIC_INSTRUMENTS+0x820u) return 1;
        uint32_t programs;
        if (!music_data_equals(mem,MUSIC_INSTRUMENTS,4,0x56414270u) ||
            !music_data_read(mem,MUSIC_INSTRUMENTS+18u,2,&programs) ||
            !programs || programs > (0x4000u-0x820u)/0x200u) return 0;
        return address+width <= MUSIC_INSTRUMENTS+0x820u+programs*0x200u;
    }

    if (address == 0x800a2b98u || address == 0x800a2ba0u ||
        address == 0x800c7d20u || address == 0x800c7d2cu)
        return music_pending_mask(mem,address,width,write,value);

    /* Current request/timer/count flags, simple driver controls and fixed
     * pitch/key lookup tables are distinct objects with different widths. */
    /* 80039c70 and 8002d240 publish the same sound request. */
    if (address == 0x800a4eccu)
        return width == 4 && (!write || value == 1 || value == 6);
    if (address == 0x800a4ed0u) return width == 4 && (!write ||
        music_data_equals(mem,0x800a4eccu,4,1));
    if (address == 0x800a4ed4u || address == 0x800a4ed6u)
        return width == 2 && (!write || music_data_equals(mem,0x800a4eccu,4,6));
    if (address >= 0x8006add8u && address < 0x8006aed8u) return width == 2 && !write;
    if (address >= 0x800a4f17u && address <= 0x800a4f1cu) return width == 1 && !write;
    if (address == 0x800a4f16u) return width == 1 && (!write || value == 0);
    if (address == 0x800a4f1du) return width == 1 && (!write || value <= 1u);
    if (address >= 0x80068b66u && address <= 0x80069476u &&
        (address-0x80068b66u) % 16u == 0)
        return !write && width == 2 &&
               music_data_equals(mem,0x800a4e8cu,2,(address-0x80068b66u)/16u);
    if (address == 0x800a4f20u || address == 0x800a4f22u)
        return width == 2;
    if (address == 0x800a4e86u || address == 0x800a4e8cu || address == 0x800a4ee0u || address == 0x800a4ef8u)
        return width == 2 && !write;
    if (address >= 0x800a4efeu && address < 0x800a4f0eu)
        return width == 1 && !write && music_data_equals(mem,0x800762b0u,4,0x800a4efeu);
    if (address >= 0x800762b4u && address < 0x800762c4u) {
        uint32_t index, slot = address-0x800762b4u;
        if (width != 1 || !write || value != 1) return width == 1 && (!write || value == 0);
        return music_data_read(mem,0x800a4e86u,2,&index) && index < 2u &&
            music_data_equals(mem,MUSIC_PLAYERS+index*MUSIC_PLAYER_BYTES+0x1fau,1,1) &&
            music_data_equals(mem,MUSIC_VOICES+slot*MUSIC_VOICE_BYTES+0x10u,4,1u<<slot);
    }
    if (address == 0x800762b0u || address == 0x800a6428u)
        return width == 4 && !write;
    if (address >= 0x80073140u && address < 0x80073180u)
        return width == 4 && !write;
    if ((address >= 0x8006ab30u && address < 0x8006abd8u) ||
        (address >= 0x8006abd8u && address < 0x8006add8u))
        return width == 2 && !write;
    return 0;
}
#endif
