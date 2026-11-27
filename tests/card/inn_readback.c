/* SPDX-License-Identifier: MIT
 * Linux fault-injection probe. From the project root:
 * cc -std=gnu11 -DBFM_MEMCARD_ONLY -ffunction-sections -Wl,--gc-sections
 *    -Ipreview -Ipc_port/card -Ipc_port/platform -Ipc_port/include
 *    tests/card/inn_readback.c preview/{inn_profile,inn_card,inn_menu,save_menu,gameplay_run}.c
 *    pc_port/card/{card_image,card_host_storage}.c pc_port/platform/bfm_plat_storage.c
 *    -Wl,--wrap=cs_storage_snapshot,--wrap=cs_storage_commit -o inn-readback
 * Pass a new private test directory to the resulting program.
 */
#include "card_host_storage.h"
#include "gameplay_run.h"
#include "inn_card.h"
#include "inn_menu.h"
#include "inn_profile.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef NDEBUG
#error "The fault-injection assertions must be enabled"
#endif

static int fail_read;
static unsigned writes;
static uint8_t prior[CS_IMAGE_BYTES], attempted[CS_IMAGE_BYTES], foreign[CS_IMAGE_BYTES];

CsMedia __real_cs_storage_snapshot(void *, unsigned, uint8_t *);
int __real_cs_storage_commit(void *, unsigned, const uint8_t *, const uint8_t *);

CsMedia __wrap_cs_storage_snapshot(void *storage, unsigned slot, uint8_t *data)
{
    return fail_read ? CS_MEDIA_ERROR : __real_cs_storage_snapshot(storage, slot, data);
}

int __wrap_cs_storage_commit(void *storage, unsigned slot, const uint8_t *old,
                             const uint8_t *data)
{
    ++writes;
    return __real_cs_storage_commit(storage, slot, old, data);
}

/* The synthetic seed already contains a game file and never needs disc art. */
int musashi_disc_media_read_root_range(const MusashiDiscMedia *disc, const char *path,
                                       size_t offset, uint8_t *out, size_t n, size_t *size)
{
    (void)disc; (void)path; (void)offset; (void)out; (void)n; (void)size;
    assert(0);
    return 0;
}

static void read_disk(uint8_t *image)
{
    assert(bfm_plat_memcard_load(0, image, CS_IMAGE_BYTES) == BFM_PLAT_OK);
}

static void replace_test_card(const uint8_t *image)
{
    assert(bfm_plat_memcard_save(0, image, CS_IMAGE_BYTES) == BFM_PLAT_OK);
}

static void uncertain(BfmInnProfile *profile, unsigned file, const BfmSaveRecord *record)
{
    read_disk(prior);
    fail_read = 1;
    unsigned before = writes;
    assert(bfm_inn_profile_save(profile, file, record, NULL));
    fail_read = 0;
    assert(writes == before + 1);
    read_disk(attempted);
    assert(memcmp(prior, attempted, sizeof prior) != 0);
}

static BfmSaveRecord make_seed(void)
{
    BfmSaveRecord record = {0};
    record.progress[8] = 10;
    record.progress[52] = 1;
    CardImage image = {prior, sizeof prior};
    const uint8_t header[] = {'S', 'C', 0, 1};
    const char *name = "BASLUS-00726MUSASHI";
    assert(card_format(image) == CARD_OK && card_create(image, name, 1) == CARD_OK);
    assert(card_write(image, name, 0, header, sizeof header) == CARD_OK);
    assert(!bfm_inn_card_encode(image, 1, &record));
    FILE *seed = fopen("seed.mcd", "wbx");
    assert(seed && fwrite(prior, 1, sizeof prior, seed) == sizeof prior && !fclose(seed));
    return record;
}

int main(int argc, char **argv)
{
    assert(argc == 2 && mkdir(argv[1], 0700) == 0 && chdir(argv[1]) == 0);
    BfmSaveRecord record = make_seed();
    const char *error;
    BfmInnProfile *profile = bfm_inn_profile_create("seed.mcd", "first", &error);
    assert(profile && !error);
    BfmSaveMenu menu;
    BfmInnMenu keeper;
    enum { DOWN = 1u << 6, RIGHT = 1u << 5, CROSS = 1u << 14 };
    assert(!bfm_inn_profile_menu(profile, &menu));
    bfm_inn_menu_begin(&keeper, &menu);
    bfm_inn_menu_input(&keeper, DOWN);
    bfm_inn_menu_input(&keeper, CROSS);
    bfm_inn_menu_input(&keeper, RIGHT);
    bfm_inn_menu_input(&keeper, CROSS);
    assert(bfm_inn_menu_input(&keeper, CROSS) == BFM_INN_WRITE && keeper.card.selected == 1);
    uncertain(profile, 2, &record);
    bfm_inn_menu_complete(&keeper, NULL, "Save verification failed");
    assert(keeper.page == BFM_INN_FILE_ERROR && keeper.notice);
    unsigned before = writes;
    fail_read = 1;
    bfm_inn_profile_menu(profile, &menu);
    assert(menu.issue && bfm_inn_profile_save(profile, 2, &record, NULL));
    fail_read = 0;
    bfm_inn_menu_input(&keeper, CROSS);
    bfm_inn_menu_input(&keeper, CROSS);
    assert(bfm_inn_menu_input(&keeper, CROSS) == BFM_INN_WRITE);
    assert(!bfm_inn_profile_save(profile, 2, &record, NULL) && writes == before);
    bfm_inn_profile_menu(profile, &menu);
    assert(!menu.issue && menu.selected == 1 && menu.rows[1].state == BFM_SAVE_VALID);
    bfm_inn_menu_complete(&keeper, &menu, NULL);
    assert(keeper.page == BFM_INN_SAVED && keeper.card.selected == 1);

    uncertain(profile, 3, &record);
    replace_test_card(prior);
    before = writes;
    assert(!bfm_inn_profile_save(profile, 3, &record, NULL) && writes == before + 1);
    read_disk(prior);
    assert(!memcmp(attempted, prior, sizeof prior));

    BfmGameplayRun run = {0};
    bfm_gameplay_remember_saved(&run, &record);
    ++record.progress[56];
    uncertain(profile, 4, &record);
    memcpy(foreign, attempted, sizeof foreign);
    foreign[sizeof foreign - 1] ^= 1;
    replace_test_card(foreign);
    before = writes;
    assert(!bfm_inn_profile_menu(profile, &menu));
    assert(menu.issue && bfm_inn_profile_save(profile, 4, &record, NULL) && writes == before);
    read_disk(prior);
    assert(!memcmp(foreign, prior, sizeof prior));
    replace_test_card(attempted);
    fail_read = 1;
    assert(!bfm_inn_profile_menu(profile, &menu));
    fail_read = 0;
    assert(run.saved_data.state.progress[56] != record.progress[56]);
    const BfmSaveRecord *recovered = bfm_inn_profile_menu(profile, &menu);
    assert(!menu.issue && menu.selected == 3 && menu.rows[3].state == BFM_SAVE_VALID);
    assert(recovered == &menu.records[3]);
    bfm_gameplay_remember_saved(&run, recovered);
    assert(!bfm_inn_profile_menu(profile, &menu));
    enum { ROOM = 0x80189d6c - 0x80128158 };
    static uint8_t scene[ROOM + sizeof record.destination] = {[ROOM] = 0x12, [ROOM + 1] = 0x30};
    BfmSaveRecord restored;
    assert(bfm_gameplay_woods_recovery(&run, BFM_CHECKPOINT_SAVED_DATA,
                                       scene, sizeof scene, &restored));
    BfmSaveRecord expected = record;
    memcpy(expected.destination, scene + ROOM, sizeof expected.destination);
    assert(!memcmp(&restored, &expected, sizeof restored));
    bfm_inn_profile_close(profile);

    profile = bfm_inn_profile_create("seed.mcd", "second", &error);
    assert(profile && !error);
    uncertain(profile, 2, &record);
    before = writes;
    assert(!bfm_inn_profile_save(profile, 3, &record, NULL) && writes == before + 1);
    bfm_inn_profile_menu(profile, &menu);
    assert(!menu.issue && menu.selected == 2 && menu.rows[1].state == BFM_SAVE_VALID &&
           menu.rows[2].state == BFM_SAVE_VALID);
    uncertain(profile, 4, &record);
    ++record.progress[56];
    before = writes;
    assert(bfm_inn_profile_save(profile, 4, &record, NULL) && writes == before);
    read_disk(prior);
    assert(!memcmp(prior, attempted, sizeof prior));
    bfm_inn_profile_close(profile);

    profile = bfm_inn_profile_create("seed.mcd", "overwrite", &error);
    assert(profile && !error);
    bfm_inn_profile_menu(profile, &menu);
    bfm_inn_menu_begin(&keeper, &menu);
    bfm_inn_menu_input(&keeper, DOWN);
    bfm_inn_menu_input(&keeper, CROSS);
    read_disk(prior);
    before = writes;
    assert(bfm_inn_menu_input(&keeper, CROSS) == BFM_INN_NONE);
    assert(keeper.page == BFM_INN_CONFIRM && keeper.overwriting);
    assert(bfm_inn_menu_input(&keeper, 1u << 12) == BFM_INN_NONE);
    assert(keeper.page == BFM_INN_FILES && writes == before);
    read_disk(attempted);
    assert(!memcmp(prior, attempted, sizeof prior));
    bfm_inn_menu_input(&keeper, CROSS);
    assert(bfm_inn_menu_input(&keeper, CROSS) == BFM_INN_WRITE);
    BfmSaveMenu stale = keeper.card;
    stale.records[0].variables[0] ^= 0x55;
    assert(bfm_inn_profile_save_selected(profile, &stale, &record, NULL) && writes == before);
    memcpy(foreign, prior, sizeof foreign);
    foreign[sizeof foreign - 1] ^= 1;
    replace_test_card(foreign);
    assert(bfm_inn_profile_save_selected(profile, &keeper.card, &record, NULL));
    read_disk(attempted);
    assert(!memcmp(attempted, foreign, sizeof foreign));
    replace_test_card(prior);
    assert(!bfm_inn_profile_save_selected(profile, &keeper.card, &record, NULL));
    bfm_inn_profile_menu(profile, &menu);
    assert(!menu.issue && !memcmp(&menu.records[0], &record, sizeof record));
    bfm_inn_menu_complete(&keeper, &menu, NULL);
    assert(keeper.page == BFM_INN_SAVED);
    before = writes;
    assert(bfm_inn_profile_save_selected(profile, &stale, &record, NULL) && writes == before);
    bfm_inn_profile_close(profile);

    profile = bfm_inn_profile_create("seed.mcd", "chapter", &error);
    assert(profile && !error);
    record.progress[8] = 2;
    record.flags[16] = 1;
    fail_read = 1;
    assert(bfm_chapter_profile_save(profile, 2, &record, NULL));
    fail_read = 0;
    before = writes;
    assert(!bfm_chapter_profile_save(profile, 2, &record, NULL) && writes == before);
    bfm_inn_profile_menu(profile, &menu);
    assert(!menu.issue && menu.selected == 1 && menu.rows[1].chapter == 1);
    bfm_inn_profile_close(profile);
    puts("Verified Hotelo error/retry, picker reopen, restored prior, foreign card, new slot, changed record and chapter retry.");
    return 0;
}
