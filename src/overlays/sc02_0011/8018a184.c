/* Room score selection is independent of story progression. */
extern int D_8012704C;

int func_8018A184(int story_step)
{
    (void)story_step;
    if (!D_8012704C)
        return 0x18;
    return 0x2b;
}
