/* Initialize the disc directory and expand its per-file offsets into game records. */
typedef struct {
    char name[20];
    int position;
    int length;
    char rest[20];
} DiscFile;
typedef struct {
    int offset;
    int length;
} DirectoryRecord;

typedef struct {
    int position;
    int length;
} FileLocation;

#define RESOURCE_DIRECTORY ((DirectoryRecord *)0x80180000u)

extern DiscFile D_80062C24[], D_80063044;
extern FileLocation D_800AE830[];
extern int D_800747D0, D_800747D4, D_80063074, D_800747D8;
extern void func_80019990(void);
extern int func_80045374(int *position, char *name);
extern int func_80019A24(int *file, int buffer, int size, int flags);
extern int func_8004239C(int mode);
extern int func_80043B1C(int *position);
extern void func_80043A18(unsigned int sector, int *position);

void func_8001971C(void)
{
    /* The original caller reserves these eight additional stack bytes. */
    unsigned char frame[8];
    int group, status, record, index, directory, n, base;
    int count;

    (void)frame;
    func_80019990();
    for (group = 0; group < 16; group++) {
        status = func_80045374(&D_80063044.position, D_80063044.name);
        if (status != -1) {
            break;
        }
    }

    D_800747D0 = status != 0;
    D_800747D4 = 1;
    D_80063074 = 0;
    D_800747D8 = 0;
    for (group = 0; (unsigned)group < 21; ++group) {
        do {
            status = func_80045374(&D_80062C24[group].position, D_80062C24[group].name);
        } while (status == -1 || status == 0);
    }
    while (!func_80019A24(&D_80062C24[0].position, (int)0x80180000u, 3648, 0)) {
        func_8004239C(0);
    }

    record = 0;
    directory = 0;
    for (group = 0; group < 8; ++group) {
        base = func_80043B1C(&D_80062C24[group + 1].position);
        count = RESOURCE_DIRECTORY[directory].offset;
        ++directory;
        for (n = 0; n < count; ++n) {
            index = RESOURCE_DIRECTORY[directory].offset;
            if (index != 0) {
                func_80043A18((unsigned int)index + (unsigned int)base, &D_800AE830[record].position);
                D_800AE830[record].length = RESOURCE_DIRECTORY[directory].length;
            }
            ++directory;
            ++record;
        }
    }
}
