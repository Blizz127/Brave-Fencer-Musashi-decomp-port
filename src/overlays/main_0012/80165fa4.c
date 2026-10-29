/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a transform in a local buffer, applies func_80020F34 with the object's +0x20 (+0x18) data, then runs func_80048FBC on two inputs. */
void func_80013F3C(int *m);
void func_80020F34(int *m, int p);
void func_80048FBC(int *m, int a, int b);
void func_80165FA4(char *o, int a1, int a2) {
    int m[8];
    func_80013F3C(m);
    func_80020F34(m, *(int *)(o + 0x20) + 0x18);
    func_80048FBC(m, a1, a1);
    func_80048FBC(m, a2, a2);
}
