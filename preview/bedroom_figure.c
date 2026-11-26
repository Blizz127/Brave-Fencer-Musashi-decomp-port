/* SPDX-License-Identifier: MIT */
#include "bedroom_figure.h"
#include "forest_raster.h"

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

struct BfmBedroomFigure {
    MusashiForestPlayerMesh *mesh;
    int direction[3][3], color[3][3], ambient[3];
    unsigned face_sources[8][4];
    int shadow_left, shadow_top;
    unsigned shadow_width, shadow_height;
    FigureFace *faces;
    size_t count, next;
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

static int light_matrix(BfmBedroomFigure *figure, const uint8_t *room)
{
    for (unsigned i = 0; i < 3; ++i) {
        const uint8_t *source = room + 6 + 10*i;
        int axis[3];
        uint64_t squared = 0;
        for (unsigned k = 0; k < 3; ++k) {
            axis[k] = (int16_t)read16(source+2*k);
            squared += (unsigned)(axis[k]*axis[k]);
        }
        unsigned length = (unsigned)sqrt((double)quantize_length(squared));
        if (!length) return 0;
        for (unsigned k = 0; k < 3; ++k) {
            figure->direction[i][k] = -axis[k]*4096/(int)length;
            figure->color[k][i] = source[6+k]*4096/255;
        }
        int ambient = (int16_t)read16(room+2*i);
        figure->ambient[i] = ambient - ((ambient % 16 + 16) % 16);
    }
    return 1;
}

BfmBedroomFigure *bfm_bedroom_figure_load(const MusashiDiscMedia *disc,
                     const uint8_t *scene, size_t size, uint32_t room_address)
{
    const unsigned origin = 0x80128158, face_table = 0x801841dc - 0x80128158;
    if (!disc || !scene || room_address < origin || size < 36 ||
        room_address-origin > size-36 || size < face_table+64 || size < 0x80184f48-origin+12)
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
    const uint8_t *shadow = scene + (0x80184f48 - origin);
    figure->shadow_width = shadow[2];
    figure->shadow_height = shadow[3];
    figure->shadow_left = (int16_t)read16(shadow+8);
    figure->shadow_top = (int16_t)read16(shadow+10);
    if (!figure->shadow_width || figure->shadow_width > 192 ||
        !figure->shadow_height || figure->shadow_height > 96 ||
        figure->shadow_left < -256 || figure->shadow_left > 256 ||
        figure->shadow_top < -256 || figure->shadow_top > 256)
        goto fail;
    figure->mesh = musashi_forest_player_mesh_load(disc);
    if (!figure->mesh) goto fail;
    size_t capacity = 1;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(figure->mesh,i);
        if (part->face_count > SIZE_MAX / sizeof(FigureFace) - capacity) goto fail;
        capacity += part->face_count;
    }
    figure->faces = calloc(capacity, sizeof(*figure->faces));
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
                         const Transform *world, unsigned i)
{
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(figure->mesh,i);
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
                (corner < 3 || i < 2 || (face->mode != 0x38 && face->mode != 0x3c)))
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
            if (!k || (face->mode & 16))
                rgb = shade(figure, light, &part->normals[face->normal[k]], face->rgb);
            out->v[k].rgb = rgb;
            include_pixel(figure,out->v[k].x,out->v[k].y);
        }
        out->material = (BfmForestRasterMaterial){
            .page=face->tpage, .clut=face->clut, .count=face->corners,
            .textured=!!(face->mode & 4), .translucent=!!(face->mode & 2)
        };
        out->depth = i < 2 ? near/4-1 : (sum*(face->corners == 3 ? 341 : 256))/4096;
        out->order = figure->count++;
    }
}

/* The shadow caster projects the player's contact, independently of the animated
 * body. Separation from the mounting plane shrinks the footprint during a fall. */
static void collect_shadow(BfmBedroomFigure *figure, const BfmBedroomView *view,
                           const Transform *eye, const BfmBedroomBody *body)
{
    const double turn = 6.28318530717958647693 / 4096;
    double yaw = -((int)((unsigned)body->heading & 4095)) * turn;
    Transform bearing = {0};
    rotation(0,4096,(int)lround(4096*sin(yaw)),(int)lround(4096*cos(yaw)),0,4096,bearing.r);
    Transform slope = support_rotation(body->shadow_normal);
    Transform ground = compose(&slope,&bearing);
    memcpy(ground.t,body->shadow_position,sizeof(ground.t));
    int separation = body->shadow_position[1] - body->position[1];
    int scale = (768-separation)*8192/768;
    if (scale <= 0 || scale > INT16_MAX) return;
    Transform posed = compose(eye,&ground);
    BfmBedroomView camera = {.focal=view->focal};
    memcpy(camera.axes,posed.r,sizeof(camera.axes));
    memcpy(camera.translation,posed.t,sizeof(camera.translation));
    FigureFace *face = &figure->faces[figure->count];
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = (i >> 1) & 1;
        int16_t point[3] = {
            (int16_t)((figure->shadow_left + right*(int)figure->shadow_width)*scale/4096),0,
            (int16_t)((figure->shadow_top + bottom*(int)figure->shadow_height)*scale/4096)
        };
        int screen[3];
        if (!bfm_bedroom_project(&camera,point,screen)) return;
        include_pixel(figure,screen[0],screen[1]);
        face->v[i] = (BfmForestRasterVertex){.x=screen[0],.y=screen[1],
            .u=(uint8_t)(64 + right*(figure->shadow_width-1)),
            .v=(uint8_t)(160 + bottom*(figure->shadow_height-1)),.rgb=0x808080u};
    }
    face->material = (BfmForestRasterMaterial){.page=0x59,.clut=0x4516,
                                               .textured=1,.translucent=1,.count=4};
    face->depth = posed.t[2]/4+2;
    face->order = figure->count++;
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
    if (body->shadow_visible) collect_shadow(figure,view,&eye,body);
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if (!(body->visible_parts & (UINT32_C(1) << i))) continue;
        Transform local = local_pose(body->pose+16,i);
        Transform world = compose(&root,&local), posed = compose(&base,&local);
        BfmBedroomView camera = {.focal=view->focal};
        memcpy(camera.axes, posed.r, sizeof(camera.axes));
        memcpy(camera.translation, posed.t, sizeof(camera.translation));
        collect_part(figure,&camera,&world,i);
    }
    qsort(figure->faces,figure->count,sizeof(*figure->faces),back_to_front);
    return 1;
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
