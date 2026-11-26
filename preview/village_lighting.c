/* SPDX-License-Identifier: MIT */
#include "village_lighting.h"

#include <string.h>

int bfm_village_lower_light(BfmBedroomFigure *figure, const uint8_t *scene,
                            size_t bytes, unsigned phase)
{
    const size_t table = 0x8018de3cu - 0x80128158u;
    if (!scene || phase >= 5 || bytes < table || bytes - table < 45) return 0;
    BfmBedroomLight lights[3] = {
        {.axis = {0,30,0}}, {.axis = {0,7,20}}, {.axis = {0,-18,0}}
    };
    const uint8_t *colors = scene + table + phase * 9;
    for (unsigned i = 0; i < 3; ++i) memcpy(lights[i].rgb,colors + 3*i,3);
    return bfm_bedroom_figure_lights(figure,lights);
}
