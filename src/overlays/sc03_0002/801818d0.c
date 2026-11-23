struct ForesActor;

extern int func_80128C98(int request);
extern void func_801CBBDC(struct ForesActor *actor);

/* The later cottage conversation has its own start callback. */
void func_801818D0(struct ForesActor *actor)
{
    if (func_80128C98(3))
        func_801CBBDC(actor);
}
