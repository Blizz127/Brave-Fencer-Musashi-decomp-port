/* Install the scene callback during the bedroom script's startup. */
extern void func_80174384(void *scene);
extern void func_80174684(void (*callback)(void *scene));

void func_8017435C(void)
{
    func_80174684(func_80174384);
}
