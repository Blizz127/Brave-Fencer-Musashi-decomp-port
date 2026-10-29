/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards its two args to func_801485B8 with the pointer at offset 0x20 of D_80126B78's target, minus 0x10. */
extern char *D_80126B78;
void func_801485B8(int base, int a, int b);
void func_801484B0(int a, int b) {
    func_801485B8(*(int *)(D_80126B78 + 0x20) - 0x10, a, b);
}
