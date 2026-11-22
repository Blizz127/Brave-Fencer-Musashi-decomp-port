#include "forest_event.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* The standalone fixture owns the real twenty-element host pool. */
ForestEventSlot D_8011F9D0[20];

_Static_assert(sizeof(ForestActionEventRecord) == 20, "event request");
_Static_assert(offsetof(ForestActionEventRecord, actor) == 8, "event actor");
_Static_assert(sizeof(ForestEventSlot) == 104, "slot stride");
_Static_assert(offsetof(ForestEventSlot, x) == 6, "slot X");
_Static_assert(offsetof(ForestEventSlot, y) == 10, "slot Y");
_Static_assert(offsetof(ForestEventSlot, z) == 14, "slot Z");
_Static_assert(offsetof(ForestEventSlot, actor) == 76, "slot actor");
_Static_assert(offsetof(ForestEventSlot, first) == 80, "slot data");
_Static_assert(offsetof(ForestEventSlot, second) == 84, "slot data");

int main(void)
{
    uint32_t header[3];
    while (fread(header, sizeof(header), 1, stdin) == 1) {
        ForestActionEventRecord local;
        ForestActionEventRecord *request = &local;
        assert(fread(D_8011F9D0, sizeof(D_8011F9D0), 1, stdin) == 1);
        assert(fread(&local, sizeof(local), 1, stdin) == 1);
        assert(header[0] <= 1 && header[2] >= 1 && header[2] <= 3);
        if (header[1] != UINT32_MAX) {
            assert(header[1] % _Alignof(ForestActionEventRecord) == 0);
            assert(header[1] <= sizeof(D_8011F9D0) - sizeof(local));
            request = (ForestActionEventRecord *)((unsigned char *)D_8011F9D0 + header[1]);
        }
        for (unsigned i = 0; i < header[2]; i++) {
            ForestEventSlot *slot = header[0] ? func_8014680C(request) : func_80146750(request);
            uint32_t result = 0;
            if (slot) {
                assert(slot >= D_8011F9D0 && slot < D_8011F9D0 + 20);
                result = 0x8011f9d0u + (uint32_t)(slot - D_8011F9D0) * sizeof(*slot);
            }
            assert(fwrite(&result, sizeof(result), 1, stdout) == 1);
        }
        assert(fwrite(D_8011F9D0, sizeof(D_8011F9D0), 1, stdout) == 1);
        assert(fwrite(&local, sizeof(local), 1, stdout) == 1);
    }
    return ferror(stdin) || !feof(stdin) || fflush(stdout);
}
