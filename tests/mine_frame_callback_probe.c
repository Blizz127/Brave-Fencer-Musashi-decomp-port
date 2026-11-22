#include "mine_frame_callback.h"

#include <assert.h>
#include <string.h>

u8 D_80126B58[16];
u32 D_801A0D90[27];
u16 D_800B99DA;
u8 D_800B9A64;
extern void func_801287B8(void);

static u32 table_address, table_target, first_pending, second_pending;
static u32 selected_record, selected_pending;
static unsigned index_reads, pending_reads, parent_steps, option_calls;
static int fail_read, allow_call = 1, parent_mode;
static u8 *expected_record;

int bfm_mine_frame_callback_word(u32 address, u32 *value)
{
    if (address == BFM_MINE_PENDING_CALLBACK) {
        ++pending_reads;
        *value = pending_reads == 1 ? first_pending : second_pending;
        return fail_read != (int)pending_reads;
    }
    assert(address == table_address);
    assert(!parent_mode || parent_steps == 15);
    ++index_reads;
    *value = table_target;
    return !fail_read;
}

int bfm_mine_record_callback(u32 target, u8 *record)
{
    assert(target == table_target && record == expected_record);
    selected_record = target;
    if (parent_mode) {
        first_pending = target;
        second_pending = target + 12;
        record[9] = 81;
    }
    return allow_call;
}

int bfm_mine_pending_callback(u32 target)
{
    assert(target == second_pending);
    selected_pending = target;
    return allow_call;
}

static void put_index(u8 *p, u32 index)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        p[0x68+i] = (u8)(index >> (8*i));
}

static void clear(void)
{
    table_target = 0x18340;
    first_pending = second_pending = 0;
    selected_record = selected_pending = 0;
    index_reads = pending_reads = parent_steps = option_calls = 0;
    allow_call = 1;
    fail_read = parent_mode = 0;
}

/* The remaining frame services check the position of each callback in the frame. */
#define STEP(name) void name(void) { ++parent_steps; }
STEP(func_80129CF8)
STEP(func_8017849C)
STEP(func_80165E90)
STEP(func_801627E8)
STEP(func_80162B1C)
STEP(func_80165CA0)
STEP(func_80129010)
STEP(func_8013CA14)
STEP(func_800190AC)
STEP(func_8012956C)
STEP(func_8016E95C)
STEP(func_801754A8)

void func_8014FDF4(u8 *p) { assert(p == D_80126B58); ++parent_steps; }
s32 func_801505FC(void *p) { assert(p == D_80126B58); ++parent_steps; return 5; }
s32 func_801508B4(void *p) { assert(p == D_80126B58); ++parent_steps; return -1; }

void func_801379FC(void)
{
    assert(parent_steps == 15 && pending_reads >= 1);
    ++parent_steps;
}

void func_8001212C(void)
{
    assert(D_800B99DA == 0);
    ++option_calls;
}

int main(void)
{
    u8 backing[109];
    u8 *record = backing + 1;
    u32 indices[] = {0, 1, 17, 0x7fffffffu, 0xabcd1986u, 0x40000000u, 0xffffffffu};
    unsigned i;
    memset(backing,0,sizeof(backing));
    for (i = 0; i < sizeof(indices)/sizeof(*indices); ++i) {
        clear();
        expected_record = record;
        table_address = BFM_MINE_RECORD_CALLBACK_TABLE + 4u*indices[i];
        put_index(record, indices[i]);
        assert(bfm_mine_frame_record_step(record,108));
        assert(index_reads == 1 && selected_record == table_target);
    }
    clear();
    assert(!bfm_mine_frame_record_step(0,108));
    assert(!bfm_mine_frame_record_step(record,107) && !index_reads);
    put_index(record,0);table_address=BFM_MINE_RECORD_CALLBACK_TABLE;
    fail_read=1;assert(!bfm_mine_frame_record_step(record,108) && !selected_record);
    fail_read=0;table_target=0;assert(!bfm_mine_frame_record_step(record,108));
    table_target=0x127a;assert(!bfm_mine_frame_record_step(record,108) && !selected_record);
    table_target=0x1234;allow_call=0;assert(!bfm_mine_frame_record_step(record,108));

    clear();assert(bfm_mine_frame_pending_step() && pending_reads == 1 && !selected_pending);
    clear();first_pending=0x8874;second_pending=0x1198;
    assert(bfm_mine_frame_pending_step() && pending_reads==2 && selected_pending==second_pending);
    clear();fail_read=1;assert(!bfm_mine_frame_pending_step() && pending_reads==1);
    clear();first_pending=4;fail_read=2;assert(!bfm_mine_frame_pending_step() && pending_reads==2);
    clear();first_pending=4;second_pending=2;assert(!bfm_mine_frame_pending_step() && !selected_pending);
    clear();first_pending=4;assert(!bfm_mine_frame_pending_step() && !selected_pending);
    clear();first_pending=4;second_pending=8;allow_call=0;
    assert(!bfm_mine_frame_pending_step() && selected_pending==8);
    for (i=0;i<2;++i) {
        clear();parent_mode=1;expected_record=(u8 *)D_801A0D90;
        D_801A0D90[0]=i;D_800B99DA=65535;D_800B9A64=1;
        put_index(expected_record,9);table_address=BFM_MINE_RECORD_CALLBACK_TABLE+36;
        expected_record[9]=0;
        func_801287B8();
        assert(option_calls==1 && parent_steps==16);
        assert(index_reads==(i==0) && expected_record[9]==(i ? 0 : 81));
        assert(pending_reads==(i ? 1u : 2u) && selected_pending==(i ? 0u : table_target+12));
    }
    return 0;
}
