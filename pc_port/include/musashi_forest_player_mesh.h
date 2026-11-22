#ifndef MUSASHI_FOREST_PLAYER_MESH_H
#define MUSASHI_FOREST_PLAYER_MESH_H

#include "musashi_disc_media.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { MUSASHI_FOREST_PLAYER_PARTS = 31 };

typedef struct MusashiForestPlayerMesh MusashiForestPlayerMesh;

typedef struct {
    int16_t x, y, z;
} MusashiForestPlayerVector;

typedef struct {
    uint8_t mode, corners;
    uint8_t rgb[3], u[4], v[4];
    uint16_t clut, tpage;
    uint16_t vertex[4], normal[4];
} MusashiForestPlayerFace;

typedef struct {
    size_t vertex_count, normal_count, face_count;
    const MusashiForestPlayerVector *vertices, *normals;
    const MusashiForestPlayerFace *faces;
} MusashiForestPlayerPart;

/* Decode all components of Musashi's forest model from authenticated media.
 * The result owns its data and survives closing the disc. Part order follows
 * the model table. The scene chooses which equipment components to draw. */
MusashiForestPlayerMesh *musashi_forest_player_mesh_load(const MusashiDiscMedia *media);

/* Arrays and part descriptions remain valid until mesh_free. Indices in a
 * face refer to vertices and normals in that part. Textured faces use the
 * neutral input RGB (128,128,128) for subsequent lighting. */
const MusashiForestPlayerPart *musashi_forest_player_mesh_part(
    const MusashiForestPlayerMesh *mesh, unsigned index);
void musashi_forest_player_mesh_free(MusashiForestPlayerMesh *mesh);

#ifdef __cplusplus
}
#endif
#endif
