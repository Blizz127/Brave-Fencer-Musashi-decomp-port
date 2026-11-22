/* Advance one step of a disc read, including seek, transfer and recovery. */
typedef unsigned char Byte;
typedef unsigned int Word;
typedef int Signed;
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ Address;
#else
typedef unsigned long Address;
#endif

/* The game shares this command buffer across multiple states. These offsets
 * are relative to the associated word in the original stream-state block. */
#define BUFFER_AT(cell, displacement) ((Byte *)((Address)&(cell) + (displacement)))
#define SIGNED_WORD(cell) (*(Signed *)&(cell))
/* Increment the mapped word with the original 32-bit wraparound. */
#define ADVANCE_WORD(cell) (SIGNED_WORD(cell) = (Signed)((Word)SIGNED_WORD(cell) + 1u))

extern Word D_800AE6F0, D_800AE6F8, D_800AE6FC, D_800AE700;
extern Word D_800AE708, D_800AE70C, D_800AE71C, D_800AE724;
extern Word D_800AE728, D_800AE72C, D_800AE734, D_800AE748, D_800AE74C;
extern Word D_800AE750, D_800AE754, D_800AE7A4, D_800AE7AC;
extern Signed D_800AE6E8, D_800AE7A8, D_800AE7B0;
extern Byte D_800AE738[8], D_800AE740[8], D_800AE798, D_800AE799;

extern void func_800435CC(int command, const void *parameter, Byte *reply);
extern int func_8004355C(int mode, Byte *reply);
extern void func_800434BC(void);
extern Word func_800435B4(Word callback);
extern int func_80043B1C(const void *position);
extern int func_8002FD14(int id, int argument);
extern void func_8001A0FC(void);
extern int func_8001A114(void);
extern void func_8001A338(Byte event);

void func_80019AF8(int seek_only)
{
    int status, index;
    const int *track;
    switch (D_800AE6F0) {
    case 0:
        /* Configure the drive for this request. */
        SIGNED_WORD(D_800AE750) = 0;
        D_800AE71C = 0;
        D_800AE70C = 0;
        BUFFER_AT(D_800AE750, -16)[0] = 160;
        func_800435CC(14, BUFFER_AT(D_800AE750, -16), BUFFER_AT(D_800AE750, -24));
        if (BUFFER_AT(D_800AE750, -24)[0] & 16) {
recover:
            D_800AE6F0 = 9;
            break;
        }
        ++D_800AE6F0;
        /* fall through */
    case 1:
        status = func_8004355C(1, D_800AE738);
        D_800AE6E8 = status;
        if (status == 0) break;
        if (status == 5) goto restart;
        if (status != 2) break;
        D_800AE750 = 0;
        ++D_800AE6F0;
        /* fall through */
    case 2:
        /* Allow three polls for the mode change. */
        if (ADVANCE_WORD(D_800AE750) >= 3) {
            SIGNED_WORD(D_800AE750) = 0;
            ++D_800AE6F0;
        }
        break;
    case 3:
        /* Seek to the position stored by the request. */
        func_800435CC(21, (const void *)(Address)(Word)SIGNED_WORD(D_800AE7A4), BUFFER_AT(D_800AE7A4, -108));
        if (BUFFER_AT(D_800AE7A4, -108)[0] & 16) goto recover;
        D_800AE754 = 0;
        D_800AE7AC = func_80043B1C((const void *)(Address)(Word)SIGNED_WORD(D_800AE7A4));
        ++D_800AE6F0;
        /* fall through */
    case 4:
        /* Poll the seek result and decide whether to start reading. */
        status = func_8004355C(1, D_800AE738);
        D_800AE6E8 = status;
        if (status == 5) {
            ++D_800AE750;
seek_again:
            func_800434BC();
            D_800AE6F0 = 3;
            break;
        }
        if (!status) break;
        if (status != 2) {
            D_800AE7B0 = 0;
            if ((Signed)++D_800AE754 <= 180) return;
            goto seek_again;
        }
        if (seek_only) {
            func_800435CC(9, 0, D_800AE738);
            if (D_800AE738[0] & 16) goto recover;
            D_800AE6F0 = 8;
            D_800AE70C = *(Word *)(Address)D_800AE7A4;
            break;
        }
        ++D_800AE6F0;
        /* fall through */
    case 5:
        /* Install the sector callback and start transfer. */
        D_800AE74C = 0;
        if (D_800AE7A8 <= 0) {
            D_800AE6F8 = 0;
        } else {
            D_800AE6F8 = 3;
            D_800AE748 = (Signed)((Word)D_800AE7A8 + 3u) / 4;
            D_800AE798 = 1;
            D_800AE728 = D_800AE72C;
        }
        SIGNED_WORD(D_800AE734) = func_800435B4((Word)(Address)func_8001A338);
        func_800435CC(6, 0, BUFFER_AT(D_800AE734, 4));
        if (BUFFER_AT(D_800AE734, 4)[0] & 16) goto recover;
        D_800AE750 = 0;
        ++D_800AE6F0;
        /* fall through */
    case 6:
        /* Poll the callback status and restore the previous callback. */
        if (SIGNED_WORD(D_800AE74C) == 1) {
            func_800435B4(D_800AE734);
            func_800435CC(9, 0, BUFFER_AT(D_800AE74C, -20));
            if (BUFFER_AT(D_800AE74C, -20)[0] & 16) goto recover;
            ++D_800AE6F0;
            if (!D_800AE71C) ++D_800AE6F0;
        } else if (SIGNED_WORD(D_800AE74C) == 2) {
            func_800435B4(D_800AE734);
            goto restart;
        } else if ((Signed)++D_800AE750 >= 300) {
            func_800435B4(D_800AE734);
            D_800AE6F0 = 0;
            func_800434BC();
        }
        break;
    case 7:
        /* Submit any track requests collected during the transfer. */
        for (index = 0; index < (Signed)D_800AE71C; ++index) {
            do {
                track = (const int *)(Address)((Word)index * 8u + D_800AE724);
            } while (!func_8002FD14(track[0], track[1]));
        }
        ADVANCE_WORD(D_800AE6F0);
        break;
    case 8:
        /* A successful final poll releases the outstanding request. */
        status = func_8004355C(1, D_800AE738);
        D_800AE6E8 = status;
        if (status == 2) {
            D_800AE728 = 0x80010000u;
            D_800AE6F0 = 0;
            D_800AE6F8 = 0;
            D_800AE6FC = 0;
            D_800AE700 = 0;
            D_800AE748 = 1;
            D_800AE74C = 0;
            D_800AE798 = 0;
            D_800AE799 = 0;
            D_800AE70C = 0;
            D_800AE7B0 = 1;
            D_800AE708 = 0;
            return;
        }
        if (status == 5) {
            func_800434BC();
            func_800435CC(9, 0, D_800AE738);
            if (!(D_800AE738[0] & 16)) {
                break;
            }
            goto recover;
        }
        break;
    case 9:
        /* Start disc recovery. */
        func_8001A0FC();
        ADVANCE_WORD(D_800AE6F0);
        /* fall through */
    case 10:
        /* Poll disc recovery. */
        D_800AE6E8 = func_8001A114();
        if (D_800AE6E8) {
restart:
            D_800AE6F0 = 0;
        }
        break;
    }
    D_800AE7B0 = 0;
}
