/* Owner re-derivation from retail assembly (clean input set). */
/* Returns bit 3 of the flags word at offset 0x1F4 of the object. */
int func_8017267C(char *obj) {
    return *(int *)(obj + 0x1F4) & 8;
}
