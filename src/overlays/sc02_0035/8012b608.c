/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the signed 12-bit angle difference from a to b divided by the step c, never 0 unless the angles are equal (clamped to +/-1). */
int func_8012B608(int a, int b, int c) {
    int d;
    int t;
    a &= 0xFFF;
    b &= 0xFFF;
    if (a == b) {
        return 0;
    }
    d = b - a;
    if (d > 0x800) {
        t = a + 0x1000;
        d = b - t;
    }
    if (d < -0x800) {
        t = a - 0x1000;
        d = b - t;
    }
    if (d >= 0) {
        d = d / c;
        if (d == 0) {
            d = 1;
        }
        return d;
    }
    d = d / c;
    if (d == 0) {
        d = -1;
    }
    return d;
}
