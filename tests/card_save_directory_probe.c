#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "../pc_port/card/card_image.h"
#include "../pc_port/card/card_save_directory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int set(const char *key, const char *value)
{
#ifdef _WIN32
    return _putenv_s(key, value);
#else
    return *value ? setenv(key, value, 1) : unsetenv(key);
#endif
}

static int make_card(const char *path, const char *entry)
{
    static uint8_t bytes[CARD_IMAGE_SIZE];
    CardImage image = {bytes, sizeof(bytes)};
    FILE *file;
    size_t written;
    if (card_format(image) != CARD_OK ||
        (entry && card_create(image, entry, 1) != CARD_OK))
        return 1;
    file = fopen(path, "wb");
    if (!file)
        return 1;
    written = fwrite(bytes, 1, sizeof(bytes), file);
    return fclose(file) != 0 || written != sizeof(bytes);
}

/* Supplying the environment explicitly also tests absent APPDATA on Windows. */
int main(int argc, char **argv)
{
    static const char *const keys[] = {"BFM_SAVE_DIR", "XDG_STATE_HOME", "HOME", "APPDATA"};
    char path[600];
    size_t capacity;
    int i, result;
    if ((argc == 3 || argc == 4) && strcmp(argv[1], "--make-card") == 0)
        return make_card(argv[2], argc == 4 ? argv[3] : NULL);
    if (argc != 6 && argc != 7)
        return 2;
    if (set("BFM_PORT_CONFIG", argc == 7 ? argv[6] : "") || set("XDG_CONFIG_HOME", ""))
        return 2;
    for (i = 0; i < 4; ++i)
        if (set(keys[i], argv[i + 2]) != 0)
            return 2;
    capacity = (size_t)strtoul(argv[1], NULL, 10);
    if (capacity > sizeof(path))
        return 2;
    memset(path, '!', sizeof(path));
    result = musashi_card_save_directory(path, capacity);
    if (musashi_card_save_directory(NULL, capacity) ||
        (capacity && !memchr(path, 0, capacity)) ||
        (capacity < sizeof(path) && path[capacity] != '!'))
        return 3;
    printf("%d\n%s\n", result, capacity ? path : "");
    return 0;
}
