#ifndef BFM_STEAM_KNIGHT_PARTICLE_H
#define BFM_STEAM_KNIGHT_PARTICLE_H
#include "steam_knight_hit.h"

typedef struct { u8 bytes[8]; } SteamKnightParticlePosition;
typedef struct { u32 words[8]; } SteamKnightParticleTransform;
typedef struct { s32 vector[3]; u32 selector; } SteamKnightParticleLaunch;
void func_8018B16C(SteamKnightPart *part, int count, SteamKnightParticlePosition *offset);
void func_80187664(SteamKnightPart *part);

#endif
