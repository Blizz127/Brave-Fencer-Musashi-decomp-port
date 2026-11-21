#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "psx_types.h"

s32 D_80078E8C;
s32 func_8018C914(s32 delta);
s32 func_801783D0(s32 balance, s32 shift);
u8 *musashi_wallet_hud_packets(u8 *packet, u32 guest_address, s32 packed, u16 x, s32 y);
void func_80177F84(u8 *packet, s32 packed);

static u32 word(const u8 *p)
{
    return p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void check_glyphs(const u8 *packet, unsigned amount)
{
    unsigned i;
    unsigned divisor = 10000;
    unsigned hide = 1;
    assert(packet[12] == 0);
    for (i = 1; i <= 5; ++i) {
        const u8 *cell = packet + 20 * i;
        unsigned digit = amount / divisor % 10u;
        if (digit != 0)
            hide = 0;
        assert(cell[12] == 8 + digit * 8);
        assert((cell[11] & 1u) == hide);
        divisor /= 10;
    }
    assert(packet[20 * 6 + 12] == 8 && !(packet[20 * 6 + 11] & 1u));
}

int main(void)
{
    static const unsigned values[] = { 0, 1, 7, 10, 92, 100, 903, 9999, 10000, 99999 };
    u8 *storage[2] = { malloc(160), malloc(160) };
    u8 *packets[2], before[140], unaligned[142];
    const u32 guest = 0x81000010u;
    unsigned i, j;
    assert(storage[0] && storage[1]);
    packets[0] = storage[0] + 1;
    packets[1] = storage[1] + 3;
    if ((((uintptr_t)packets[0] ^ (uintptr_t)packets[1]) & 0x00ffffffu) == 0)
        packets[1] += 2;
    D_80078E8C = 100;
    for (i = 0; i < 2; ++i) {
        memset(storage[i], 0x5a, 160);
        assert(musashi_wallet_hud_packets(packets[i], guest, func_801783D0(D_80078E8C, 0),
                                          57, 81) == packets[i] + 140);
        assert(packets[i][-1] == 0x5a && packets[i][140] == 0x5a);
    }
    assert(!memcmp(packets[0], packets[1], 140));
    for (i = 0; i < 7; ++i) {
        u8 *p = packets[0] + 20 * i;
        u32 expected_link = (guest + 20u * i - 20u) & 0x00ffffffu;
        assert(word(p) == (0x03000000u | expected_link));
        assert(word(p + 4) == 0x74808080u);
        assert(p[8] == 57 + 8 * i && p[10] == 81);
        assert(word(p + 16) == 0x5a5a5a5au);
    }
    check_glyphs(packets[0], 100);
    assert(musashi_wallet_hud_packets(packets[1], guest + 1u, 0, 0, 0) == NULL);
    assert(musashi_wallet_hud_packets(packets[1], 0xffffffc0u, 0, 0, 0) == NULL);
    assert(musashi_wallet_hud_packets(NULL, guest, 0, 0, 0) == NULL);
    assert(!memcmp(packets[0], packets[1], 140));

    /* Debit the wallet and follow the displayed balance down one unit per tick. */
    assert(func_8018C914(-8) == 0 && D_80078E8C == 92);
    for (s32 shown = 99; shown >= D_80078E8C; --shown) {
        func_80177F84(packets[0], func_801783D0(shown, 0));
        check_glyphs(packets[0], (unsigned)shown);
    }
    assert(func_8018C914(-93) == 1 && D_80078E8C == 92);
    memcpy(before, packets[0], sizeof before);
    func_80177F84(packets[0], func_801783D0(D_80078E8C, 0));
    assert(!memcmp(before, packets[0], sizeof before));

    /* The updater accepts the byte-oriented backing that the GPU consumes. */
    unaligned[0] = unaligned[141] = 0x79;
    memcpy(unaligned + 1, packets[0], sizeof before);
    for (i = 0; i < sizeof values / sizeof values[0]; ++i) {
        memcpy(before, unaligned + 1, sizeof before);
        func_80177F84(unaligned + 1, func_801783D0((s32)values[i], 0));
        check_glyphs(unaligned + 1, values[i]);
        for (j = 0; j < sizeof before; ++j) {
            if (j >= 20 && j < 120 && (j % 20 == 11 || j % 20 == 12))
                continue;
            assert(before[j] == unaligned[1 + j]);
        }
    }
    assert(unaligned[0] == 0x79 && unaligned[141] == 0x79);
    free(storage[0]);
    free(storage[1]);
    return 0;
}
