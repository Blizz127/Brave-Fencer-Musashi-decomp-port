/* Resident sound startup. Record sizes and field widths describe the game's
 * writable state. Service implementations are supplied by the caller's build. */
#include "game_track_state.h"

typedef unsigned char Byte;
typedef short Half;
typedef int Word;

typedef struct {
    Word mask;
    Half left, right, field8, field10;
    Byte gap12[4];
    Half output_left, output_right;
    Word field20, enabled, field28, field32, field36;
} SoundMix;

typedef struct { Word value; Byte reserved[8]; } SoundBank;
typedef struct {
    Half field0, channel, active;
    Byte gap6[10], available, gap17[7];
} SoundSlot;
typedef struct { Half state; Byte tail[82]; } SoundEntry;
typedef struct {
    Byte leading[4];
    Word word0;
    Half padding4, voice;
    Byte gap8[52];
    Word word60;
    Byte gap64[10], status, gap75, active, index, tail78[2];
} SoundChannel;
typedef struct { Word voice; Byte a, b, tail[66]; } SoundVoice;
typedef struct { Word enabled, value; } SoundOption;

extern void func_8003A424(void), func_8003D518(void), func_80037D98(void);
extern void func_8003C598(SoundMix *mix);
extern void func_8003BE24(int mode), func_8002D1F0(Half count), func_8003B280(int mode);
extern void func_8002CC4C(void), func_8002FAE0(void), func_80037CC8(void);
extern void func_8003BE74(int mode, unsigned int mask), func_8003B1EC(SoundOption *option);
extern void func_80034C24(void), func_80037004(void);

extern SoundMix D_800A4EA4;
extern SoundBank D_800A64B0[8];
extern SoundSlot D_800A4640[];
extern SoundEntry D_800A46E8[8];
extern SoundChannel D_800A4988[8];
extern SoundVoice D_800A4C68[8];
extern Word D_800A4E68, D_800A463C;
extern Word D_800A4654, D_800A466C, D_800A4684, D_800A469C;
extern Word D_800A2B98, D_800C7D20, D_800C7D2C, D_800A2BA0;
extern Word D_800A4EE8, D_800A4EEC, D_800A4F24;
extern Half D_800A4E6C, D_800A4E6E, D_800A4EF6, D_800A4EFA, D_800A4EF8;
extern Half D_800A4E8E, D_800A4EA2, D_800A4EF0, D_800A4EFC, D_800A4F20;
extern Half D_800A4F22, D_800A4EE0, D_800A4EE4;
extern Byte D_800A4E70, D_800A4F18, D_800A4F19, D_800A4E7A, D_800A46E4;
extern Byte D_800A4F1B, D_800A46BA, D_800A4F17, D_800A4F1D, D_800A4EE6;
extern Byte D_800A4F16, D_800A4F1C, D_800A4F1E;

void func_8002C8F4(void)
{
    SoundOption option;
    int bank_offset, slot, entry_offset;
    int voice;
    SoundChannel *channel;

    func_8003A424();
    func_8003D518();
    D_800A4EA4.mask = 0x23cf;
    D_800A4EA4.left = 0x3fff;
    D_800A4EA4.right = 0x3fff;
    D_800A4EA4.output_left = 0x3fff;
    D_800A4EA4.output_right = 0x3fff;
    D_800A4EA4.field8 = 0;
    D_800A4EA4.field10 = 0;
    D_800A4EA4.field20 = 0;
    D_800A4EA4.enabled = 1;
    D_800A4EA4.field36 = 0;
    func_8003C598(&D_800A4EA4);
    func_8003BE24(1);
    func_8002D1F0(4);
    func_8003B280(1);
    func_80037D98();

    D_800A4E68 = 60;
    D_800A4E6C = 47;
    D_800A4E6E = 47;
    D_800A4E70 = 1;
    D_800A4F18 = 1;
    D_800A4F19 = 1;
    D_800A4EF6 = 1;
    D_800A463C = 0x1010;
    D_800A4638.tick = 0;
    D_800A4E7A = 0;
    D_800A46E4 = 0;
    D_800A4654 = 0x10000;
    D_800A466C = 0x14000;
    D_800A4684 = 0x18000;
    D_800A469C = 0x39f00;
    for (bank_offset = 7 * (int)sizeof(SoundBank); bank_offset >= 0; bank_offset -= (int)sizeof(SoundBank))
        ((SoundBank *)((Byte *)&D_800A64B0 + bank_offset))->value = 0;
    for (slot = 0; slot < 5; ++slot) {
        D_800A4640[slot].available = 1;
        D_800A4640[slot].active = 0;
        D_800A4640[slot].channel = -1;
    }
    for (entry_offset = 7 * (int)sizeof(SoundEntry); entry_offset >= 0; entry_offset -= (int)sizeof(SoundEntry))
        ((SoundEntry *)((Byte *)&D_800A46E8 + entry_offset))->state = 0;
    channel = D_800A4988;
    voice = 16;
    for (slot = 0; slot < 8; ++slot, ++voice, ++channel) {
        channel->index = slot;
        channel->voice = voice;
        channel->status = 0;
        channel->word0 = 0;
        channel->word60 = 0;
        channel->active = 0;
        D_800A4C68[slot].voice = voice;
        D_800A4C68[slot].a = 0;
        D_800A4C68[slot].b = 0;
    }
    func_8002CC4C();
    D_800A4EFA = 127;
    D_800A4EF8 = 127;
    D_800A4F1B = 1;
    D_800A46BA = 0;
    D_800A2B98 = 0;
    D_800C7D20 = 0;
    D_800C7D2C = 0;
    D_800A2BA0 = 0;
    D_800A4F17 = 0;
    D_800A4E8E = 0;
    D_800A4EA2 = 0;
    D_800A4EF0 = 0;
    D_800A4EE8 = 0;
    D_800A4EFC = 0;
    D_800A4F20 = 0;
    D_800A4F22 = 0;
    D_800A4F1D = 0;
    D_800A4EEC = 0;
    D_800A4EE6 = 0;
    D_800A4EE0 = 0x4000;
    D_800A4EE4 = 0x4000;
    D_800A4F24 = 0;
    D_800A4F16 = 0;
    func_8002FAE0();
    func_80037CC8();
    func_8003BE74(0, 0xffffff);
    option.enabled = 1;
    option.value = 0;
    func_8003B1EC(&option);
    D_800A4F1C = 0;
    D_800A4F1E = 0;
    func_80034C24();
    func_80037004();
}
