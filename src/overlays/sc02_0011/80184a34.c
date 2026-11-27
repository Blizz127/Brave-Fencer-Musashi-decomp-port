/* Let the summit script advance once the audio service is ready. */
extern int func_800D0CE0(void);

int func_80184A34(void)
{
    return func_800D0CE0() != 0;
}
