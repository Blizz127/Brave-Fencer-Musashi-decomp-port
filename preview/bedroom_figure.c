/* SPDX-License-Identifier: MIT */
#include "bedroom_visitor.h"
#include "forest_raster.h"
#include "forest_tether_trails.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int r[3][3], t[3];
} Transform;

typedef struct {
    BfmForestRasterVertex v[4];
    BfmForestRasterMaterial material;
    int depth;
    size_t order;
} FigureFace;

typedef struct {
    int left, top;
    unsigned width, height;
} ShadowRect;

struct BfmBedroomFigure {
    MusashiForestPlayerMesh *mesh;
    int direction[3][3], color[3][3], ambient[3];
    unsigned face_sources[8][4];
    ShadowRect shadow;
    FigureFace *faces;
    size_t count, next, capacity;
    unsigned visitors_added;
    int x_min, y_min, x_max, y_max;
    BfmForestRaster surface;
};

static unsigned read16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static int floor_q12(int64_t value)
{
    return (int)(value / 4096 - (value % 4096 < 0));
}

/* The 31-piece model uses common-root local transforms. Its fixed-point
 * quarter waves and between-term rounding match the public forest pose
 * reader. The leading-eight-bit length reduction also covers sleeping poses. */
static void angle(unsigned units, int *s, int *c)
{
    const double radians = 6.28318530717958647693 / 4096;
    unsigned quadrant = (units / 1024) & 3, phase = units & 1023;
    int direct = (int)lround(4096 * sin(phase * radians));
    int mirror = (int)lround(4096 * sin((1023-phase) * radians));
    *s = quadrant & 1 ? mirror : direct;
    *c = quadrant & 1 ? direct : mirror;
    if (quadrant >= 2) *s = -*s;
    if (quadrant == 1 || quadrant == 2) *c = -*c;
}

static void rotation(int sx, int cx, int sy, int cy, int sz, int cz, int r[3][3])
{
    int xz = floor_q12(cx * sz), zz = floor_q12(cx * cz), xy = floor_q12(sx * sy);
    r[0][0] = floor_q12(cz * cy);
    r[1][0] = floor_q12(sz * cy);
    r[2][0] = -sy;
    r[0][1] = floor_q12(xy * cz) - xz;
    r[1][1] = floor_q12(xy * sz) + zz;
    r[2][1] = floor_q12(cy * sx);
    r[0][2] = floor_q12(zz * sy) + floor_q12(sx * sz);
    r[1][2] = floor_q12(xz * sy) - floor_q12(sx * cz);
    r[2][2] = floor_q12(cy * cx);
}

static uint64_t quantize_length(uint64_t squared)
{
    unsigned shift = 0;
    while ((squared >> shift) > 255) shift += 2;
    return (squared >> shift) << shift;
}

static Transform local_pose(const uint8_t *payload, unsigned i)
{
    const uint8_t *record = payload + (i ? (i-1)*8 : 0);
    const uint8_t *xyz = i ? record + 3 : record;
    const uint8_t *a = i < 2 ? payload + 24 : record;
    int sx, cx, sy, cy, sz, cz;
    Transform tr;
    angle(read16(a+6), &sx, &cx);
    angle(((a[0] & 15u) << 8) | a[1], &sy, &cy);
    angle(((a[0] & 240u) << 4) | a[2], &sz, &cz);
    rotation(sx,cx,sy,cy,sz,cz,tr.r);
    for (unsigned k = 0; k < 3; ++k) {
        tr.t[k] = (int8_t)xyz[k];
        if (i) continue;
        uint64_t squared = 0;
        for (unsigned j = 0; j < 3; ++j)
            squared += (unsigned)(tr.r[j][k] * tr.r[j][k]);
        uint64_t length = quantize_length(squared);
        if (length) {
            int gain = (int)sqrt(281474976710656.0 / (double)length);
            for (unsigned j = 0; j < 3; ++j)
                tr.r[j][k] = floor_q12((k == 1 ? 1 : -1) * tr.r[j][k] * gain);
        }
    }
    return tr;
}

static Transform compose(const Transform *a, const Transform *b)
{
    Transform out;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t delta = 0;
        for (unsigned j = 0; j < 3; ++j) {
            int64_t value = 0;
            for (unsigned k = 0; k < 3; ++k)
                value += (int64_t)a->r[i][k] * b->r[k][j];
            out.r[i][j] = floor_q12(value);
            delta += (int64_t)a->r[i][j] * b->t[j];
        }
        out.t[i] = a->t[i] + floor_q12(delta);
    }
    return out;
}

static Transform support_rotation(const int normal[3])
{
    const double radians = 6.28318530717958647693 / 4096;
    double roll = lround(atan2(normal[0], -normal[1])/radians) * radians;
    double incline = lround(atan2(-normal[2], -normal[1])/radians) * radians;
    Transform bank = {0}, rise = {0};
    rotation(0,4096,0,4096,(int)lround(4096*sin(roll)),(int)lround(4096*cos(roll)),bank.r);
    rotation((int)lround(4096*sin(incline)),(int)lround(4096*cos(incline)),0,4096,0,4096,rise.r);
    return compose(&rise,&bank);
}

int bfm_bedroom_figure_lights(BfmBedroomFigure *figure, const BfmBedroomLight lights[3])
{
    if (!figure || !lights) return 0;
    int length[3];
    for (unsigned i = 0; i < 3; ++i) {
        uint64_t squared = 0;
        for (unsigned k = 0; k < 3; ++k) {
            int axis = lights[i].axis[k];
            squared += (unsigned)(axis * axis);
        }
        length[i] = (int)sqrt((double)quantize_length(squared));
        if (!length[i]) return 0;
    }
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            figure->direction[i][k] = -lights[i].axis[k]*4096/length[i];
            figure->color[k][i] = lights[i].rgb[k]*4096/255;
        }
    }
    return 1;
}

static int light_matrix(BfmBedroomFigure *figure, const uint8_t *room)
{
    BfmBedroomLight lights[3];
    for (unsigned i = 0; i < 3; ++i) {
        const uint8_t *source = room + 6 + 10*i;
        for (unsigned k = 0; k < 3; ++k)
            lights[i].axis[k] = (int16_t)read16(source+2*k);
        memcpy(lights[i].rgb,source+6,3);
        int ambient = (int16_t)read16(room+2*i);
        figure->ambient[i] = ambient - ((ambient % 16 + 16) % 16);
    }
    return bfm_bedroom_figure_lights(figure,lights);
}

static int read_shadow(ShadowRect *rect, const uint8_t *source)
{
    rect->width = source[2];
    rect->height = source[3];
    rect->left = (int16_t)read16(source+8);
    rect->top = (int16_t)read16(source+10);
    return rect->width && rect->width <= 192 && rect->height && rect->height <= 96 &&
           rect->left >= -256 && rect->left <= 256 && rect->top >= -256 && rect->top <= 256;
}

BfmBedroomFigure *bfm_bedroom_figure_load(const MusashiDiscMedia *disc,
                     const uint8_t *scene, size_t size, uint32_t room_address,
                     uint32_t face_address, uint32_t shadow_address)
{
    const uint32_t origin = 0x80128158;
    if (face_address < origin || shadow_address < origin) return NULL;
    size_t face_table = face_address-origin, shadow_offset = shadow_address-origin;
    if (!disc || !scene || room_address < origin || size < 36 ||
        room_address-origin > size-36 || size < face_table+64 || shadow_offset > size || size-shadow_offset < 12)
        return NULL;
    BfmBedroomFigure *figure = calloc(1, sizeof(*figure));
    if (!figure) return NULL;
    if (!light_matrix(figure, scene + (room_address-origin))) goto fail;
    for (unsigned i = 0; i < 8; ++i) {
        unsigned *rect = figure->face_sources[i];
        for (unsigned j = 0; j < 4; ++j)
            rect[j] = read16(scene + face_table + 8*i + 2*j);
        if (!rect[2] || rect[2] > 512 || !rect[3] || rect[3] > 256 ||
            rect[0] > 1024-rect[2] || rect[1] > 512-rect[3] ||
            rect[0]+rect[2] > 512)
            goto fail;
    }
    const uint8_t *shadow = scene + shadow_offset;
    if (!read_shadow(&figure->shadow,shadow)) goto fail;
    figure->mesh = musashi_forest_player_mesh_load(disc);
    if (!figure->mesh) goto fail;
    size_t capacity = 1 + 8 + 10; /* Ground shadow, Fusion halo, and Lumina halo. */
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(figure->mesh,i);
        if (part->face_count > SIZE_MAX / sizeof(FigureFace) - capacity) goto fail;
        capacity += part->face_count;
    }
    figure->faces = calloc(capacity, sizeof(*figure->faces));
    figure->capacity = capacity;
    if (figure->faces) return figure;
fail:
    bfm_bedroom_figure_free(figure);
    return NULL;
}

void bfm_bedroom_figure_free(BfmBedroomFigure *figure)
{
    if (!figure) return;
    musashi_forest_player_mesh_free(figure->mesh);
    free(figure->faces);
    free(figure);
}

static int clamp_light(int n)
{
    return n < 0 ? 0 : n > 32767 ? 32767 : n;
}

static uint32_t shade(const BfmBedroomFigure *figure, const int light[3][3],
                      const MusashiForestPlayerVector *normal, const uint8_t rgb[3])
{
    int scale[3];
    for (unsigned i = 0; i < 3; ++i)
        scale[i] = clamp_light(floor_q12((int64_t)light[i][0]*normal->x +
                          (int64_t)light[i][1]*normal->y + (int64_t)light[i][2]*normal->z));
    uint32_t out = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k) sum += (int64_t)figure->color[i][k]*scale[k];
        int level = clamp_light(figure->ambient[i] + floor_q12(sum));
        unsigned c = (unsigned)rgb[i]*level / 4096;
        out |= (c > 255 ? 255u : c) << (8*i);
    }
    return out;
}

static void include_pixel(BfmBedroomFigure *figure, int x, int y)
{
    if (x < figure->x_min) figure->x_min = x;
    if (x > figure->x_max) figure->x_max = x;
    if (y < figure->y_min) figure->y_min = y;
    if (y > figure->y_max) figure->y_max = y;
}

static void collect_part(BfmBedroomFigure *figure, const BfmBedroomView *camera,
                         const Transform *world, const MusashiForestPlayerPart *part,
                         int near_priority)
{
    int light[3][3];
    for (unsigned a = 0; a < 3; ++a)
        for (unsigned b = 0; b < 3; ++b) {
            int64_t dot = 0;
            for (unsigned k = 0; k < 3; ++k)
                dot += (int64_t)figure->direction[a][k] * world->r[k][b];
            light[a][b] = floor_q12(dot);
        }
    for (size_t f = 0; f < part->face_count; ++f) {
        const MusashiForestPlayerFace *face = &part->faces[f];
        FigureFace *out = &figure->faces[figure->count];
        int near = INT_MAX, sum = 0;
        unsigned corner = 0;
        for (; corner < face->corners; ++corner) {
            const MusashiForestPlayerVector *point = &part->vertices[face->vertex[corner]];
            int16_t xyz[3] = {point->x, point->y, point->z};
            int screen[3];
            if (!bfm_bedroom_project(camera, xyz, screen)) break;
            if (screen[2] <= camera->focal/2 &&
                (corner < 3 || near_priority || (face->mode != 0x38 && face->mode != 0x3c)))
                break;
            if (screen[2] < near) near = screen[2];
            sum += screen[2];
            out->v[corner] = (BfmForestRasterVertex){.x=screen[0],.y=screen[1],
                                                    .u=face->u[corner],.v=face->v[corner]};
        }
        if (corner != face->corners) continue;
        const BfmForestRasterVertex *p = out->v;
        if ((p[1].x-p[0].x)*(p[2].y-p[0].y) <= (p[1].y-p[0].y)*(p[2].x-p[0].x))
            continue;
        uint32_t rgb = 0;
        for (unsigned k = 0; k < face->corners; ++k) {
            if (!k || (face->mode & 16)) {
                const uint8_t *ink = face->corner_rgb[k];
                /* Unlit mounted effects carry their own vertex colors. */
                if (face->mode & 1)
                    rgb = ink[0] | ((uint32_t)ink[1] << 8) | ((uint32_t)ink[2] << 16);
                else
                    rgb = shade(figure, light, &part->normals[face->normal[k]], ink);
            }
            out->v[k].rgb = rgb;
            include_pixel(figure,out->v[k].x,out->v[k].y);
        }
        out->material = (BfmForestRasterMaterial){
            .page=face->tpage, .clut=face->clut, .count=face->corners,
            .textured=!!(face->mode & 4), .translucent=!!(face->mode & 2)
        };
        out->depth = near_priority ? near/4-1 : (sum*(face->corners == 3 ? 341 : 256))/4096;
        out->order = figure->count++;
    }
}

/* The shadow caster projects the player's contact, independently of the animated
 * body. Separation from the mounting plane shrinks the footprint during a fall. */
static void collect_shadow(BfmBedroomFigure *figure, const BfmBedroomView *view,
                           const Transform *eye, const BfmBedroomBody *body, const ShadowRect *rect,
                           int size, uint32_t rgb)
{
    const double turn = 6.28318530717958647693 / 4096;
    double yaw = -((int)((unsigned)body->heading & 4095)) * turn;
    Transform bearing = {0};
    rotation(0,4096,(int)lround(4096*sin(yaw)),(int)lround(4096*cos(yaw)),0,4096,bearing.r);
    Transform slope = support_rotation(body->shadow_normal);
    Transform ground = compose(&slope,&bearing);
    memcpy(ground.t,body->shadow_position,sizeof(ground.t));
    int separation = body->shadow_position[1] - body->position[1];
    int scale = (768-separation)*size/768;
    if (scale <= 0 || scale > INT16_MAX) return;
    Transform posed = compose(eye,&ground);
    BfmBedroomView camera = {.focal=view->focal};
    memcpy(camera.axes,posed.r,sizeof(camera.axes));
    memcpy(camera.translation,posed.t,sizeof(camera.translation));
    FigureFace *face = &figure->faces[figure->count];
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = (i >> 1) & 1;
        int16_t point[3] = {
            (int16_t)((rect->left + right*(int)rect->width)*scale/4096),0,
            (int16_t)((rect->top + bottom*(int)rect->height)*scale/4096)
        };
        int screen[3];
        if (!bfm_bedroom_project(&camera,point,screen)) return;
        include_pixel(figure,screen[0],screen[1]);
        face->v[i] = (BfmForestRasterVertex){.x=screen[0],.y=screen[1],
            .u=(uint8_t)(64 + right*(rect->width-1)),
            .v=(uint8_t)(160 + bottom*(rect->height-1)),.rgb=rgb};
    }
    face->material = (BfmForestRasterMaterial){.page=0x59,.clut=0x4516,
                                               .textured=1,.translucent=1,.count=4};
    face->depth = posed.t[2]/4+2;
    face->order = figure->count++;
}

static void lumina_quad(const BfmBedroomBody *body, unsigned face, int16_t xyz[4][3], uint32_t rgb[4])
{
    int guard=face<4, seam=face>=8;
    unsigned start=guard ? 2*face : seam ? 10+2*(face-8) : (face<6 ? 2*(face-4) : 5+2*(face-6));
    const int8_t *outline=guard ? body->lumina_guard : body->lumina_blade;
    unsigned pulse=body->blade_tick&3u;
    unsigned level=128+32*(pulse<=2 ? pulse : 1);
    for (unsigned k=0;k<4;++k) {
        int center=seam ? k<2 : k==1;
        xyz[k][0]=xyz[k][1]=xyz[k][2]=0;
        rgb[k]=center ? level | ((level/4)<<8) | ((level/4)<<16) : 0;
        if (center) {
            if (!guard) xyz[k][2]=seam ? (k ? 32 : -32) : (face<6 ? -32 : 32);
        } else {
            unsigned i=start+(seam ? k-2 : k ? k-1 : 0);
            xyz[k][0]=outline[2*i];
            xyz[k][guard ? 1 : 2]=outline[2*i+1];
        }
    }
}

/* The leading six bytes of the current body pose describe the held blade
 * axis in actor coordinates. The two mounted outlines face the camera. */
static void collect_blade(BfmBedroomFigure *figure, const BfmBedroomView *view,
                          const BfmBedroomBody *body, const Transform *root,
                          const Transform *eye, int lumina)
{
    int8_t anchors[2][3];
    BfmForestTransform player, halo;
    memcpy(anchors, body->pose+(lumina ? 8 : 0), sizeof anchors);
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) player.rotation[i][j] = (int16_t)root->r[i][j];
        player.translation[i] = root->t[i];
    }
    if (!bfm_forest_blade_transform(anchors, &player, view->eye, &halo)) return;
    Transform world;
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) world.r[i][j] = halo.rotation[i][j];
        world.t[i] = halo.translation[i];
    }
    Transform posed = compose(eye, &world);
    BfmBedroomView camera = {.focal = view->focal};
    memcpy(camera.axes, posed.r, sizeof camera.axes);
    memcpy(camera.translation, posed.t, sizeof camera.translation);
    for (unsigned i = 0; i < (lumina ? 10u : 8u); ++i) {
        uint32_t colors[4];
        int16_t xyz[4][3];
        if (lumina) lumina_quad(body,i,xyz,colors);
        else if (!bfm_forest_blade_quad(body->blade, i, body->blade_tick, xyz, colors)) return;
        FigureFace *face = &figure->faces[figure->count];
        int screen[3];
        unsigned k = 0;
        for (; k < 4; ++k) {
            if (!bfm_bedroom_project(&camera, xyz[k], screen)) break;
            uint32_t color = colors[k];
            if (body->green_blade && !lumina) {
                unsigned blue = color >> 16;
                color = ((blue / 2) << 16) | (blue << 8) | (i < 4 ? blue / 4 : blue);
            }
            face->v[k] = (BfmForestRasterVertex){.x = screen[0], .y = screen[1], .rgb = color};
            include_pixel(figure, screen[0], screen[1]);
        }
        if (k < 4) continue;
        face->depth = screen[2] / 4;
        face->material = (BfmForestRasterMaterial){.page = 0x12a, .count = 4, .translucent = 1};
        face->order = figure->count++;
    }
}

static int back_to_front(const void *a, const void *b)
{
    const FigureFace *x = a, *y = b;
    if (x->depth != y->depth) return x->depth < y->depth ? 1 : -1;
    return x->order < y->order ? 1 : x->order > y->order ? -1 : 0;
}

int bfm_bedroom_figure_prepare(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmBedroomBody *body, uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!figure) return 0;
    figure->count = figure->next = 0;
    figure->visitors_added = 0;
    figure->surface.pixels = NULL;
    figure->x_min = 320; figure->x_max = -1;
    figure->y_min = 230; figure->y_max = 9;
    if (!view || !tiles || !pixels || !body || !body->pose || body->pose_bytes < 256 ||
        view->focal <= 0 || view->focal > 65535 ||
        (body->face_word && (body->face_word & ~7u) != UINT32_C(0x40000000)))
        return 0;
    if (body->surface_normal[1] >= 0 || (body->shadow_visible && body->shadow_normal[1] >= 0))
        return 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (body->shadow_visible && (body->shadow_position[i] < INT16_MIN ||
            body->shadow_position[i] > INT16_MAX || body->shadow_normal[i] < -4096 ||
            body->shadow_normal[i] > 4096)) return 0;
        if (body->surface_normal[i] < -4096 || body->surface_normal[i] > 4096 ||
            body->position[i] < INT16_MIN || body->position[i] > INT16_MAX ||
            view->translation[i] < -16777216 || view->translation[i] > 16777215)
            return 0;
        for (unsigned j = 0; j < 3; ++j)
            if (view->axes[i][j] < INT16_MIN || view->axes[i][j] > INT16_MAX) return 0;
    }
    if (body->face_word) {
        const unsigned *rect = figure->face_sources[body->face_word & 7u];
        for (unsigned row = 0; row < rect[3]; ++row)
            memcpy(&tiles[256+row][512], &tiles[rect[1]+row][rect[0]], rect[2]*sizeof(uint16_t));
    }
    figure->surface = (BfmForestRaster){.tiles=(const uint16_t (*)[1024])tiles,
                        .pixels=(uint32_t (*)[320])pixels,.top=10,.bottom=229};
    Transform root, eye;
    const double radians = 6.28318530717958647693 / 4096;
    double yaw = ((unsigned)body->heading & 4095)*radians;
    double pitch = ((unsigned)body->pitch & 4095)*radians;
    rotation((int)lround(4096*sin(pitch)), (int)lround(4096*cos(pitch)),
             (int)lround(4096*sin(yaw)), (int)lround(4096*cos(yaw)), 0, 4096, root.r);
    Transform slope = support_rotation(body->surface_normal);
    memset(root.t, 0, sizeof(root.t));
    root = compose(&slope,&root);
    memcpy(root.t, body->position, sizeof(root.t));
    memcpy(eye.r, view->axes, sizeof(eye.r));
    memcpy(eye.t, view->translation, sizeof(eye.t));
    Transform base = compose(&eye,&root);
    if (body->shadow_visible) collect_shadow(figure,view,&eye,body,&figure->shadow,8192,0x808080u);
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if (!(body->visible_parts & (UINT32_C(1) << i))) continue;
        Transform local = local_pose(body->pose+16,i);
        Transform world = compose(&root,&local), posed = compose(&base,&local);
        BfmBedroomView camera = {.focal=view->focal};
        memcpy(camera.axes, posed.r, sizeof(camera.axes));
        memcpy(camera.translation, posed.t, sizeof(camera.translation));
        collect_part(figure,&camera,&world,musashi_forest_player_mesh_part(figure->mesh,i),i < 2);
    }
    if (body->blade) collect_blade(figure, view, body, &root, &eye, 0);
    if (body->lumina_guard && body->lumina_blade) collect_blade(figure, view, body, &root, &eye, 1);
    qsort(figure->faces,figure->count,sizeof(*figure->faces),back_to_front);
    return 1;
}

static int reserve_faces(BfmBedroomFigure *figure, size_t extra)
{
    if (extra > SIZE_MAX / sizeof(*figure->faces) - figure->count) return 0;
    size_t needed = figure->count + extra;
    if (needed <= figure->capacity) return 1;
    FigureFace *faces = realloc(figure->faces, needed * sizeof(*faces));
    if (!faces) return 0;
    figure->faces = faces;
    figure->capacity = needed;
    return 1;
}

static Transform model_transform(const BfmForestTransform *source)
{
    Transform transform;
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) transform.r[i][j] = source->rotation[i][j];
        transform.t[i] = source->translation[i];
    }
    return transform;
}

int bfm_bedroom_figure_add_fusion(BfmBedroomFigure *figure, const BfmBedroomView *view,
                     const BfmForestActor *blade, const BfmForestDustCard *cards, size_t count)
{
    if (!figure || !view || figure->next || !figure->surface.pixels ||
        count > BFM_TETHER_CARDS || (count && !cards) || view->focal <= 0 ||
        view->focal > UINT16_MAX) return 0;
    if (!count && !blade) return 1;
    for (size_t i = 0; i < count; ++i)
        if (cards[i].material.count != 4 || cards[i].bucket > INT_MAX) return 0;
    const MusashiForestPlayerPart *part = NULL;
    if (blade) {
        if (!blade->mesh || !blade->root || !blade->pose || blade->visible_parts != 1) return 0;
        part = musashi_forest_player_mesh_part(blade->mesh, 0);
        if (!part || part->face_count > SIZE_MAX - count) return 0;
    }
    if (!reserve_faces(figure, count + (part ? part->face_count : 0))) return 0;
    if (blade) {
        Transform eye;
        memcpy(eye.r, view->axes, sizeof eye.r);
        memcpy(eye.t, view->translation, sizeof eye.t);
        Transform root = model_transform(blade->root);
        Transform local = model_transform(&blade->pose->local[0]);
        Transform world = compose(&root, &local), base = compose(&eye, &root);
        Transform posed = compose(&base, &local);
        BfmBedroomView camera = {.focal = view->focal};
        memcpy(camera.axes, posed.r, sizeof camera.axes);
        memcpy(camera.translation, posed.t, sizeof camera.translation);
        collect_part(figure, &camera, &world, part, 0);
    }
    for (size_t i = 0; i < count; ++i) {
        const BfmForestDustCard *card = &cards[i];
        FigureFace *face = &figure->faces[figure->count];
        memcpy(face->v, card->vertices, sizeof face->v);
        face->material = card->material;
        face->depth = (int)card->bucket;
        face->order = figure->count++;
        for (unsigned k = 0; k < 4; ++k) include_pixel(figure, face->v[k].x, face->v[k].y);
    }
    qsort(figure->faces, figure->count, sizeof(*figure->faces), back_to_front);
    return 1;
}

int bfm_bedroom_figure_next_depth(const BfmBedroomFigure *figure)
{
    return figure && figure->next < figure->count ? figure->faces[figure->next].depth : INT_MIN;
}

void bfm_bedroom_figure_draw_to(void *context, int depth)
{
    BfmBedroomFigure *figure = context;
    if (!figure) return;
    while (figure->next < figure->count && figure->faces[figure->next].depth >= depth) {
        const FigureFace *face = &figure->faces[figure->next++];
        bfm_forest_raster_face(&figure->surface,&face->material,face->v);
    }
    /* The room scanout maps five-bit channels to RGB using integer division.
     * The shared raster stores the same codes with bit replication. */
    if (depth != INT_MIN || !figure->count) return;
    int x0 = figure->x_min < 0 ? 0 : figure->x_min;
    int x1 = figure->x_max > 319 ? 319 : figure->x_max;
    int y0 = figure->y_min < 10 ? 10 : figure->y_min;
    int y1 = figure->y_max > 229 ? 229 : figure->y_max;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            uint32_t p = figure->surface.pixels[y][x];
            uint32_t rgb = p & 0xff000000u;
            for (unsigned bit = 0; bit < 24; bit += 8)
                rgb |= (((p >> (bit+3)) & 31) * 255 / 31) << bit;
            figure->surface.pixels[y][x] = rgb;
        }
}

static const uint32_t visitor_scene_origin = UINT32_C(0x80128158);
struct BfmBedroomVisitor {
    const uint8_t *scene;
    size_t bytes;
    MusashiForestPlayerMesh *model[2];
    unsigned parts[2];
    size_t faces[2];
    ShadowRect shadow;
};

void bfm_bedroom_visitor_free(BfmBedroomVisitor *visitor)
{
    if (!visitor) return;
    for (unsigned i = 0; i < 2; ++i) musashi_forest_player_mesh_free(visitor->model[i]);
    free(visitor);
}

BfmBedroomVisitor *bfm_bedroom_visitor_load(const uint8_t *scene, size_t size,
                                             const uint32_t *tables, unsigned count, uint32_t shadow_address)
{
    if (!tables || !count || count > 2 || shadow_address < visitor_scene_origin) return NULL;
    size_t shadow_offset = shadow_address-visitor_scene_origin;
    if (!scene || size > BFM_FIELD_SCENE_CAPACITY || size < shadow_offset+12) return NULL;
    BfmBedroomVisitor *visitor = calloc(1,sizeof(*visitor));
    if (!visitor) return NULL;
    if (!read_shadow(&visitor->shadow,scene+shadow_offset)) {
        free(visitor);
        return NULL;
    }
    visitor->scene = scene;
    visitor->bytes = size;
    for (unsigned i = 0; i < count; ++i) {
        visitor->model[i] = musashi_forest_player_mesh_from_table(scene,size,
                                              visitor_scene_origin,tables[i],&visitor->parts[i]);
        if (!visitor->model[i]) {
            bfm_bedroom_visitor_free(visitor);
            return NULL;
        }
        for (unsigned part = 0; part < visitor->parts[i]; ++part)
            visitor->faces[i] += musashi_forest_player_mesh_part(visitor->model[i],part)->face_count;
    }
    return visitor;
}

int bfm_bedroom_visitor_add(const BfmBedroomVisitor *visitor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view, unsigned index,
                           const int position[3], int heading, uint32_t pose_address)
{
    if (!visitor || !figure || !view || !position || index >= 2 || !visitor->model[index] || figure->next ||
        (figure->visitors_added & (1u << index)) ||
        !figure->surface.pixels || pose_address < visitor_scene_origin ||
        (pose_address & 3) || view->focal <= 0 || view->focal > 65535) return 0;
    size_t offset = pose_address-visitor_scene_origin;
    if (offset > visitor->bytes || visitor->parts[index] > (visitor->bytes-offset)/8) return 0;
    for (unsigned k = 0; k < 3; ++k)
        if (position[k] < INT16_MIN || position[k] > INT16_MAX) return 0;
    if (!reserve_faces(figure, visitor->faces[index] + 1)) return 0;
    int sine, cosine;
    angle((unsigned)heading,&sine,&cosine);
    Transform root, eye;
    rotation(0,4096,sine,cosine,0,4096,root.r);
    memcpy(root.t,position,sizeof(root.t));
    memcpy(eye.r,view->axes,sizeof(eye.r));
    memcpy(eye.t,view->translation,sizeof(eye.t));
    Transform base = compose(&eye,&root);
    /* The visitors walk on the bedroom floor. Their sprite faces downward
     * and takes room lighting before its texels subtract from the floor. */
    const MusashiForestPlayerVector underside = {0,4096,0};
    const uint8_t neutral[3] = {128,128,128};
    uint32_t ink = shade(figure,figure->direction,&underside,neutral);
    BfmBedroomBody caster = {.heading=(int)((2048u-(unsigned)heading) & 4095u),.shadow_normal={0,-4096,0}};
    memcpy(caster.position,position,sizeof(caster.position));
    memcpy(caster.shadow_position,position,sizeof(caster.shadow_position));
    collect_shadow(figure,view,&eye,&caster,&visitor->shadow,12288,ink);
    figure->visitors_added |= 1u << index;
    const uint8_t *pose = visitor->scene + offset;
    for (unsigned i = 0; i < visitor->parts[index]; ++i, pose += 8) {
        int sx, cx, sy, cy, sz, cz;
        Transform local;
        angle(read16(pose+6),&sx,&cx);
        angle(((pose[0]&15u)<<8) | pose[1],&sy,&cy);
        angle(((pose[0]&240u)<<4) | pose[2],&sz,&cz);
        rotation(sx,cx,sy,cy,sz,cz,local.r);
        for (unsigned k = 0; k < 3; ++k) local.t[k] = (int8_t)pose[3+k];
        Transform world = compose(&root,&local), posed = compose(&base,&local);
        BfmBedroomView camera = {.focal=view->focal};
        memcpy(camera.axes,posed.r,sizeof(camera.axes));
        memcpy(camera.translation,posed.t,sizeof(camera.translation));
        collect_part(figure,&camera,&world,musashi_forest_player_mesh_part(visitor->model[index],i),0);
    }
    qsort(figure->faces,figure->count,sizeof(*figure->faces),back_to_front);
    return 1;
}
