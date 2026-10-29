/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update: when the counter at +2 is zero run func_80143458, otherwise integrate motion and test collision. */
extern int func_80143458(char *o);
void func_8012931C(char *o);
int func_80128ED8(int a, char *b);
extern int func_801292C8(char *o);
/* Returns retail's leftover v0: func_80143458's result, else func_80128ED8's result (0) or func_801292C8's result. */
int func_801433F0(char *o) {
    int r;
    if (*(unsigned short *)(o + 2) == 0) {
        r = func_80143458(o);
    } else {
        func_8012931C(o);
        r = func_80128ED8(*(int *)(o + 0x20), o + 0x24);
        if (r != 0) {
            r = func_801292C8(o);
        }
    }
    return r;
}
