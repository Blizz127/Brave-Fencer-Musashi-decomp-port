/* Transform a short scene vector using the supplied rotation and translation. */
#include "camera_transform.h"

extern void func_8004914C(void *matrix);
extern void func_800491AC(void *matrix);
extern void func_8004945C(void *source, void *destination, int *flags);

void func_8012F14C(ForestCameraMatrix *matrix, ForestCameraVector *source,
                   ForestCameraVector *destination)
{
    int flags;

    func_8004914C(matrix);
    func_800491AC(matrix);
    func_8004945C(source, destination, &flags);
}
