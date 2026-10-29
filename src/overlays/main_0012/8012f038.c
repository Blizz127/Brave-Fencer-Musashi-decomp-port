/* Owner re-derivation from retail assembly (clean input set). */
/* Subtracts the object's position (+0x14..+0x1C) from a short vector, transforms it with func_8004D504 and stores the result back as three shorts. */
void func_8004D504(char *o, int *in, int *out);
void func_8012F038(char *o, short *a, short *out) {
    int v[3];
    int w[3];
    v[0] = a[0] - *(int *)(o + 0x14);
    v[1] = a[1] - *(int *)(o + 0x18);
    v[2] = a[2] - *(int *)(o + 0x1C);
    func_8004D504(o, v, w);
    out[0] = w[0];
    out[1] = w[1];
    out[2] = w[2];
}
