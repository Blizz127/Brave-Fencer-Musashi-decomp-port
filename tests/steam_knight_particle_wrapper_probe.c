#include <assert.h>
#include <string.h>

#include "steam_knight_particle.h"

SteamKnightParticlePosition D_801E2EF0;
static SteamKnightPart *expected_part;
static unsigned calls;

void func_8018B16C(SteamKnightPart *part, int count, SteamKnightParticlePosition *position)
{
    const SteamKnightParticlePosition before = D_801E2EF0;
    assert(part == expected_part && count == 6 && position != &D_801E2EF0);
    memset(&D_801E2EF0, 0xb1, sizeof D_801E2EF0);
    assert(memcmp(position, &before, sizeof before) == 0);
    memset(position, 0x7d, sizeof *position);
    for (unsigned i = 0; i < sizeof position->bytes; ++i)
        assert(D_801E2EF0.bytes[i] == 0xb1);
    ++calls;
}

int main(void)
{
    SteamKnightPart part = {0};
    for (unsigned i = 0; i < 2; ++i) {
        for (unsigned j = 0; j < sizeof D_801E2EF0.bytes; ++j)
            D_801E2EF0.bytes[j] = (u8)(17 + 5*i + 11*j);
        expected_part = i ? &part : NULL;
        func_80187664(expected_part);
    }
    assert(calls == 2);
    return 0;
}
