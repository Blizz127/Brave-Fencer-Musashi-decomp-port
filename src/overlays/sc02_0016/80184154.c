/* Four faces of the object effect share a vertex strip and brightness. */
struct Vertex {
    short x, y, z, padding;
};

struct Color {
    unsigned char r, g, b, padding;
};

struct Face {
    struct Vertex vertices[4];
    struct Color colors[4];
    unsigned int flags;
};

struct Model {
    unsigned char reserved[0x2c];
    unsigned short flags;
};

struct Effect {
    unsigned char reserved_00[0x1c];
    int age;
    struct Model *model;
    unsigned char reserved_24[0x4c];
    short brightness;
    unsigned char reserved_72[0x5a];
    void *primary;
    void *secondary;
};

extern unsigned char D_801B0A1C[];
extern void func_80017758(struct Face *, void *);
extern void func_8012C218(struct Effect *);

static inline void draw_faces(void *transform, short brightness)
{
    struct Face face;
    unsigned char *point = D_801B0A1C;
    short i;

    face.vertices[1].x = face.vertices[1].y = face.vertices[1].z = 0;
    face.vertices[0].z = face.vertices[2].z = face.vertices[3].z = 0;
    face.colors[0].r = face.colors[0].g = face.colors[0].b = 0;
    face.colors[2].r = face.colors[2].g = face.colors[2].b = 0;
    face.colors[3].r = face.colors[3].g = face.colors[3].b = 0;
    face.flags = 0x50000000;
    face.colors[1].r = face.colors[1].b = face.colors[1].g = brightness;

    for (i = 0; i < 4; ++i) {
        face.vertices[0].x = (signed char)*point++;
        face.vertices[0].y = (signed char)*point++;
        face.vertices[2].x = (signed char)*point++;
        face.vertices[2].y = (signed char)*point++;
        face.vertices[3].x = (signed char)*point++;
        face.vertices[3].y = (signed char)*point++;
        /* The next face starts with the preceding face's last coordinate pair. */
        point -= 2;
        func_80017758(&face, transform);
    }
}

void func_80184154(struct Effect *effect)
{
    struct Model *model = effect->model;

    if (effect->age++ < 2) {
        draw_faces(effect->primary, effect->brightness);
        if (effect->secondary)
            draw_faces(effect->secondary, effect->brightness);
        effect->brightness >>= 1;
        model->flags |= 1;
    } else {
        func_8012C218(effect);
    }
}
