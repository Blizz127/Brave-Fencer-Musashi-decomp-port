extern int func_80029504(void);
extern int (*D_800D375C)(int story);

int func_800D05E8(int room)
{
    int track = 0;
    int story = func_80029504();

    (void)room;
    if (D_800D375C) {
        track = D_800D375C(story);
    }
    return track;
}
