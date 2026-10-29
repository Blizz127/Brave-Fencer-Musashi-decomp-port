/* Owner re-derivation from retail assembly (clean input set). */
/* Boot-time disc index load: reads the header sector after lba, skips the header's data, reads the
 * table-of-contents sector into the 0x80180000 buffer, builds the file positions, then reads the
 * 0xE40-byte directory and fills the D_800AE830 slot table (position + length per file). */
#include "psx_types.h"

/* libcd CdlLOC (4 bytes). */
typedef struct {
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdLoc;

/* Header sector image; only the size word at 0x1C is used. */
typedef struct {
    u8 data[0x88];
} DiscHeader;

/* 0x30-byte file record; starts with its disc position. */
typedef struct {
    u8 data[0x30];
} FileRec;

/* Table-of-contents / directory word pair at the 0x80180000 sector buffer. */
typedef struct {
    s32 ofs;
    s32 len;
} TocEntry;

/* 8-byte records at 0x800AE830. */
typedef struct {
    CdLoc pos;
    s32 len;
} Slot;

#define SECTOR_BUF 0x80180000
#define TOC ((TocEntry *)SECTOR_BUF)

extern s32 D_800747D0;
extern s32 D_80063074;
extern s32 D_800747D8;
extern FileRec D_80062C38[];
extern Slot D_800AE830[];
extern void func_80019990(void);
extern s32 func_80019A24(void *loc, s32 dest, s32 size, s32 wait);
extern void func_8004239C(s32);
extern void func_80043A18(s32 lba, void *loc);
extern s32 func_80043B1C(void *loc);

void func_800193B8(s32 lba)
{
    CdLoc loc;
    DiscHeader hdr;
    s32 i;
    s32 j;
    s32 k;
    s32 n;
    s32 slot;
    s32 base;
    s32 next;

    func_80019990();
    lba++;
    D_800747D0 = 0;
    D_80063074 = 0;
    D_800747D8 = 0;
    func_80043A18(lba, &loc);
    while (func_80019A24(&loc, SECTOR_BUF, 0x800, 0) == 0) {
        func_8004239C(0);
    }
    hdr = *(DiscHeader *)SECTOR_BUF;
    next = lba + 1;
    lba = next + ((*(u32 *)&hdr.data[0x1C] + 0x7FF) >> 11); /* skip the header's data sectors */
    func_80043A18(lba, &loc);
    while (func_80019A24(&loc, SECTOR_BUF, 0x800, 0) == 0) {
        func_8004239C(0);
    }
    lba++;
    func_80043A18(lba + TOC[0].ofs, &D_80062C38[0]);
    func_80043A18(lba + TOC[1].ofs, &D_80062C38[1]);
    for (i = 2; i < 9; i++) {
        func_80043A18(lba + TOC[2].ofs, &D_80062C38[i]);
    }
    func_80043A18(lba + TOC[3].ofs, &D_80062C38[9]);
    func_80043A18(lba + TOC[4].ofs, &D_80062C38[20]);
    while (func_80019A24(&D_80062C38[0], SECTOR_BUF, 0xE40, 0) == 0) {
        func_8004239C(0);
    }
    slot = 0;
    k = 0;
    for (i = 0; i < 8; i++) {
        base = func_80043B1C(&D_80062C38[i + 1]);
        n = TOC[k++].ofs;
        for (j = 0; j < n; j++) {
            if (TOC[k].ofs != 0) {
                func_80043A18(TOC[k].ofs + base, &D_800AE830[slot]);
                D_800AE830[slot].len = TOC[k].len;
            }
            k++;
            slot++;
        }
    }
}
