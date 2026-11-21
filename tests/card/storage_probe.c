#include "card_host_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t before[CS_IMAGE_BYTES], after[CS_IMAGE_BYTES], check[CS_IMAGE_BYTES];

static void fill(uint8_t *bytes, unsigned tag)
{
    size_t i;
    for (i = 0; i < CS_IMAGE_BYTES; ++i)
        bytes[i] = (uint8_t)(i * 23u + i / 113u + tag);
}

int main(int argc, char **argv)
{
    CsStorage *handles[2];
    char line[128], operation[16];
    if (argc != 2 || bfm_plat_memcard_set_dir(argv[1]) != BFM_PLAT_OK)
        return EXIT_FAILURE;
    handles[0] = cs_storage_create();
    handles[1] = cs_storage_create();
    if (!handles[0] || !handles[1])
        return EXIT_FAILURE;
    while (fgets(line, (int)sizeof(line), stdin)) {
        unsigned handle = 0, slot = 0;
        int old_tag = -1, new_tag = -1;
        int fields = sscanf(line, "%15s %u %u %d %d", operation,
                            &handle, &slot, &old_tag, &new_tag);
        if (fields >= 1 && strcmp(operation, "end") == 0)
            break;
        if (fields >= 1 && strcmp(operation, "abandon") == 0)
            _Exit(EXIT_SUCCESS);
        if (fields < 3 || handle > 1)
            return EXIT_FAILURE;
        if (strcmp(operation, "attach") == 0) {
            printf("%d\n", cs_storage_attach(handles[handle], slot));
        } else if (strcmp(operation, "detach") == 0) {
            cs_storage_detach(handles[handle], slot);
            puts("ok");
        } else if (strcmp(operation, "drop") == 0) {
            cs_storage_destroy(handles[handle]);
            handles[handle] = cs_storage_create();
            if (!handles[handle])
                return EXIT_FAILURE;
            puts("ok");
        } else if (strcmp(operation, "put") == 0 && fields == 5) {
            fill(before, (unsigned)old_tag);
            fill(after, (unsigned)new_tag);
            printf("%d\n", cs_storage_commit(handles[handle], slot,
                                            old_tag < 0 ? NULL : before, after));
        } else if (strcmp(operation, "snap") == 0 && fields == 4) {
            CsMedia media;
            memset(before, 0x82, sizeof(before));
            memset(check, 0x82, sizeof(check));
            media = cs_storage_snapshot(handles[handle], slot, before);
            if (media == CS_MEDIA_OK)
                fill(check, (unsigned)old_tag);
            printf("%d %d\n", (int)media, memcmp(before, check, sizeof(before)) == 0);
        } else {
            return EXIT_FAILURE;
        }
        fflush(stdout);
    }
    cs_storage_destroy(handles[0]);
    cs_storage_destroy(handles[1]);
    return EXIT_SUCCESS;
}
