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
    uint8_t corner_rgb[4][3];
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

/* Decode a bounded, null-terminated scene TMD table. Its ordered part blocks
 * end at the table. Returns the actual number of parts, up to the player limit. */
MusashiForestPlayerMesh *musashi_forest_player_mesh_from_table(
    const uint8_t *scene, size_t bytes, uint32_t origin, uint32_t table,
    unsigned *part_count);

/* Arrays and part descriptions remain valid until mesh_free. Indices in a
 * face refer to vertices and normals in that part. Textured faces use the
 * neutral input RGB (128,128,128) for subsequent lighting. */
const MusashiForestPlayerPart *musashi_forest_player_mesh_part(
    const MusashiForestPlayerMesh *mesh, unsigned index);
/* Borrow this mesh's geometry and replace one part's draw material with opaque
 * white. The source must outlive the view; release either with mesh_free. */
MusashiForestPlayerMesh *musashi_forest_player_mesh_white_part_view(
    const MusashiForestPlayerMesh *mesh, unsigned part);
uint32_t musashi_forest_player_mesh_white_parts(const MusashiForestPlayerMesh *mesh);
/* Borrow geometry with extra vertex modulation for the selected parts.
 * Textured faces retain the mounted material. The source outlives the view. */
MusashiForestPlayerMesh *musashi_forest_player_mesh_lit_view(
    const MusashiForestPlayerMesh *mesh, uint32_t parts, uint8_t extra_light);
unsigned musashi_forest_player_mesh_extra_light(const MusashiForestPlayerMesh *mesh,
                                                unsigned part);
void musashi_forest_player_mesh_free(MusashiForestPlayerMesh *mesh);

#ifdef __cplusplus
}
#endif
#endif
