extern int func_800291DC(int offset);
extern int func_800D0D7C(int hours, int minutes);
extern void func_800291C8(int offset, short value);

/* The Steamwood time limit keeps its first active start stamp. */
void func_80162120(void)
{
    if ((short)func_800291DC(8) == 0) {
        func_800291C8(8, (short)(func_800D0D7C(0, 1) | -32768));
    }
}
