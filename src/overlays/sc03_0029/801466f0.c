#include "fusion_queue_game.h"

FusionWord func_801466F0(int kind, FusionWord actor, int component0, int component1,
                         FusionHalf component2, FusionWord parameter5, FusionWord parameter6,
                         int secondary)
{
    FusionRequest request;

    request.kind = kind;
    request.actor = actor;
    request.component0 = component0;
    request.component1 = component1;
    request.component2 = component2;
    request.parameter5 = parameter5;
    request.parameter6 = parameter6;
    if (!secondary)
        return func_8014680C(&request);
    return func_80146750(&request);
}
