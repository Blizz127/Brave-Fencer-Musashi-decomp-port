#include "fusion_queue_game.h"

FusionWord func_8014680C(FusionRequest *request)
{
    int i;
    for (i = 0; i < 8; ++i) {
        if (D_8011F9D0[i].kind == 0) {
            D_8011F9D0[i].actor = request->actor;
            D_8011F9D0[i].kind = request->kind;
            D_8011F9D0[i].component0 = request->component0;
            D_8011F9D0[i].component1 = request->component1;
            D_8011F9D0[i].component2 = request->component2;
            D_8011F9D0[i].parameter5 = request->parameter5;
            D_8011F9D0[i].parameter6 = request->parameter6;
            return TWINPEAK_SLOT_WORD(i);
        }
    }
    return 0;
}
