#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "steam_knight_hit.h"

extern void func_8018850C(SteamKnightPart *);
enum { INITIAL_PARENT=0x560, REFRESH_PARENT=0x980, FINAL_PARENT=0x710, MODEL=0x200 };
u8 D_801E44E0[16];
u32 D_801E44EC;
static SteamKnightPart core,initial,ready;
static u8 model[96];
static uint32_t refreshed;
static unsigned stage,alternate;
static u16 get_half(const u8 *p) { u16 value;memcpy(&value,p,2);return value; }
static void half(u8 *p,u16 value) { memcpy(p,&value,2); }
SteamKnightPart *bfm_steam_knight_break_actor(u32 value)
{
    switch(value) {
    case INITIAL_PARENT:return &initial;
    case REFRESH_PARENT:return &ready;
    default:abort();
    }
}
u8 *bfm_steam_knight_core_model(u32 value) { assert(value==MODEL);return model; }
void func_8013240C(void *effect)
{
    assert(stage++==0 && effect==D_801E44E0);
    assert(get_half(model+0x2c)==0x8a61);
    if(alternate) core.body_address=REFRESH_PARENT;
    D_801E44EC=refreshed;
}
void func_80183FCC(SteamKnightPart *part)
{
    assert(stage++==1 && part==&core);
    assert((alternate?ready.phase_flags:initial.phase_flags)==0x80000012u);
    core.reaction=0xc7;
    core.hit_kind=0x8465;
    D_801E44EC=0;
}
void func_8012AD44(u8 *actor,u16 state)
{
    assert(stage++==2 && actor==(u8 *)&core && state==2);
    assert(core.reaction==0 && core.hit_kind==0);
    core.body_address=alternate?0xa0000710u:FINAL_PARENT;
    core.display_record=0x60554433;
}
void bfm_steam_knight_core_attach(SteamKnightPart *part,u32 parent,int mode)
{
    assert(stage++==3 && part==&core && parent==(alternate?0xa0000710u:FINAL_PARENT) && mode==21);
    assert(core.display_record==0xc0196338u);
}
static void run(uint32_t old,uint32_t next,unsigned change,unsigned open)
{
    uint32_t token=MODEL;
    memset(&core,0x75,sizeof core);memset(model,0x39,sizeof model);
    memcpy((u8 *)&core+0x20,&token,4);
    core.body_address=INITIAL_PARENT;core.display_record=0xf177549a;
    initial.phase_flags=ready.phase_flags=open?0x80000012u:0x80000010u;
    half(model+0x2c,0x8a60);
    stage=0;alternate=change;
    D_801E44EC=old;refreshed=next;
    func_8018850C(&core);
    assert(stage==((next&0x4000u)?4u:1u));
    if(!(next&0x4000u)) {
        assert(initial.phase_flags==(open?0x80000012u:0x80000010u) && ready.phase_flags==initial.phase_flags);
        assert(core.reaction==0x75 && core.hit_kind==0x7575 && core.display_record==0xf177549a);
    }
}
int main(void)
{
    assert((uintptr_t)&core>UINT32_MAX && (uintptr_t)model>UINT32_MAX);
    for(unsigned bit=0;bit<32;bit++)
        for(unsigned alternate_parent=0;alternate_parent<2;alternate_parent++)
            for(unsigned existing=0;existing<2;existing++) {
                uint32_t v=UINT32_C(1)<<bit;
                run(~v,v,alternate_parent,existing);
                run(v,~v,alternate_parent,existing);
            }
    return 0;
}
