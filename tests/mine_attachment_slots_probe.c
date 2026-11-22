#include "mine_attachment_slots.h"

#include <assert.h>
#include <string.h>

s16 D_8011DB2C;
MineAttachedSlot D_8011DB30[128];
u32 D_80126AEC;

static const u32 pool = 0x20000u, position = 0x25000u;
static const u32 primary_table = 0x901c0u, secondary_table = 0x33008u;
static u8 coordinates[16], low[8];
static unsigned lookups, calls, stores, mutate, fail_at;
static u32 seen_slots[128], seen_entry[128], store_addr[384];
static u16 store_value[384];
static u32 returned_handler;

static void half(u8 *p, u16 v)
{
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}

static void word(u8 *p, u32 v)
{
    half(p, (u16)v);
    half(p + 2, (u16)(v >> 16));
}

static u16 read_half(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}

static void slot(unsigned i, u16 type, u32 object)
{
    u8 *p = D_8011DB30[i].bytes;
    half(p, type);
    half(p + 6, (u16)(0x1122u + i));
    half(p + 10, (u16)(0xbc30u + 2*i));
    half(p + 14, (u16)(0x7300u - i));
    word(p + 32, object);
}

int bfm_mine_attachment_handler_at(u32 address, u32 *handler)
{
    assert(lookups < 128);
    seen_entry[lookups++] = address;
    *handler = returned_handler;
    return fail_at != 1;
}

int bfm_mine_run_attachment_handler(u32 handler, MineAttachedSlot *p)
{
    unsigned which = (unsigned)(p-D_8011DB30);
    assert(handler == returned_handler && calls < 128 && which < 128);
    seen_slots[calls++] = which;
    if (fail_at == 2) return 0;
    if (mutate && calls == 1) {
        D_8011DB2C = 0;
        D_80126AEC = secondary_table;
        slot(127,9,position);
        slot(which,0,position);
        half(p->bytes + 6,0x9328u);
    }
    return 1;
}

int bfm_mine_store_attachment_half(u32 address, u16 value)
{
    u8 *p;
    assert(stores < 384);
    store_addr[stores] = address;
    store_value[stores++] = value;
    if (fail_at == 3 && stores == 2) return 0;
    if (address >= pool && address - pool <= sizeof(D_8011DB30)-2)
        p = (u8 *)D_8011DB30 + (address-pool);
    else if (address >= position && address - position <= sizeof(coordinates)-2)
        p = coordinates + (address-position);
    else if (address <= sizeof(low)-2)
        p = low + address;
    else
        return 0;
    half(p,value);
    return 1;
}

static void reset(void)
{
    memset(D_8011DB30,0,sizeof(D_8011DB30));
    memset(coordinates,0,sizeof(coordinates));
    memset(low,0,sizeof(low));
    D_8011DB2C = -7;
    D_80126AEC = primary_table;
    lookups = calls = stores = mutate = fail_at = 0;
    returned_handler = 0x14370;
}

int main(void)
{
    unsigned i, skip;
    for (skip=0;skip<129;++skip) {
        reset();
        for (i=0;i<128;++i)
            slot(i,(i == skip || skip == 128) ? (u16)(65535u-i) : 0, position);
        assert(bfm_mine_update_attachment_slots());
        assert(calls == (skip == 128 ? 128u : 1u) && lookups == calls && stores == calls*3u);
        for (i=0;i<calls;++i) {
            unsigned n = skip == 128 ? i : skip;
            assert(seen_slots[i] == n && seen_entry[i] == primary_table + (65535u-n)*4u);
            assert(store_value[3*i] == (u16)(0x1122u+n));
            assert(store_value[3*i+1] == (u16)(0xbc30u+2*n));
            assert(store_value[3*i+2] == (u16)(0x7300u-n));
        }
    }
    reset();
    slot(3,2,0);D_8011DB2C=0;func_80129010();
    assert(!calls && !stores);
    D_8011DB2C=1;mutate=1;func_80129010();
    assert(calls==2 && seen_slots[0]==3 && seen_slots[1]==127);
    assert(seen_entry[0]==primary_table+8 && seen_entry[1]==secondary_table+36);
    assert(store_value[0]==0x9328u && stores==6);

    /* Each halfword write can replace the destination of the next write. */
    reset();slot(0,3,pool+24);
    half(D_8011DB30[0].bytes+6,(u16)(pool+22));
    half(D_8011DB30[0].bytes+10,(u16)position);
    assert(bfm_mine_update_attachment_slots());
    assert(stores==3 && store_addr[0]==pool+32 && store_addr[1]==pool+32);
    assert(store_addr[2]==position+12 && read_half(coordinates+12)==0x7300);
    reset();slot(0,3,pool+2);
    assert(bfm_mine_update_attachment_slots());
    assert(store_value[0]==0x1122 && store_value[1]==0x1122);
    reset();slot(0,1,0xfffffff8u);D_80126AEC=0xfffffffcu;
    assert(bfm_mine_update_attachment_slots());
    assert(seen_entry[0]==0 && stores==3 && store_addr[0]==0 && store_addr[2]==4);
    reset();slot(0,1,0);
    assert(bfm_mine_update_attachment_slots() && calls==1 && !stores);
    for (i=1;i<=3;++i) {
        reset();slot(0,1,position);fail_at=i;
        assert(!bfm_mine_update_attachment_slots());
        assert(lookups==1 && calls==(i>=2) && stores==(i==3 ? 2u : 0u));
        if (i==3) assert(read_half(coordinates+8)==0x1122 && !read_half(coordinates+10));
    }
    reset();slot(0,1,position);D_80126AEC|=2;
    assert(!bfm_mine_update_attachment_slots() && !lookups);
    reset();slot(0,1,position);returned_handler=0;
    assert(!bfm_mine_update_attachment_slots() && lookups==1 && !calls);
    reset();slot(0,1,position);returned_handler|=2;
    assert(!bfm_mine_update_attachment_slots() && lookups==1 && !calls);
    reset();slot(0,1,position+1);
    assert(!bfm_mine_update_attachment_slots() && calls==1 && !stores);
    return 0;
}
