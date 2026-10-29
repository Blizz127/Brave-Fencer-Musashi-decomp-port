/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the 12-bit angle from object b to object a, rotated by -0x400. */
int func_8004CFEC(int dx, int dy);
int func_8012B6D4(short *a, short *b) {
    int r = func_8004CFEC(a[5] - b[5], b[1] - a[1]);
    return (r - 0x400) & 0xFFF;
}
