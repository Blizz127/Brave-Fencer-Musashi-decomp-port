#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "card_save_directory.h"
#include "card_image.h"
#include "../platform/bfm_plat_storage.h"
#include "../platform/bfm_plat_config.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
typedef wchar_t CardPathChar;
#define CARD_SEPARATOR '\\'

static int is_separator(int c)
{
    return c == '/' || c == '\\';
}

/* Follow directory links and open share roots without restricting other access. */
static int open_directory(const wchar_t *path)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE dir = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    int result;
    if (dir == INVALID_HANDLE_VALUE)
        return 0;
    result = GetFileInformationByHandle(dir, &info) &&
             (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    CloseHandle(dir);
    return result;
}

static int directory_exists(const wchar_t *path)
{
    DWORD attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return 0;
    return !(attrs & FILE_ATTRIBUTE_REPARSE_POINT) || open_directory(path);
}

static int make_directory(const wchar_t *path)
{
    return CreateDirectoryW(path, NULL) ||
           (GetLastError() == ERROR_ALREADY_EXISTS && directory_exists(path));
}

static int parent_missing(void)
{
    DWORD error = GetLastError();
    return error == ERROR_PATH_NOT_FOUND || error == ERROR_FILE_NOT_FOUND;
}

/* The server and share (or drive root) are supplied by the host. */
static size_t windows_root_length(const wchar_t *path)
{
    const wchar_t *p = path;
    const wchar_t *end;
    if (wcsncmp(p, L"\\\\?\\", 4) == 0) {
        p += 4;
        if (_wcsnicmp(p, L"UNC\\", 4) == 0)
            p += 4;
        else if (p[0] && p[1] == ':' && p[2] == '\\')
            return (size_t)(p - path) + 3;
        else
            return 0;
    } else if (wcsncmp(p, L"\\\\", 2) == 0) {
        p += 2;
        if (p[0] == '.' && p[1] == '\\')
            return 0;
    } else {
        return p[0] && p[1] == ':' && p[2] == '\\' ? 3 : 0;
    }
    end = wcschr(p, '\\');
    if (!end || p == end || !end[1])
        return 0;
    p = end + 1;
    end = wcschr(p, '\\');
    if (!end)
        end = p + wcslen(p);
    return p != end ? (size_t)(end - path) : 0;
}
#else
#include <errno.h>
#include <sys/stat.h>
typedef char CardPathChar;
#define CARD_SEPARATOR '/'

static int is_separator(char c)
{
    return c == '/';
}

static int directory_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int make_directory(const char *path)
{
    return mkdir(path, 0700) == 0 || (errno == EEXIST && directory_exists(path));
}
static int parent_missing(void)
{
    return errno == ENOENT;
}
#endif

/* Recurse only when a requested directory has a missing parent. */
static int create_parents(CardPathChar *path, size_t length, size_t root)
{
    size_t parent = length;
    int created;
    CardPathChar separator;

    if (make_directory(path))
        return 1;
    if (!parent_missing())
        return 0;
    while (parent > root && !is_separator(path[parent - 1]))
        --parent;
    while (parent > root && is_separator(path[parent - 1]))
        --parent;
    if (parent <= root)
        return 0;
    separator = path[parent];
    path[parent] = '\0';
    created = create_parents(path, parent, root);
    path[parent] = separator;
    return created && make_directory(path);
}

static int prepare_directory(const char *path, int create_missing)
{
    size_t root, length;
#ifdef _WIN32
    wchar_t input[BFM_PLAT_PATH_MAX], full[BFM_PLAT_PATH_MAX + 8];
    size_t i;
    DWORD n;
    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, input, BFM_PLAT_PATH_MAX))
        return 0;
    n = GetFullPathNameW(input, BFM_PLAT_PATH_MAX, full, NULL);
    if (!n || n >= BFM_PLAT_PATH_MAX)
        return 0;
    length = n;
    for (i = 0; i < length; ++i)
        if (full[i] == '/')
            full[i] = '\\';
    /* Legacy Win32 processes can require a namespace prefix for long names. */
    if (length >= MAX_PATH - 12 && wcsncmp(full, L"\\\\?\\", 4) != 0) {
        if (wcsncmp(full, L"\\\\", 2) == 0) {
            wmemmove(full + 8, full + 2, length - 1);
            wmemcpy(full, L"\\\\?\\UNC\\", 8);
            length += 6;
        } else {
            wmemmove(full + 4, full, length + 1);
            wmemcpy(full, L"\\\\?\\", 4);
            length += 4;
        }
    }
    root = windows_root_length(full);
    if (!root)
        return 0;
#else
    char full[BFM_PLAT_PATH_MAX];
    length = strlen(path);
    memcpy(full, path, length + 1);
    root = path[0] == '/' ? 1 : 0;
#endif
    while (length > root && is_separator(full[length - 1]))
        full[--length] = '\0';
#ifdef _WIN32
    if (length == root)
        return directory_exists(full) || open_directory(full);
#endif
    if (directory_exists(full))
        return 1;
    if (create_missing)
        return create_parents(full, length, root);
    /* A missing previous directory needs no migration. An inaccessible
     * directory, or a link to an offline one, can still own the player's save. */
#ifdef _WIN32
    return GetFileAttributesW(full) == INVALID_FILE_ATTRIBUTES && parent_missing();
#else
    struct stat entry;
    return lstat(full, &entry) != 0 && errno == ENOENT;
#endif
}

static int launcher_path(char *path, size_t capacity)
{
    const char *base = getenv("BFM_SAVE_DIR");
    const char *suffix = "";
    size_t base_length, suffix_length, separator;

    if (!path || !capacity)
        return 0;
    path[0] = '\0';
    if (!base || !*base) {
#ifdef _WIN32
        base = getenv("APPDATA");
        suffix = "bfm-port";
#else
        base = getenv("XDG_STATE_HOME");
        suffix = "bfm-port";
        if (!base || !*base) {
            base = getenv("HOME");
            suffix = ".local/state/bfm-port";
        }
#endif
    }
    if (!base || !*base)
        return 0;
    base_length = strlen(base);
    suffix_length = strlen(suffix);
    separator = suffix_length && !is_separator(base[base_length - 1]);
    if (base_length >= BFM_PLAT_PATH_MAX ||
        base_length + separator + suffix_length >= BFM_PLAT_PATH_MAX ||
        base_length + separator + suffix_length >= capacity)
        return 0;
    memcpy(path, base, base_length);
    if (separator)
        path[base_length++] = CARD_SEPARATOR;
    memcpy(path + base_length, suffix, suffix_length + 1);
    return 1;
}

/* Examine the last committed image without creating a card or taking the
 * game's write lock. The player subsequently attaches the selected slot. */
static int inspect_card(const char *directory, uint8_t *image, const uint8_t *fresh)
{
    int result = bfm_plat_memcard_set_dir(directory);
    if (result != BFM_PLAT_OK)
        return CARD_DIRECTORY_UNAVAILABLE;
    result = bfm_plat_memcard_load(0, image, CARD_IMAGE_SIZE);
    if (result == BFM_PLAT_NOT_FOUND)
        return CARD_DIRECTORY_READY;
    if (result != BFM_PLAT_OK)
        return CARD_DIRECTORY_UNAVAILABLE;
    return memcmp(image, fresh, CARD_IMAGE_SIZE) == 0 ? CARD_DIRECTORY_READY : CARD_DIRECTORY_EXISTING;
}

int musashi_card_save_directory(char *path, size_t capacity)
{
    BfmPlatConfig config;
    char ini[BFM_PLAT_PATH_MAX], previous[BFM_PLAT_PATH_MAX];
    const char *override = getenv("BFM_SAVE_DIR");
    uint8_t *images;
    int selection;

    if (!path || !capacity)
        return CARD_DIRECTORY_UNAVAILABLE;
    path[0] = '\0';
    if (override && *override)
        return launcher_path(path, capacity) && prepare_directory(path, 1);

    bfm_plat_config_defaults(&config);
    if (bfm_plat_config_default_path(ini, sizeof ini) != BFM_PLAT_OK ||
        bfm_plat_config_load_file(&config, ini) != BFM_PLAT_OK ||
        !config.save_dir[0] || strcmp(config.save_dir, "saves") == 0)
        return launcher_path(path, capacity) && prepare_directory(path, 1);
    if (strlen(config.save_dir) >= capacity)
        return CARD_DIRECTORY_UNAVAILABLE;
    strcpy(path, config.save_dir);
    if (!prepare_directory(path, 1))
        return CARD_DIRECTORY_UNAVAILABLE;

    images = malloc(2 * CARD_IMAGE_SIZE);
    if (!images)
        return CARD_DIRECTORY_UNAVAILABLE;
    card_format((CardImage){images, CARD_IMAGE_SIZE});
    selection = inspect_card(path, images + CARD_IMAGE_SIZE, images);
    /* A player may have saved to the old default while the INI was ignored.
     * Prefer that card only over a missing or exactly factory-fresh one. */
    if (selection == CARD_DIRECTORY_READY && launcher_path(previous, sizeof previous) &&
        strcmp(path, previous) != 0) {
        selection = prepare_directory(previous, 0)
            ? inspect_card(previous, images + CARD_IMAGE_SIZE, images) : CARD_DIRECTORY_UNAVAILABLE;
        if (selection == CARD_DIRECTORY_EXISTING) {
            if (strlen(previous) < capacity)
                strcpy(path, previous);
            else
                selection = CARD_DIRECTORY_UNAVAILABLE;
        }
    }
    free(images);
    return selection;
}
