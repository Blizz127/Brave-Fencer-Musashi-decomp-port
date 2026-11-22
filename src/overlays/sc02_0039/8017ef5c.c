#include "mine_child_query.h"

#if defined(__mips__) && !defined(__LP64__)
extern s32 func_8012DEB8(char *actor, int start, int end);
#define query_segment(actor, start, end) func_8012DEB8((char *)(actor), (int)(start), (int)(end))
#else
#define query_segment bfm_mine_child_query_segment
#endif

s32 func_8017EF5C(MineChildModelActor *actor, MineChildModelRecord *record)
{
    MineChildQueryShape *shape = (MineChildQueryShape *)record;
    SceneProbeVector start, end;
    int row, column;

#if !defined(__mips__) || defined(__LP64__)
    if (!actor || !record)
        return 0;
#endif
    end.x = shape->record.extent >> 1;
    start.x = -end.x;
    for (row = 0; row < shape->record.count; ++row) {
        start.y = end.y = shape->samples[row];
        for (column = 0; column < shape->record.count; ++column) {
            start.z = end.z = shape->samples[column];
            if (query_segment(actor, &start, &end))
                return 1;
        }
    }
    return 0;
}
