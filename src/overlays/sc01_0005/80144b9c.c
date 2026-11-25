/* Room entry and point selection in the scene overlay.
 * Object layouts use the original 32-bit pointer ABI.
 */
typedef void (*Callback)(void);

typedef struct RoomPoint {
    unsigned char unknown00[3];
    unsigned char worldValue;
    unsigned char unknown04[4];
    unsigned int roomValue;
    unsigned char *sceneData;
    unsigned short x, y, z, facing;
    Callback activate;
    unsigned char unknown1C[4];
    Callback finish;
} RoomPoint;

/* These are the portions of each game object used during room entry. */
typedef struct {
    unsigned char unknown00[4];
    RoomPoint *activePoint;
    unsigned int flags;
    unsigned int pointValue;
    unsigned char unknown10[4];
    unsigned char pending;
    unsigned char unknown15;
    unsigned char pointIndex;
    unsigned char unknown17;
    unsigned char restoreFlag;
    unsigned char unknown19[3];
    RoomPoint **points;
    unsigned short savedX, savedY, savedZ, savedFacing;
} Room;

typedef struct {
    unsigned char unknown00[0xa3d8];
    short roomId, modeA, modeB;
} RoomState;

typedef struct {
    unsigned char unknown00[0xa0];
    unsigned char pointValue;
} World;

typedef struct {
    unsigned char unknown00[0x12];
    unsigned short facing;
} Orientation;

typedef struct {
    unsigned char unknown00[0x3a];
    unsigned short field3A;
    unsigned char unknown3C[2];
    unsigned short field3E;
} Player;

typedef struct {
    unsigned char unknown00[4];
    unsigned short position[6];
    unsigned char unknown10[0x10];
    Orientation *orientation;
    unsigned char unknown24[0x29];
    unsigned char field4D;
    unsigned char unknown4E[0x3a];
    unsigned short position88[3], unknown8E, position90[3];
    unsigned char unknown96[0x1a];
    int fieldB0;
    unsigned char unknownB4[0x18];
    int fieldCC;
    unsigned char unknownD0[0x84];
    unsigned short position154[3], unknown15A, position15C[3];
    unsigned char unknown162[0x36];
    unsigned char *pointData;
    unsigned char *entryBlock;
    unsigned char unknown1A0[0x48];
    int field1E8;
    unsigned char unknown1EC[4];
    int field1F0;
} Scene;

typedef struct {
    short direction[3];
    unsigned char color[3];
    unsigned char reserved;
} LightSetting;

typedef struct {
    short ambient[3];
    LightSetting lights[3];
    unsigned short field24, field26;
    int field28, field2C, field30, field34;
    unsigned short field38, field3A, field3C, field3E;
    unsigned char color40[3], color43[3], color46[3], color49[3];
    int field4C, field50, field54, field58, field5C, field60, field64;
    unsigned char block68[0x20];
    int field88, field8C, field90, field94, field98, field9C;
    int fieldA0, fieldA4, fieldA8;
    unsigned char fieldAC[8];
    int fieldB4, fieldB8, fieldBC, fieldC0, fieldC4;
    RoomPoint **points;
    unsigned char unknownCC[8];
    int fieldD4, fieldD8;
} RoomEntry;

extern Scene D_80126B58;
extern Player D_80078E78;
extern unsigned char D_8010F5C8[];
extern RoomState D_800AF630;
extern Room D_800AE6A8;
extern World D_80126948;
extern RoomPoint D_80078E50;
extern int D_8011DB14, D_8011DB1C, D_80128150;
extern void func_80019018(void), func_80018E78(void), func_8002AB64(void);
extern RoomEntry *func_801457A4(int);

struct RoomDirectionalLight {
    int x, y, z;
    unsigned char r, g, b;
};

extern unsigned char D_80126DB0[];
extern unsigned short D_80126DB6;
extern World *D_801151D4;
extern struct RoomDirectionalLight D_800A5E88, D_800A5E98, D_800A5EA8;
extern void func_80015978(void *, void *);
extern void func_8012A1BC(void);
extern void func_8002850C(int, int, int);
extern void func_80028620(int, struct RoomDirectionalLight *);
extern unsigned short D_800B9AAC, D_800B9B00, D_800B9AB2, D_800B9B06;
extern unsigned short D_800B9AB6, D_800B9B0A;
extern int D_800B9AC4, D_800B9B18, D_800B9AC8, D_800B9B1C;
extern unsigned char D_800B9ABC, D_800B9ABD, D_800B9ABE;
extern unsigned char D_800B9B10, D_800B9B11, D_800B9B12;
extern unsigned char D_800B9ABF, D_800B9AC0, D_800B9AC1;
extern unsigned char D_800B9B13, D_800B9B14, D_800B9B15;

extern int D_80126728, D_8011DB08, D_80126AEC;
extern unsigned char D_801151F0[8];
extern int D_801274C8, D_801274CC, D_801274D0, D_801274D4, D_801270B8;
extern int D_80127520, D_801151D8, D_8011DB28;

extern void func_80129398(void);
extern void func_8012944C(void);
extern void func_801336E8(int, int, int);
extern void func_8013B83C(int, int, int);
extern void func_8013B568(int);
extern void func_800D02C0(int);
extern void func_80011E84(int);
extern void func_8013D3D4(int, int);
extern void func_8002D4C8(int, int);
extern int func_800CF854(void);
extern void func_8001C14C(void);
extern void func_8012A110(void);

void func_80144B9C(void)
{
    Scene *scene = &D_80126B58;
    Player *player = &D_80078E78;
    register RoomState *state = &D_800AF630;
    Room *room = &D_800AE6A8;
    World *world = &D_80126948;
    RoomEntry *entry;
    RoomPoint *point;
    RoomPoint **points;
    short index;
    Callback finish;
    int fresh;

    func_80019018();
    func_80018E78();
    if (room->flags == 0) {
        entry = func_801457A4(state->roomId);
        room->pointIndex = 0;
        fresh = 1;
        D_8011DB14 = player->field3A;
        D_8011DB1C = player->field3E;
        D_80128150 = 100;
        func_8002AB64();
    } else {
        entry = func_801457A4(state->roomId);
        fresh = 0;
    }

    room->points = entry->points;
    points = entry->points;
    if (room->pointIndex == 255) {
        point = &D_80078E50;
    } else {
        point = points[room->pointIndex];
    }
    room->pointValue = point->roomValue;
    room->pending = 0;
    scene->field4D = 0;
    if (point->sceneData != D_8010F5C8) {
        scene->pointData = point->sceneData;
    }

    if (state->modeA == 0x1000 && state->modeB == 0x1000) {
        scene->position[1] = scene->position88[0] = scene->position90[0] = room->savedX;
        scene->position[3] = scene->position88[1] = scene->position90[1] = room->savedY;
        scene->position[5] = scene->position88[2] = scene->position90[2] = room->savedZ;
        scene->orientation->facing = room->savedFacing;
        room->restoreFlag = 0;
    } else {
        scene->position[1] = scene->position88[0] = scene->position90[0] = point->x;
        scene->position[3] = scene->position88[1] = scene->position90[1] = point->y;
        scene->position[5] = scene->position88[2] = scene->position90[2] = point->z;
        scene->orientation->facing = point->facing;
    }
    scene->position154[0] = scene->position15C[0] = scene->position[1];
    scene->position154[1] = scene->position15C[1] = scene->position[3];
    scene->position154[2] = scene->position15C[2] = scene->position[5];
    func_80015978(scene->position, D_80126DB0);
    D_80126DB6 = scene->orientation->facing;

    if (state->modeA == 0x1000
        && (state->modeB == 0x1000 || state->modeB == 0x2000)
        && !fresh) {
        point->activate();
        func_8012A1BC();
    } else {
        if (state->modeA == 0x1000 && state->modeB == 0x3000) {
            world->pointValue = point->worldValue;
        } else {
            world->pointValue = 0;
        }
        point->activate();
    }
    if (state->roomId == 0x3069) {
        world->pointValue = point->worldValue;
    }
    D_801151D4 = world;
    func_8002850C(entry->ambient[0], entry->ambient[1], entry->ambient[2]);

    D_800A5E88.x = entry->lights[0].direction[0];
    D_800A5E88.y = entry->lights[0].direction[1];
    D_800A5E88.z = entry->lights[0].direction[2];
    D_800A5E88.r = entry->lights[0].color[0];
    D_800A5E88.g = entry->lights[0].color[1];
    D_800A5E88.b = entry->lights[0].color[2];
    func_80028620(0, &D_800A5E88);
    D_800A5E98.x = entry->lights[1].direction[0];
    D_800A5E98.y = entry->lights[1].direction[1];
    D_800A5E98.z = entry->lights[1].direction[2];
    D_800A5E98.r = entry->lights[1].color[0];
    D_800A5E98.g = entry->lights[1].color[1];
    D_800A5E98.b = entry->lights[1].color[2];
    func_80028620(1, &D_800A5E98);
    D_800A5EA8.x = entry->lights[2].direction[0];
    D_800A5EA8.y = entry->lights[2].direction[1];
    D_800A5EA8.z = entry->lights[2].direction[2];
    D_800A5EA8.r = entry->lights[2].color[0];
    D_800A5EA8.g = entry->lights[2].color[1];
    D_800A5EA8.b = entry->lights[2].color[2];
    func_80028620(2, &D_800A5EA8);
    func_80129398();
    D_800B9AAC = entry->field24;
    D_800B9B00 = entry->field26;
    D_800B9AC4 = entry->field28;
    D_800B9B18 = entry->field2C;
    D_800B9AC8 = entry->field30;
    D_800B9B1C = entry->field34;
    D_800B9AB2 = entry->field38;
    D_800B9B06 = entry->field3A;
    D_800B9AB6 = entry->field3C;
    D_800B9B0A = entry->field3E;
    D_800B9ABC = entry->color40[0];
    D_800B9ABD = entry->color40[1];
    D_800B9ABE = entry->color40[2];
    D_800B9B10 = entry->color43[0];
    D_800B9B11 = entry->color43[1];
    D_800B9B12 = entry->color43[2];
    D_800B9ABF = entry->color46[0];
    D_800B9AC0 = entry->color46[1];
    D_800B9AC1 = entry->color46[2];
    D_800B9B13 = entry->color49[0];
    D_800B9B14 = entry->color49[1];
    D_800B9B15 = entry->color49[2];
    func_8012944C();

    scene->fieldB0 = entry->field4C;
    scene->fieldCC = entry->field50;
    scene->field1E8 = entry->field54;
    scene->field1F0 = entry->field58;
    D_80126728 = entry->field5C;
    D_8011DB08 = entry->field60;
    D_80126AEC = entry->field64;
    scene->entryBlock = entry->block68;

    func_801336E8(entry->field88, entry->field8C, entry->field90);
    func_8013B83C(entry->field94, entry->field98, entry->field9C);
    func_8013B568(entry->fieldA8);
    func_800D02C0(entry->fieldD4);
    func_80011E84(entry->fieldD8);

    for (index = 0; index < 8; index++) {
        D_801151F0[index] = ((unsigned char *)entry + 0xAC)[index];
    }
    D_801274C8 = entry->fieldB4;
    D_801274CC = entry->fieldB8;
    D_801274D0 = entry->fieldBC;
    D_801274D4 = entry->fieldC0;
    D_801270B8 = entry->fieldC4;
    D_80127520 = entry->fieldC4;
    D_801151D8 = 0;
    D_8011DB28 = 0;
    finish = point->finish;
    finish();
    func_8013D3D4(entry->fieldA0, entry->fieldA4);
    func_8002D4C8(30, 0);

    if (room->flags == 0) {
        D_80078E50 = *point;
    }
    room->activePoint = &D_80078E50;
    room->flags &= 0x7F000000;
    if (func_800CF854()) {
        func_8001C14C();
        func_8012A110();
    }
}
