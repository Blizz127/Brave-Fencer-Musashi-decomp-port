#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
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

/* Supplying the environment explicitly also tests absent APPDATA on Windows. */
int main(int argc, char **argv)
{
    static const char *const keys[] = {"BFM_SAVE_DIR", "XDG_STATE_HOME", "HOME", "APPDATA"};
    char path[600];
    size_t capacity;
    int i, result;
    if (argc != 6)
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
