/* Independent processes exercise one card through the public storage API. */
#include "bfm_plat_storage.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

static int fail_replace;
static uint8_t card[BFM_MEMCARD_SIZE];

int __real_renameat(int from, const char *oldpath, int to, const char *newpath);

int __wrap_renameat(int from, const char *oldpath, int to, const char *newpath) {
    if (fail_replace) {
        errno = EIO;
        return -1;
    }
    return __real_renameat(from, oldpath, to, newpath);
}

static int save_byte(uint8_t value) {
    memset(card, value, sizeof card);
    return bfm_plat_memcard_save(0, card, sizeof card);
}

int main(int argc, char **argv) {
    CHECK(argc == 3 || argc == 4);
    const char *mode = argv[1];
    CHECK(bfm_plat_memcard_set_dir(argv[2]) == BFM_PLAT_OK);

    if (strcmp(mode, "contend") == 0) {
        CHECK(bfm_plat_memcard_acquire(0) == BFM_PLAT_NOT_READY);
        CHECK(save_byte(0x24) == BFM_PLAT_NOT_READY);
        /* The other slot in this directory is independently usable. */
        CHECK(bfm_plat_memcard_acquire(1) == BFM_PLAT_OK);
        bfm_plat_memcard_release(1);
        return 0;
    }

    CHECK(bfm_plat_memcard_acquire(0) == BFM_PLAT_OK);
    if (strcmp(mode, "fork-writer") == 0) {
        int status;
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0)
            _exit(save_byte(0x24) == BFM_PLAT_NOT_READY ? 0 : 1);
        CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
        bfm_plat_memcard_release(0);
        return 0;
    }
    if (strcmp(mode, "exit-with-child") == 0) {
        pid_t child = fork();
        CHECK(child >= 0);
        if (child != 0) _exit(0);
        /* Stay alive, without using the card API, past the owner's exit. */
        puts("ready");
        fflush(stdout);
        (void)getchar();
        puts("done");
        return 0;
    }
    if (strcmp(mode, "relocate") == 0) {
        CHECK(argc == 4);
        puts("ready");
        fflush(stdout);
        CHECK(getchar() != EOF && chdir(argv[3]) == 0);
        CHECK(bfm_plat_memcard_load(0, card, sizeof card) == BFM_PLAT_OK);
        for (size_t i = 0; i < sizeof card; ++i) CHECK(card[i] == 0x42);
        CHECK(save_byte(0x7a) == BFM_PLAT_OK);
    } else if (strcmp(mode, "hold") == 0) {
        CHECK(bfm_plat_memcard_load(0, card, sizeof card) == BFM_PLAT_NOT_FOUND);
        CHECK(save_byte(0x42) == BFM_PLAT_OK);
        CHECK(bfm_plat_memcard_set_dir(".") == BFM_PLAT_NOT_READY);
        puts("ready");
        fflush(stdout);
        (void)getchar();
    } else {
        CHECK(bfm_plat_memcard_load(0, card, sizeof card) == BFM_PLAT_OK);
        for (size_t i = 0; i < sizeof card; ++i) CHECK(card[i] == 0x42);
        if (strcmp(mode, "short-write") == 0) {
            struct rlimit limit;
            CHECK(signal(SIGXFSZ, SIG_IGN) != SIG_ERR);
            CHECK(getrlimit(RLIMIT_FSIZE, &limit) == 0);
            limit.rlim_cur = 4096;
            CHECK(setrlimit(RLIMIT_FSIZE, &limit) == 0);
            CHECK(save_byte(0x24) == BFM_PLAT_ERROR);
        } else if (strcmp(mode, "failed-rename") == 0) {
            fail_replace = 1;
            CHECK(save_byte(0x24) == BFM_PLAT_ERROR);
        } else {
            CHECK(strcmp(mode, "save") == 0);
            CHECK(save_byte(0x7a) == BFM_PLAT_OK);
        }
    }
    bfm_plat_memcard_release(0);
    return 0;
}
