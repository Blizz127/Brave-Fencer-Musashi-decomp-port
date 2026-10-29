/* Owner re-derivation from retail assembly (clean input set). */
/* If the target is absent or its +0xAA field is zero, runs the func_80135888 query on D_80126B58's pair of words and, if it hits, queues func_8012F568 with D_801152A8; returns whether it queued. */
struct Ctx {
    int pad0[8];
    int a;
    int pad1[5];
    int b;
};
extern struct Ctx D_80126B58;
extern char D_801152A8[];
int func_80135888(int a, int b, int c, int d);
void func_8012F568(int a, int b, int c, int d, int e, char *f);
int func_8012DF34(char *t, int a1, int a2) {
    struct Ctx *c = &D_80126B58;
    if (t != 0 && *(short *)(t + 0xAA) != 0) {
        return 0;
    }
    if (func_80135888(c->a, c->b, a1, a2) != 0) {
        func_8012F568(1, 1, 0, 10, a2, D_801152A8);
        return 1;
    }
    return 0;
}
