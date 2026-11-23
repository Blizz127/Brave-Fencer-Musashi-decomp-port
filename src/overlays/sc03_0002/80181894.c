struct ForesActor;

extern int func_80128C98(int request);
extern void func_801CBB54(struct ForesActor *actor);

/* The first cottage conversation waits for interaction request 3. */
void func_80181894(struct ForesActor *actor)
{
    if (func_80128C98(3))
        func_801CBB54(actor);
}
