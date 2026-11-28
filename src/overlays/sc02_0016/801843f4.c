/* SC02 member 16 selects an object handler with the object's second halfword. */
struct SceneObject {
    unsigned short field_00;
    unsigned short action;
};

typedef void (*ActionHandler)(struct SceneObject *);

extern ActionHandler D_801B0A68[];

void func_801843F4(struct SceneObject *object)
{
    D_801B0A68[object->action](object);
}
