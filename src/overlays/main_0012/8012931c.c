/* Owner re-derivation from retail assembly (clean input set). */
/* Adds the velocity words at +0x10..+0x18 to the position words at +4..+0xC. */
void func_8012931C(int *p) {
    p[1] += p[4];
    p[2] += p[5];
    p[3] += p[6];
}
