/* SPDX-License-Identifier: MIT */
typedef struct {
    unsigned char header[2];
    short phase;
} Hopper;

extern int func_8012BCCC(Hopper *hopper);

void func_80183BC8(Hopper *hopper)
{
    /* Wait until the target is close enough. An unavailable target gives
     * the largest signed distance and leaves the waiting phase in place. */
    if (func_8012BCCC(hopper) <= 0x18000)
        hopper->phase = 5;
}
