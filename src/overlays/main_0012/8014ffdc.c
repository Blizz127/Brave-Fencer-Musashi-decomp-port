/* Select the lowest successful actor probe and its default normal. */
#include "floor_providers.h"

s32 func_8014FFDC(Obj8014FE60 *actor, SceneProbeVector *best, SceneProbeVector *normal)
{
    SceneProbeVector start;
    SceneProbeVector probe;
    Obj8014A2E4 *entry;
    s32 found;

    (void)actor;
    found = 0;
    start = *best;
    best->y = 0x7FFF;
    for (entry = D_801202A0; entry < D_801202A0 + 96; entry++) {
        if (entry->active != 0 && (entry->flags & 0x400) != 0 && entry->probe_geometry_reference != 0) {
            probe = start;
            if (func_80135D20(entry->probe_object_address, entry->probe_geometry_reference, &probe) != 0) {
                found = 1;
                if (probe.y < best->y) {
                    *best = probe;
                    *normal = D_801152A8;
                }
            }
        }
    }
    return found;
}
