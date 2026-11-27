/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_notice.h"
#include "woods_fusion_art.h"

int bfm_woods_rootrick_notice_start(BfmForestMarker *marker,
                                    const uint8_t *scene, size_t bytes,
                                    const int16_t feet[3])
{
    const size_t offset = UINT32_C(0x80192148) - UINT32_C(0x80128158);
    if (!marker || !feet || !scene || bytes < offset + 12 ||
        !bfm_reaction_start_sprite(&marker->reaction, scene + offset, 12))
        return 0;
    for (unsigned i = 0; i < 3; ++i) marker->owner[i] = feet[i];
    marker->y_offset = -128;
    bfm_bedroom_reaction_step(&marker->reaction);
    return 1;
}

int bfm_woods_rootrick_notice_add(const BfmForestMarker *marker, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view)
{
    BfmForestCamera camera;
    BfmForestDustCard sprite;
    if (!marker || !figure || !bfm_woods_effect_camera(view, &camera)) return 0;
    return !bfm_forest_marker_project(marker, &camera, &sprite) ||
           bfm_bedroom_figure_add_fusion(figure, view, NULL, &sprite, 1);
}
