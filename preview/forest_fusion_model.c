/* SPDX-License-Identifier: MIT */
#include "forest_fusion_model.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

struct BfmFusionModel {
    MusashiForestPlayerMesh *mesh;
    BfmForestTransform root;
    BfmForestPose pose;
    BfmForestActor actor;
    int width;
    BfmFusionFlightPhase previous;
};

static MusashiForestPlayerMesh *load_sword(const MusashiDiscMedia *disc)
{
    const uint32_t table = UINT32_C(0x800de5a0);
    ForestPlayerArchive archive;
    uint8_t pointers[8];
    if (!forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, table, pointers, sizeof pointers) ||
        u32le(pointers + 4)) return NULL;
    uint32_t start = u32le(pointers);
    if (start < UINT32_C(0x800cedf8) || start >= table || (start & 3)) return NULL;
    size_t size = table - start + sizeof pointers;
    uint8_t *data = malloc(size);
    if (!data) return NULL;
    MusashiForestPlayerMesh *mesh = NULL;
    unsigned count = 0;
    if (forest_player_archive_read(&archive, start, data, size))
        mesh = musashi_forest_player_mesh_from_table(data, size, start, table, &count);
    free(data);
    if (count == 1) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

static int load_pose(const MusashiDiscMedia *disc, BfmForestPose *pose)
{
    const uint32_t address = UINT32_C(0x800636c8);
    uint8_t header[32], part[8];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", 0, header, sizeof header, &size) ||
        size < 2048 || memcmp(header, "PS-X EXE", 8)) return 0;
    uint32_t base = u32le(header + 24), length = u32le(header + 28);
    if (base > address || length > size - 2048 || length < sizeof part ||
        address - base > length - sizeof part) return 0;
    return musashi_disc_media_read_root_range(disc, "SLUS_007.26", 2048u + (size_t)(address - base),
               part, sizeof part, &size) && bfm_forest_actor_pose_decode(part, sizeof part, 1, pose);
}

BfmFusionModel *bfm_fusion_model_open(const MusashiDiscMedia *disc)
{
    if (!disc) return NULL;
    BfmFusionModel *model = calloc(1, sizeof *model);
    if (!model) return NULL;
    model->mesh = load_sword(disc);
    if (!model->mesh || !load_pose(disc, &model->pose)) {
        bfm_fusion_model_free(model);
        return NULL;
    }
    model->actor = (BfmForestActor){ .mesh = model->mesh, .pose = &model->pose,
                                     .root = &model->root, .visible_parts = 1 };
    model->width = 6144;
    return model;
}

void bfm_fusion_model_free(BfmFusionModel *model)
{
    if (!model) return;
    musashi_forest_player_mesh_free(model->mesh);
    free(model);
}

const BfmForestActor *bfm_fusion_model_tick(BfmFusionModel *model, const BfmFusionFlight *flight,
                                           int absorption_square_edge)
{
    if (!model || !flight || flight->phase == BFM_FUSION_IDLE || flight->phase == BFM_FUSION_DONE)
        return NULL;
    if (flight->phase == BFM_FUSION_OUTBOUND) model->width = 6144;
    if (flight->phase == BFM_FUSION_ATTACHED ||
        (model->previous == BFM_FUSION_ATTACHED && absorption_square_edge)) {
        if (absorption_square_edge) model->width = 16384;
        int delta = model->width - 6144;
        if (delta) model->width -= delta < 3 ? 1 : delta / 3;
    }
    model->previous = flight->phase;
    model->root = flight->model;
    for (unsigned row = 0; row < 3; ++row) {
        for (unsigned col = 0; col < 3; ++col) {
            int scale = col == 2 ? 6144 : model->width;
            int product = scale * model->root.rotation[row][col];
            model->root.rotation[row][col] = (int16_t)(product / 4096 - (product % 4096 < 0));
        }
    }
    return &model->actor;
}
