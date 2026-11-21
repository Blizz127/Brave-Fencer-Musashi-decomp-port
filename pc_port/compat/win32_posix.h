#ifndef MUSASHI_WIN32_POSIX_H
#define MUSASHI_WIN32_POSIX_H

/* Windows only (force-included for C by CMakeLists.txt under WIN32): the
 * few POSIX calls the port makes, on the UCRT. The headers that declare the
 * UCRT spellings come first, so the mkdir macro cannot rewrite their own
 * one-argument declaration. */
#ifdef _WIN32
#include <assert.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

static inline int musashi_win32_setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value) == 0 ? 0 : -1;
}
#define setenv musashi_win32_setenv

/* Permissions do not apply; the directory is created with the default ACL. */
#define mkdir(path, mode) ((void)(mode), _mkdir(path))

static inline struct tm *musashi_win32_localtime_r(const time_t *t, struct tm *out) {
    return localtime_s(out, t) == 0 ? out : NULL;
}
#define localtime_r musashi_win32_localtime_r
#endif

#endif
