#include "mine_actor_fade.h"

#include <assert.h>
#include <string.h>

MineVisualActor D_801269F0[3];
u32 D_80127524;
s32 D_80127548;
ScreenWork D_8011DA80[2];

static MineVisualActor outside;
static ScreenEffect effect;
static u32 updates[8], draws[5];
static unsigned updated, drawn, started, finished, resolved, observed, mutations;
static const u32 external = 0x80314500u;
static const u32 alias = 0xa01269f0u;

static u32 slot(unsigned i) { return BFM_MINE_VISUAL_ACTORS_BASE + 0x4cu*i; }

static void put(u8 *p, u32 v)
{
    p[0] = (u8)v;
    p[1] = (u8)(v>>8);
    p[2] = (u8)(v>>16);
    p[3] = (u8)(v>>24);
}

static void object(MineVisualActor *a, unsigned active, unsigned kind, unsigned flags)
{
    memset(a,0,sizeof(*a));
    put(a->bytes,active);
    a->bytes[4] = (u8)kind;
    a->bytes[5] = (u8)(kind>>8);
    put(a->bytes+8,flags);
}

MineVisualActor *bfm_mine_external_visual_actor(u32 address)
{
    if (address == external) return &outside;
    if (address == alias) return &D_801269F0[0];
    return 0;
}

static MineVisualActor *check_address(u32 address)
{
    unsigned i;
    for (i=0;i<3;++i)
        if (address==slot(i)) return &D_801269F0[i];
    return bfm_mine_external_visual_actor(address);
}

void bfm_mine_update_visual_actor(u32 address, MineVisualActor *actor)
{
    assert(D_80127548==36 && updated<8 && drawn==0);
    assert(actor==check_address(address));
    updates[updated++] = address;
    if (mutations && updated==1) {
        D_80127524=slot(2);
        D_801269F0[1].bytes[0]=0;
        D_801269F0[2].bytes[0]=1;
    }
}

void bfm_mine_draw_visual_actor(u32 address, MineVisualActor *actor)
{
    assert(D_80127548==36 && drawn<3);
    assert(actor==check_address(address));
    draws[drawn++] = address;
    if (mutations && drawn==1)
        D_801269F0[2].bytes[9]=0x30;
}

void bfm_mine_fade_start(ScreenWork *which)
{
    assert(which==D_8011DA80 && started++==0);
    if (mutations) D_8011DA80[1].reserved00[0]=2;
}

void bfm_mine_fade_finish(ScreenWork *which)
{
    assert(which==D_8011DA80+1 && finished++==0);
}

ScreenEffect *bfm_screen_effect_resolve(u32 address)
{
    assert(address==0xe704u);
    ++resolved;
    return &effect;
}

void bfm_screen_observe_3aa24(ScreenEffect *a0, ScreenWork *a1, ScreenWork *a2, u32 a3)
{
    assert(a0==&effect && a1==D_8011DA80+1 && a2==a1);
    (void)a3;
    ++observed;
}

void bfm_screen_observe_15f04(s32 a,s32 b,s32 c,s32 d,u32 e,u32 f,u32 g,u32 h,u32 i)
{
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;
    assert(0);
}

void bfm_screen_observe_15d4c(s32 a,s32 b,s32 c,s32 d,u32 e,u32 f,u32 g,u32 h,u32 i)
{
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;
    assert(0);
}

static void reset(void)
{
    unsigned i;
    memset(D_8011DA80,0,sizeof(D_8011DA80));
    memset(&effect,0,sizeof(effect));
    for(i=0;i<3;++i)
        object(&D_801269F0[i],1,0x8000,0x1000);
    object(&outside,1,1,0);
    D_80127524=0;
    D_80127548=-7;
    D_8011DA80[0].reserved00[0]=1;
    D_8011DA80[1].reserved00[0]=3;
    put(D_8011DA80[1].effect_address.le,0xe704u);
    D_8011DA80[1].target.le[1]=8;
    effect.mode.le[0]=2;
    updated=drawn=started=finished=resolved=observed=mutations=0;
}

int main(void)
{
    unsigned mask, drawmask, choice, i, n, e;
    u32 order[8];
    for(mask=0;mask<8;++mask) for(drawmask=0;drawmask<8;++drawmask) for(choice=0;choice<6;++choice) {
        reset();
        for(i=0;i<3;++i)
            object(&D_801269F0[i],(mask>>i)&1,0x8000,(drawmask>>i)&1 ? 0x1000 : 0x3000);
        D_80127524=choice==0 ? 0 : choice<4 ? slot(choice-1) : choice==4 ? external : alias;
        n=0;
        if (choice==4 || (choice==5 && (mask&1)) || (choice>=1 && choice<=3 && (mask&(1u<<(choice-1)))))
            order[n++]=D_80127524;
        for(i=0;i<3;++i)
            if ((mask&(1u<<i)) && (n==0 || order[0]!=slot(i))) order[n++]=slot(i);
        assert(bfm_mine_actor_fade_update());
        assert(updated==n && !memcmp(updates,order,n*sizeof(order[0])));
        for(i=e=0;i<3;++i) if ((mask&drawmask)&(1u<<i)) assert(draws[e++]==slot(i));
        assert(drawn==e && started==1 && finished==1);
    }
    reset();D_80127524=slot(0);mutations=1;func_801379FC();
    assert(updated==2 && updates[0]==slot(0) && updates[1]==slot(0));
    assert(drawn==1 && draws[0]==slot(0));
    assert(started==1 && !finished && resolved==1 && observed==1);
    assert(D_8011DA80[1].level.le[1]==4);
    reset();D_80127524=external;outside.bytes[0]=0;mutations=1;func_801379FC();
    assert(updated==2 && updates[0]==slot(0) && updates[1]==slot(2));
    reset();D_80127524=0x17u;
    assert(!bfm_mine_actor_fade_update() && D_80127548==36 && !updated && !started);
    reset();D_80127524=0xffffffb8u;
    assert(!bfm_mine_actor_fade_update() && !updated && !started);
    reset();D_80127524=0x9a34u;
    assert(!bfm_mine_actor_fade_update() && !updated && !started);
    return 0;
}
