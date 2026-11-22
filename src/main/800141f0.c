/* Game startup calls the three graphics services before clearing its state. */
extern void func_80059234(int mode);
extern void func_800594CC(int mode);
extern void func_80059658(int mode);
extern void func_80016714(signed char *destination, int size);
extern void func_800142C8(void);
extern signed char D_800BA118[];

void func_800141F0(void) {
    func_80059234(0);
    func_800594CC(0);
    func_80059658(0);
    func_80016714(D_800BA118, 152);
    func_800142C8();
}
