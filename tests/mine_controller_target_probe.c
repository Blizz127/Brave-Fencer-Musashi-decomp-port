#include "mine_controller_target.h"
#include "mine_target_draw.h"

#include <assert.h>
#include <string.h>

u8 D_80126B58[0x50];
u8 D_801821E4[20], D_801821F8[28];
u16 D_800B99DA;
static u8 first_record[38], second_record[38];
static char trace[64];
static unsigned calls, target_reads, pass, first_policy, second_policy;
static s16 *from_address, *to_address;

enum { first = 0x18400, second = 0xab004 };
static const u32 allow_first = 1u << 28, allow_second = 1u << 29;

static void put(u8 *p, unsigned off, u32 word)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        p[off+i] = (u8)(word >> (8*i));
}

static void mark(char c)
{
    assert(calls < sizeof(trace)-1);
    trace[calls++] = c;
    trace[calls] = 0;
}

const u8 *bfm_mine_controller_target_view(u32 address, unsigned size)
{
    ++target_reads;
    assert(size == 36 && (address & 3u) == 0);
    if (address == first) return first_record+1;
    if (address == second) return second_record+1;
    return 0;
}

static void make_pair(s16 *a, s16 *b, unsigned which)
{
    unsigned i;
    assert(a && b && a != b);
    if (from_address) assert(from_address == a && to_address == b);
    from_address = a;
    to_address = b;
    pass = which;
    for (i = 0; i < 4; ++i) {
        a[i] = (s16)(i+3*which);
        b[i] = (s16)-(int)(i+5*which);
    }
    mark(which == 1 ? 'A' : 'B');
}

void bfm_mine_controller_first_points(s16 *a, s16 *b)
{
    make_pair(a,b,1);
}

void bfm_mine_controller_second_points(s16 *a, s16 *b)
{
    make_pair(a,b,2);
}

void bfm_mine_controller_transform_points(u8 *ctl, s16 *a, s16 *b)
{
    assert(ctl == D_80126B58 && a == from_address && b == to_address);
    assert(a[0] == (s16)(3*pass) && b[3] == -(s16)(3+5*pass));
    a[1] = -32766;
    b[1] = 32764;
    mark('T');
}

void bfm_mine_controller_first_draw(u8 *ctl, s16 *a, s16 *b)
{
    assert(pass == 1 && ctl == D_80126B58 && a == from_address && b == to_address);
    assert(a[1] == -32766 && b[1] == 32764);
    mark('F');
    if (first_policy) {
        put(ctl,0x44,allow_second);
        put(ctl,0x20,first_policy == 3 ? 0x44444u : second);
        if (first_policy == 2)
            put(second_record+1,4,0x80000000u);
    }
    if (second_policy) put(ctl,0x44,0);
}

/* The second draw uses its original C. These are its two opaque boundaries. */
void func_80168070(u8 *ctl,s16 *a,s16 *b,void *matrix)
{
    unsigned i;
    u32 *m=matrix;
    assert(pass == 2 && ctl == D_80126B58 && a == from_address && b == to_address);
    assert(a[1] == -32766 && b[1] == 32764);
    for (i=0;i<8;++i) m[i]=0x34501u+i;
    mark('M');
}

void func_80017758(void *prim,void *matrix)
{
    const TargetPrimitive *p=prim;
    assert(((u32 *)matrix)[7] == 0x34508u && p->tag == 0x50000000u);
    mark('P');
}

static void setup(u32 flags,u32 target,u32 state,u32 active)
{
    memset(D_80126B58,0,sizeof(D_80126B58));
    memset(first_record,0,sizeof(first_record));
    memset(second_record,0,sizeof(second_record));
    put(D_80126B58,0x44,flags);
    put(D_80126B58,0x20,target);
    put(first_record+1,4,state);
    put(first_record+1,0x20,active);
    put(second_record+1,0x20,31);
    trace[0]=0;
    calls=target_reads=pass=first_policy=second_policy=0;
    from_address=to_address=0;
}

int main(void)
{
    unsigned bits, state, active, enabled_first, enabled_second;
    const char *b_only="BTMPPPPPPPPPP";
    for(bits=0;bits<32;++bits) for(state=0;state<2;++state) for(active=0;active<2;++active) {
        u32 flags=0x101u;
        char expected[32]="";
        if(bits&1) flags|=allow_first;
        if(bits&2) flags|=allow_second;
        if(bits&4) flags|=1u<<10;
        if(bits&8) flags|=1u<<17;
        if(bits&16) flags|=1u<<31;
        setup(flags,first,state ? 0x80000003u : 0x7fffffffu,active ? 0x80000000u : 0);
        enabled_first=(bits&5u)==1;
        enabled_second=(bits&14u)==2;
        if(!state && active && enabled_first) strcat(expected,"ATF");
        if(!state && active && enabled_second) strcat(expected,b_only);
        assert(bfm_mine_controller_update());
        assert(!strcmp(trace,expected) && target_reads==enabled_first+enabled_second);
    }
    setup(allow_first,first,0,4); first_policy=1; func_80165E90();
    assert(!strcmp(trace,"ATFBTMPPPPPPPPPP") && target_reads==2);
    setup(allow_first,first,0,4); first_policy=2; func_80165E90();
    assert(!strcmp(trace,"ATF") && target_reads==2);
    setup(allow_first|allow_second,first,0,4);second_policy=1;func_80165E90();
    assert(!strcmp(trace,"ATF") && target_reads==1);
    setup(allow_first,first,0,4);first_policy=3;
    assert(!bfm_mine_controller_update() && !strcmp(trace,"ATF") && target_reads==2);
    {
        u32 bad[]={0,first+1u,0xffffffe0u,0xfffffffcu,0xabc0u,0xffffffdcu};
        unsigned i;
        for(i=0;i<sizeof(bad)/sizeof(*bad);++i) {
            setup(allow_second,bad[i],0,0);
            assert(!bfm_mine_controller_update() && calls==0);
            assert(target_reads==(i>=4));
        }
    }
    return 0;
}
