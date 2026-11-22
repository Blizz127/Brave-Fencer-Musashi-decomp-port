/* Submit the next block in the game sound stream. */
extern unsigned char D_8006AEF4;
extern int D_800A46C8;

extern int func_8003C4F0(int mode);
extern int func_8003C498(unsigned int address);
extern unsigned int func_8003C438(void *data, unsigned int bytes);
extern void func_80037D74(void);

int func_8002FC64(int bytes, void *data)
{
    int *destination;

    if ((D_8006AEF4 & 1) && !func_8003C4F0(0)) goto stop;
    destination = &D_800A46C8;
    if (!func_8003C498((unsigned int)*destination)) {
stop:
        func_80037D74();
        return 0;
    }

    func_8003C438(data, (unsigned int)bytes);
    *destination = (int)((unsigned int)*destination + (unsigned int)bytes);
    D_8006AEF4 |= 1;
    return 1;
}
