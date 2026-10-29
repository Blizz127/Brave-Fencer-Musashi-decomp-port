/* Owner re-derivation from retail assembly (clean input set). */
/* Applies a volume (0x1000) or pan (0x2000) change to the channels used by a playing kind-1 sound slot. */
#include "psx_types.h"

typedef struct {
    u16 kind;
    u8 pad2[0xC - 2];
    u16 voice_count;
    u8 voice_used[8];
    u8 pad16[0x54 - 0x16];
} SoundSlot;

typedef struct {
    u8 pad0[0x35];
    u8 stereo;
    u8 pad36[0x48 - 0x36];
    s16 volume;
    u8 pad4A[0x4F - 0x4A];
    u8 volume_dirty;
    u8 pad50[0x53 - 0x50];
    u8 pan;
} SoundChannel;

typedef struct {
    SoundSlot slots[8];
    SoundChannel channels[8];
} SoundTables;

extern SoundTables D_800A46E8;

void func_800335B8(s32 index, u16 command) {
    SoundSlot *slot;
    SoundChannel *ch;
    s32 i;
    s32 left;

    slot = &D_800A46E8.slots[index];
    if ((slot->kind & 0x3F) == 1) {
        if (command & 0x1000) {
            left = slot->voice_count;
            for (i = 0; i < 8; i++) {
                if (left == 0) {
                    break;
                }
                if (slot->voice_used[i] != 0) {
                    ch = &D_800A46E8.channels[i];
                    left--;
                    ch->volume = command & 0x7F;
                    ch->volume_dirty = 1;
                }
            }
        } else if (command & 0x2000) {
            left = slot->voice_count;
            for (i = 0; i < 8; i++) {
                if (left == 0) {
                    break;
                }
                if (slot->voice_used[i] != 0) {
                    ch = &D_800A46E8.channels[i];
                    if (ch->stereo != 0) {
                        ch->pan = command & 0x7F;
                    }
                    left--;
                }
            }
        }
    }
}
