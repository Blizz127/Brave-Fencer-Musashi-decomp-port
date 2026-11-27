/* Preserve the previous camera state before applying the new view. */
extern void func_8012A598(void);

void func_8012A568(void (*update)(void))
{
    func_8012A598();
    update();
}
