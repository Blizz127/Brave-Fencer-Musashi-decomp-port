/* Linux filesystem regression. The external test driver waits for the original
   parent to exit before letting its surviving forked child reacquire. */
#include "card_host_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static uint8_t first[CS_IMAGE_BYTES], second[CS_IMAGE_BYTES], bytes[CS_IMAGE_BYTES];
static unsigned checks;

#define REQUIRE(expr) do {                                               \
    if (!(expr)) {                                                       \
        fprintf(stderr, "fork ownership line %d: %s\n", __LINE__, #expr); \
        _Exit(EXIT_FAILURE);                                             \
    }                                                                    \
    ++checks;                                                            \
} while (0)

static void verify_exclusive(CsStorage *active)
{
    int status;
    pid_t peer = fork();
    REQUIRE(peer >= 0);
    if (peer == 0) {
        CsStorage *competing = cs_storage_create();
        REQUIRE(competing != NULL);
        cs_storage_destroy(active);
        REQUIRE(cs_storage_attach(competing, 0) == BFM_PLAT_NOT_READY);
        cs_storage_destroy(competing);
        _Exit(EXIT_SUCCESS);
    }
    REQUIRE(waitpid(peer, &status, 0) == peer);
    REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    int pipefd[2];
    char token;
    pid_t child;
    CsStorage *inherited, *fresh;
    REQUIRE(argc == 3);
    REQUIRE(bfm_plat_memcard_set_dir(argv[1]) == BFM_PLAT_OK);
    inherited = cs_storage_create();
    REQUIRE(inherited != NULL);
    REQUIRE(cs_storage_attach(inherited, 0) == BFM_PLAT_OK);
    REQUIRE(cs_storage_commit(inherited, 0, NULL, first) == 1);
    memset(second, 0x39, sizeof(second));
    REQUIRE(pipe(pipefd) == 0);
    child = fork();
    REQUIRE(child >= 0);
    if (child > 0) {
        close(pipefd[1]);
        REQUIRE(read(pipefd[0], &token, 1) == 1 && token == 'R');
        close(pipefd[0]);
        REQUIRE(cs_storage_snapshot(inherited, 0, bytes) == CS_MEDIA_OK);
        REQUIRE(memcmp(bytes, first, sizeof(bytes)) == 0);
        REQUIRE(cs_storage_commit(inherited, 0, first, second) == 1);
        /* Leave the slot acquired. The test driver waits for this very
           process to exit before sending the surviving child one byte. */
        printf("parent %u\n", checks);
        fflush(stdout);
        _Exit(EXIT_SUCCESS);
    }
    close(pipefd[0]);
    checks = 0;
    fresh = cs_storage_create();
    REQUIRE(fresh != NULL);
    if (strcmp(argv[2], "inspect-before-exit") == 0) {
        REQUIRE(cs_storage_snapshot(inherited, 0, bytes) == CS_MEDIA_ABSENT);
        REQUIRE(cs_storage_commit(inherited, 0, first, second) == 0);
        REQUIRE(cs_storage_attach(inherited, 0) == BFM_PLAT_NOT_READY);
        REQUIRE(cs_storage_attach(fresh, 0) == BFM_PLAT_NOT_READY);
    }
    REQUIRE(write(pipefd[1], "R", 1) == 1);
    close(pipefd[1]);
    REQUIRE(getchar() == 'G');
    REQUIRE(cs_storage_attach(fresh, 0) == BFM_PLAT_OK);
    REQUIRE(cs_storage_snapshot(inherited, 0, bytes) == CS_MEDIA_ABSENT);
    cs_storage_detach(inherited, 0);
    cs_storage_destroy(inherited);
    REQUIRE(cs_storage_snapshot(fresh, 0, bytes) == CS_MEDIA_OK);
    REQUIRE(memcmp(bytes, second, sizeof(bytes)) == 0);
    verify_exclusive(fresh);
    REQUIRE(cs_storage_commit(fresh, 0, second, first) == 1);
    cs_storage_destroy(fresh);
    printf("child %u\n", checks);
    return EXIT_SUCCESS;
}
