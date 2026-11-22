/* Consume one CD data notification for the game stream. */
typedef unsigned char Byte;
typedef unsigned short Half;
typedef unsigned int Word;
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ Address;
#else
typedef unsigned long Address;
#endif

typedef struct {
    short x, y, w, h;
} TileRect;

typedef struct {
    Word main_state, reserved4, mode, reserved12, reserved16, buffer_index;
    Word reserved24[2];
    TileRect rect;
    Word rect_cursor, track_index, skip_tracks, tracks;
    Word destination, initial_destination;
    Word reserved64[6];
    Word remaining, result, reserved96[2];
    Byte tile_map[64];
    Byte last_part, reserved169, bit, map_index;
    Word reserved172[3];
    int request_size;
} Stream;
typedef struct {
    Word signature;
    Byte kind, last_part;
    Half reserved;
    int count;
    Word size;
} PartHeader;
typedef struct {
    int track;
    Word destination;
} TrackRequest;

extern Stream D_800AE6F0;
extern Byte D_80078E40[];
extern PartHeader D_80079A70;
extern Byte D_8007A280[];
extern Word D_80072C80;
extern Word D_800AE7AC;

extern int func_80043994(Word destination, int words);
extern int func_80043B1C(const void *position);
extern void func_8002FDC8(void);
extern int func_8002FB08(int track);
extern int func_8002FC64(int bytes, void *data);
extern int func_800599B8(TileRect *area, Byte *pixels);
extern void func_80018714(int base);
extern int func_80018730(Byte *buffer);

/* Addresses passed to the disc interface are words in the game address space. */
#define GAME_ADDRESS(p) ((Word)(Address)(p))
#define CURRENT_TRACK(s) (*(TrackRequest *)(Address)((s)->track_index * 8u + (s)->tracks))
#define SIGNATURE_PAC ((Word)'P' | (Word)'A' << 8 | (Word)'C' << 16)

void func_8001A338(Byte event)
{
    Stream *s = &D_800AE6F0;
    Byte *pixels;
    int index, count;
    int rectangles_seen;
    Word sector, read_destination;

    if (event == 1) {
        func_80043994(GAME_ADDRESS(D_80078E40), 3);
        sector = func_80043B1C(D_80078E40);
        if (sector != D_800AE7AC) {
            if (s->mode == 4) func_8002FDC8();
            s->result = 2;
            return;
        }
        D_800AE7AC = sector + 1;
        switch (s->mode) {
        case 0:
            /* A part header selects the destination and transfer format. */
            func_80043994(GAME_ADDRESS(&D_80079A70), 4);
            if (D_80079A70.signature != SIGNATURE_PAC) {
                s->result = 2;
                return;
            }
            s->last_part = D_80079A70.last_part;
            switch (D_80079A70.kind) {
            case 6:
                func_80043994(GAME_ADDRESS(D_8007A280), 508);
                pixels = D_8007A280 + 144;
                count = D_80079A70.count;
                s->rect_cursor = (Word)(Address)D_8007A280;
                for (rectangles_seen = 0; rectangles_seen < count; ++rectangles_seen) {
                    TileRect *rect = (TileRect *)(Address)s->rect_cursor;
                    func_800599B8(rect, pixels);
                    rect = (TileRect *)(Address)s->rect_cursor;
                    pixels = (Byte *)(Address)((Word)(Address)pixels + (Word)((rect->w / 2) * 4));
                    s->rect_cursor += 8;
                }
                if (s->last_part) {
                    s->mode = 2;
                } else {
                    s->mode = 0;
                }
                break;
            case 0:
            case 5:
                s->buffer_index = 0;
                func_80043994(GAME_ADDRESS(s->tile_map), 16);
                s->rect.w = s->rect.h = 32;
                s->rect.x = s->rect.y = 0;
                s->bit = 1;
                s->map_index = 0;
                for (;;) {
                    if (s->map_index >= 64) {
                        if (s->last_part) {
                            s->mode = 2;
                        } else {
                            s->mode = 0;
                        }
                        break;
                    }
                    if (s->tile_map[s->map_index] & s->bit) break;
                    if (s->bit == 128) {
                        s->bit = 1;
                        ++s->map_index;
                    } else {
                        s->bit <<= 1;
                    }
                    s->rect.x = (short)(s->rect.x + 32);
                    if (s->rect.x >= 1024) {
                        s->rect.x = 0;
                        s->rect.y = (short)(s->rect.y + 32);
                    }
                }
                ++s->mode;
                break;
            case 7:
                if (s->request_size < 0) goto complete;
                s->mode = 3;
                s->remaining = (D_80079A70.size - 2048u + 3u) >> 2;
                s->destination = D_80072C80;
                break;
            case 1:
                s->mode = 3;
                s->remaining = (D_80079A70.size - 2048u + 3u) >> 2;
                s->destination = s->initial_destination;
                break;
            case 8:
                s->mode = 6;
                s->remaining = (D_80079A70.size - 2048u + 3u) >> 2;
                break;
            case 2:
                if (s->skip_tracks) goto complete;
                s->mode = 3;
                /* Save the destination before updating the shared remaining count. */
                read_destination = CURRENT_TRACK(s).destination;
                s->remaining = (D_80079A70.size - 2048u + 3u) >> 2;
                s->destination = read_destination;
                break;
            case 3:
                if (s->skip_tracks) goto complete;
                do {
                    index = CURRENT_TRACK(s).track;
                } while (!func_8002FB08(index));
                s->mode = 4;
                s->remaining = D_80079A70.size - 2048;
                break;
            case 4:
                s->mode = 5;
                s->remaining = (D_80079A70.size - 2048u + 3u) >> 2;
                s->destination = s->initial_destination;
                func_80018714((int)s->destination);
                break;
            }
            break;
        case 1:
            /* One set bit selects one 32-by-32 tile. */
            pixels = (Byte *)(Address)((Word)(Address)&D_80079A70 + (s->buffer_index << 11));
            func_80043994(GAME_ADDRESS(pixels), 512);
            func_800599B8(&s->rect, pixels);
            s->buffer_index = (s->buffer_index + 1) & 3;
            do {
                if (s->bit == 128) {
                    s->bit = 1;
                    ++s->map_index;
                } else {
                    s->bit <<= 1;
                }
                if (s->map_index >= 64) {
                    if (s->last_part) {
                        s->mode = 2;
                    } else {
                        s->mode = 0;
                    }
                    break;
                }
                s->rect.x = (short)(s->rect.x + 32);
                if (s->rect.x >= 1024) {
                    s->rect.x = 0;
                    s->rect.y = (short)(s->rect.y + 32);
                }
            } while (!(s->tile_map[s->map_index] & s->bit));
            break;
        case 3:
            /* Raw reads consume 512 words per sector and a shorter final block. */
            if ((int)s->remaining <= 512) {
                func_80043994(s->destination, (int)s->remaining);
                if (s->last_part) {
                    s->mode = 2;
                } else {
                    s->mode = 0;
                }
                break;
            }
            func_80043994(s->destination, 512);
            s->destination += 2048;
            s->remaining -= 512;
            break;
        case 4:
            if ((int)s->remaining <= 2048) {
                int words = (int)(s->remaining + 3) / 4;
                func_80043994(GAME_ADDRESS(&D_80079A70), words);
                if (!func_8002FC64((int)s->remaining, &D_80079A70)) goto discard;
                ++s->track_index;
                if (s->last_part) goto complete;
                s->mode = 0;
                break;
            }
            func_80043994(GAME_ADDRESS(&D_80079A70), 512);
            index = func_8002FC64(2048, &D_80079A70);
            s->destination += 2048;
            s->remaining -= 2048;
            if (!index) {
discard:
                func_8002FDC8();
                s->mode = 7;
            }
            break;
        case 5:
            func_80043994(GAME_ADDRESS(&D_80079A70), 512);
            if (!func_80018730((Byte *)&D_80079A70)) {
                if (s->last_part) {
                    s->mode = 2;
                } else {
                    s->mode = 0;
                }
            }
            break;
        case 6:
            s->remaining -= 512;
            if ((int)s->remaining <= 0) {
                if (s->last_part) {
complete:
                    s->mode = 2;
                } else {
                    s->mode = 0;
                }
            }
            break;
        }
        if (s->mode == 2) s->result = 1;
        else if (s->mode == 7) s->result = 2;
    }
    if (event == 5) {
        if (s->mode == 4) func_8002FDC8();
        s->result = 2;
    }
}
