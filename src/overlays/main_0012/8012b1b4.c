/* Owner re-derivation from retail assembly (clean input set). */
/* Fetches a 3-word vector via func_800484EC from the object at +0x20 (+0x34) and copies words 0 and 2 into this object at +0x10/+0x18. */
void func_800484EC(int base, int a1, int *out);
void func_8012B1B4(char *o, int a1) {
    int v[3];
    func_800484EC(*(int *)(o + 0x20) + 0x34, a1, v);
    *(int *)(o + 0x10) = v[0];
    *(int *)(o + 0x18) = v[2];
}
