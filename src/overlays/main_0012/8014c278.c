/* Owner re-derivation from retail assembly (clean input set). */
/* Returns whether the distance between two objects' +4 positions is below a 16-bit limit. */
#include "object_distance.h"
#include "scene_object_probes.h"
int func_8014C278(char *a, char *b, int limit) {
    return func_80013328(a + 4, b + 4) < (limit & 0xFFFF);
}
