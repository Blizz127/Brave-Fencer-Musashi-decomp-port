/* MAIN/0010 room resource and track selection. */
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ PointerBits;
#else
typedef u32 PointerBits;
#endif

typedef struct {
    u32 first;
    u32 second;
} DirectoryRow;

typedef struct {
    u8 before_room[0xA3D8];
    s16 room;
    u8 after_room[4];
    s16 branch;
} RoomState;

extern RoomState D_800AF630;
extern s16 D_800B9A08;
extern u32 D_800D34AC[];
extern s16 D_800D34AE[];
extern u32 D_800C7C60;
extern u32 D_800C7C64[];
extern u8 D_800A2E20[];
extern DirectoryRow D_800AE830[];
extern s32 D_80072C74;

extern s32 func_8002F648(void);
extern void func_8002F620(void);
extern s32 func_80029504(void);
extern s32 func_80029178(u32 flag);
extern s32 func_8002F658(void);
extern s32 func_80019A24(DirectoryRow *row, s32 destination, s32 size, u32 *track);
extern s32 func_800D00E4(s32 group);

#define ROOM_INDEX(room) ((s32)((room) & 0xFFFF0FFFu))
#define ROOM_BYTE_OFFSET(room) ((s32)((u32)ROOM_INDEX(room) << 2))
#define RESOURCE_GROUP(entry) (((entry) >> 12) & 15)

s32 func_800CFE60(void)
{
    s32 row;
    s32 track;
    s32 size;
    s32 result;
    s32 active_group;
    s32 progress;
    u32 *track_argument;
    RoomState *state = &D_800AF630;

    row = *(u32 *)((u8 *)D_800D34AC + ROOM_BYTE_OFFSET(D_800B9A08)) & 0xFFF;
    track = *(s16 *)((u8 *)D_800D34AE + ROOM_BYTE_OFFSET(D_800B9A08));
    active_group = func_8002F648();
    if ((u32)active_group != RESOURCE_GROUP(D_800D34AC[ROOM_INDEX(D_800B9A08)])) {
        if (active_group != 0) {
            func_8002F620();
        }
        size = 0;
    } else {
        size = -1;
    }

    progress = func_80029504();
    if (state->room == 0x300B && state->branch != 0) {
        track = 8;
        row = 138;
    }
    if (state->room == 0x3001 && progress < 5) {
        row = 55;
    }
    if (state->room == 0x304E && (u8)func_80029178(23)) {
        row = 297;
        track = 47;
    }
    if (state->room == 0x1077 && progress >= 680) {
        row = 337;
        track = 52;
    }
    if (state->room == 0x3021 && (u8)func_80029178(185)) {
        row = 193;
        track = 65;
    }

    if (func_8002F658() == track && size < 0) {
        track_argument = 0;
    } else {
        u32 *request = &D_800C7C60;
        *request = track;
        track_argument = request;
        D_800C7C64[0] = (u32)(PointerBits)D_800A2E20;
    }

    if (row >= 0 && D_800AE830[row].first != 0) {
        result = func_80019A24(&D_800AE830[row], D_80072C74, size, track_argument);
    } else {
        result = 1;
    }
    if (size == 0 && (result & 1)) {
        func_800D00E4(RESOURCE_GROUP(D_800D34AC[ROOM_INDEX(state->room)]));
    }
    return result;
}
