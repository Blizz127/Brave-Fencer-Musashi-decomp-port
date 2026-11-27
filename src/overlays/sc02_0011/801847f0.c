/* Restore the field camera after the race. */
extern void func_8012A568(void (*update)(void));
extern void func_801823EC(void);

void func_801847F0(void)
{
    func_8012A568(func_801823EC);
}
