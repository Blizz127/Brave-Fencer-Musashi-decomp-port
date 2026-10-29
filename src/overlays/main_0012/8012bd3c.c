/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 0 when the squared distance limit a2 is below the measured distance to D_80126CB4 (or unlimited when D_80126CBA is set); otherwise returns func_8012BDBC(o, a1). */
extern short D_80126CBA;
extern char D_80126CB4[];
int func_80013478(char *a, char *b);
int func_8012BDBC(char *o, int a);
int func_8012BD3C(char *o, int a1, int a2) {
    int d;
    int lim = a2;
    int idx = a1;
    if (D_80126CBA != 0) {
        d = 0x7FFFFFFF;
    } else {
        d = func_80013478(o + 4, D_80126CB4);
    }
    if (d <= lim) {
        return func_8012BDBC(o, (short)idx);
    }
    return 0;
}
