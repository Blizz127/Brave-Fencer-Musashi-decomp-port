/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update: when the counter at +2 is zero run func_80143458, otherwise integrate motion and test collision. */
void func_80143458(char *o);
void func_8012931C(char *o);
int func_80128ED8(int a, char *b);
void func_801292C8(char *o);
void func_801433F0(char *o) {
    if (*(unsigned short *)(o + 2) == 0) {
        func_80143458(o);
    } else {
        func_8012931C(o);
        if (func_80128ED8(*(int *)(o + 0x20), o + 0x24) != 0) {
            func_801292C8(o);
        }
    }
}
