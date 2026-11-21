#include "musashi_boot_memory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MusashiBootMemory memory;

static void discard_console(void *userdata, const uint8_t *data, int32_t size) {
    (void)userdata;
    (void)data;
    (void)size;
}

static int map_image(const char *path) {
    const size_t size = 0x65000u;
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    uint8_t *image = malloc(size);
    int ok = image && fread(image, 1, size, file) == size && fgetc(file) == EOF && !ferror(file);
    if (fclose(file)) ok = 0;
    if (ok) ok = musashi_boot_map_exe(&memory, image, size);
    free(image);
    return ok;
}

int main(int argc, char **argv) {
    if (argc != 2 || !map_image(argv[1])) return EXIT_FAILURE;
    const uint32_t context = 0x8006bb34u;
    MusashiCallbackDevice callback = {0};
    MusashiStartupPrefixDevice devices = {.callback = &callback};
    MusashiResetGraphPrefixStop stop = {0};
    uint32_t original;
    if (!musashi_boot_write32(&memory, context, 0x80042780u) ||
        !musashi_boot_write32(&memory, context + 4u, 0x8006cb14u) ||
        !musashi_boot_write32(&memory, context + 8u, 0x8006cb40u) ||
        !musashi_boot_write32(&memory, context + 0x2cu, 0x80074750u)) return EXIT_FAILURE;
    if (musashi_boot_execute_source_card_vblank(&memory, &devices, context,
        discard_console, NULL, &stop) != MUSASHI_RESETGRAPH_PREFIX_COMPLETE) return EXIT_FAILURE;

    if (!musashi_boot_read32(&memory, 0x8006291cu, &original) ||
        !musashi_boot_write32(&memory, 0x8006291cu, original ^ 1u)) return EXIT_FAILURE;
    if (musashi_boot_execute_source_card_vblank(&memory, &devices, context,
        discard_console, NULL, &stop) == MUSASHI_RESETGRAPH_PREFIX_COMPLETE ||
        stop.target_address != 0x80062988u) return EXIT_FAILURE;
    if (!musashi_boot_write32(&memory, 0x8006291cu, original) ||
        !musashi_boot_write32(&memory, context + 4u, 0x801ff000u)) return EXIT_FAILURE;
    if (musashi_boot_execute_source_card_vblank(&memory, &devices, context,
        discard_console, NULL, &stop) == MUSASHI_RESETGRAPH_PREFIX_COMPLETE) return EXIT_FAILURE;
    puts("Owned card callback returned on the installed IRQ stack and refused a changed helper or stack.");
    return EXIT_SUCCESS;
}
