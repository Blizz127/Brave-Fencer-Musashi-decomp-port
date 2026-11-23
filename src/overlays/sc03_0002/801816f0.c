struct ForesPart {
    unsigned char padding[12];
    short limit;
};
struct ForesActor {
    unsigned short kind;
    unsigned short phase;
    unsigned char reserved04[0x68 - 0x04];
    struct ForesPart *part;
    unsigned char reserved6c[0xd4 - 0x6c];
    struct ForesActor *first_child;
    struct ForesActor *second_child;
};
extern const unsigned char D_80189938[], D_801AB098[], D_80189930[], D_801A4304[], D_80189928[];
extern int func_80182D68(struct ForesActor *, const unsigned char *);
extern int func_80029504(void);
extern void func_8012E88C(struct ForesActor *);
extern void func_8012A828(struct ForesActor *, const unsigned char *);
extern void func_8018323C(struct ForesActor *);
extern void func_8012E8E0(struct ForesActor *, const unsigned char *);
extern void func_80183480(struct ForesActor *, const unsigned char *, int, int);
extern void func_80183264(struct ForesActor *, const unsigned char *);
struct ForesInteraction;
typedef void (*ForesFieldCallback)(void);
typedef ForesFieldCallback (*ForesInteractionCallback)(struct ForesInteraction *);
extern ForesFieldCallback func_80181990(struct ForesInteraction *);
extern ForesFieldCallback func_801813E4(struct ForesInteraction *);
extern struct ForesActor *func_801788B8(struct ForesActor *, ForesInteractionCallback);

void func_801816F0(struct ForesActor *actor)
{
    int progress;

    if (!func_80182D68(actor, D_80189938))
        return;
    progress = func_80029504();
    if (progress < 100)
        goto inactive;
    if (progress < 120)
        actor->phase = 7;
    else if (progress < 130)
        actor->phase = 11;
    else
        goto inactive;
    func_8012E88C(actor);
    func_8012A828(actor, D_801AB098);
    func_8018323C(actor);
    func_8012E8E0(actor, D_80189930);
    func_80183480(actor, D_801A4304, 1, 112);
    goto attach;
inactive:
    actor->phase = 2;
    func_80183264(actor, D_80189928);
attach:
    actor->part->limit = 32767;
    actor->first_child = func_801788B8(actor, func_80181990);
    func_8012E8E0(actor->first_child, D_80189928);
    actor->second_child = func_801788B8(actor, func_801813E4);
}
