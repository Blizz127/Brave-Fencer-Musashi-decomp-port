#include "saved_room_actor.h"

/* Subtypes 1 and 2 have their own state handlers in the saved-room scene. */
typedef void (*SceneActorStep)(struct FileActor *);

extern SceneActorStep D_801BBCAC[];
extern SceneActorStep D_801BBCC0[];
extern SceneActorStep D_801BBCD4[];

void func_8017FF74(struct FileActor *actor)
{
    SceneActorStep step;

    switch (actor->subtype) {
    case 0:
    default:
        step = D_801BBCAC[actor->state];
        break;
    case 1:
        step = D_801BBCC0[actor->state];
        break;
    case 2:
        step = D_801BBCD4[actor->state];
        break;
    }
    step(actor);
}
