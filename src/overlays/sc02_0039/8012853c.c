/* Field stage four calls the room hook, then advances the resident substate. */
extern void func_80146074(void);
extern void func_80011C10(void);

void func_8012853C(void)
{
    func_80146074();
    func_80011C10();
}
