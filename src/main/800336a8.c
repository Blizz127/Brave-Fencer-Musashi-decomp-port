/* Recalculate the levels of the enabled sound slots in this mixing job. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef struct {
    u8 unused0[10];
    u16 id;
    u8 unused12[40];
    u8 level, pan;
    u8 unused54[28];
    u8 mixed_pan, unused83;
} Voice;
typedef struct {
    u32 mask, flags;
    u16 volume[2];
    u8 unused12[52];
    u32 id;
    u8 pending, unused69[3];
} Output;
typedef struct {
    Voice voices[8];
    Output outputs[8];
    u8 unused1248[146];
    short master;
} Mixer;
typedef struct {
    u16 gain;
    u8 unused[6];
} Ramp;
typedef struct {
    u8 unused0[22];
    u8 scaled, unused23;
    u16 scale;
    u8 unused26[2];
    Ramp ramps[3];
    u8 unused52[4];
    u8 pan, unused57;
    u8 enabled[8];
} Job;
extern Mixer D_800A4988;
extern u32 D_80073140[];
extern u16 D_8006AA30[], D_800731A0[];
extern u8 D_800A4F19;
void func_800336A8(Job *job)
{
    int i, j;
    Voice *voice;
    Output *out;
    u32 gain, pan, volume0, volume1;
    Ramp *ramp;
    for (i = 0; i < 8; ++i) {
        if (!job->enabled[i])
            continue;
        voice = &D_800A4988.voices[i];
        out = &D_800A4988.outputs[i];
        out->mask = D_80073140[voice->id];
        gain = voice->level;
        gain = D_8006AA30[gain];
        gain *= D_800A4988.master;
        gain >>= 7;
        if (job->scaled) {
            gain *= job->scale;
            gain >>= 15;
        }
        ramp = job->ramps;
        for (j = 2; j >= 0; --j, ++ramp) {
            gain *= ramp->gain;
            gain >>= 14;
        }
        pan = voice->pan;
        if (pan) {
            if (job->pan) {
                pan += job->pan;
                if (pan >= 66) {
                    pan -= 64;
                    if (pan > 127)
                        pan = 127;
                } else {
                    pan = 1;
                }
            }
            voice->mixed_pan = pan;
            if (D_800A4F19) {
                volume1 = gain * D_800731A0[pan - 1];
                volume0 = gain * D_800731A0[127 - pan];
                out->volume[1] = volume1 >> 14;
                out->volume[0] = volume0 >> 14;
            } else {
                out->volume[0] = out->volume[1] = (gain * D_800731A0[63]) >> 14;
            }
        } else {
            out->volume[0] = out->volume[1] = gain;
        }
        out->id = voice->id;
        if (out->pending) {
            out->flags |= 3;
        } else {
            out->flags = 3;
            out->pending = 1;
        }
    }
}
