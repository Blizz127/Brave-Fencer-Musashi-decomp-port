/* Owner re-derivation from retail assembly (clean input set). */
/* Updates the node at +0x20 (+0x34) via func_8004914C/func_800491AC, then calls func_8004945C into a local buffer. */
void func_8004914C(int p);
void func_800491AC(int p);
void func_8004945C(int a, int b, int *out);
void func_8012F214(char *o, int a1, int a2) {
    int v[2];
    int node = *(int *)(o + 0x20) + 0x34;
    func_8004914C(node);
    func_800491AC(node);
    func_8004945C(a1, a2, v);
}
