#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "screen_effect.h"

_Static_assert(sizeof(void*)==8,"This receipt exercises LP64");
_Static_assert(sizeof(ScreenWork)==16 && sizeof(ScreenEffect)==56,"Record size");
_Static_assert(offsetof(ScreenWork,effect_address)==4 && sizeof(ScreenAddress)==4,"Guest key");
_Static_assert(offsetof(ScreenWork,scaled)==8 && offsetof(ScreenWork,level)==12 && offsetof(ScreenWork,target)==14,"Work offsets");
_Static_assert(offsetof(ScreenEffect,mode)==24 && offsetof(ScreenEffect,parameter)==26 && offsetof(ScreenEffect,flags)==34,"Mode fields");
_Static_assert(offsetof(ScreenEffect,center_x)==36 && offsetof(ScreenEffect,factors)==40 && offsetof(ScreenEffect,extent_x)==52,"Geometry fields");
_Static_assert(_Alignof(ScreenWork)==1 && _Alignof(ScreenEffect)==1,"Byte views");

struct Trace { u32 rows; u32 calls[3][10]; };
static struct Trace *log_result;
static ScreenWork *active;
static ScreenEffect *mapped;
static u32 mapkey,mapbytes,changed,lookups;

ScreenEffect *bfm_screen_effect_resolve(u32 address)
{
    lookups++;
    return address && address == mapkey && !(address & 1u) && mapbytes >= sizeof(*mapped) ? mapped : NULL;
}
static u32 read_le(u8 *p){return p[0] | (u32)p[1]<<8;}
static void write_le(u8 *p,u32 x){p[0]=(u8)x;p[1]=(u8)(x>>8);}
static void geometry(u32 addr,s32 x,s32 y,s32 w,s32 h,u32 r,u32 g,u32 b,u32 parameter,u32 flag)
{
    u32 row[10]={addr,x,y,w,h,r,g,b,parameter,flag};
    memcpy(log_result->calls[log_result->rows++],row,sizeof row);
    if (changed) {
        u8 *p=(u8 *)mapped;
        write_le(p+26,3*read_le(p+26)+7);
        write_le(p+36,read_le(p+36)^0xcc33);
        write_le(p+54,read_le(p+54)^0xa33a);
    }
}
void bfm_screen_observe_15f04(s32 x,s32 y,s32 w,s32 h,u32 a,u32 b,u32 c,u32 d,u32 e)
{geometry(0x80015f04u,x,y,w,h,a,b,c,d,e);}
void bfm_screen_observe_15d4c(s32 x,s32 y,s32 w,s32 h,u32 a,u32 b,u32 c,u32 d,u32 e)
{geometry(0x80015d4cu,x,y,w,h,a,b,c,d,e);}
void bfm_screen_observe_3aa24(ScreenEffect *a0,ScreenWork *a1,ScreenWork *a2,u32 a3)
{
    u32 row[10]={0x8013aa24u,mapkey,0x2000,0x2000,a3};
    assert(a0==mapped && a1==active && a2==active);
    memcpy(log_result->calls[log_result->rows++],row,sizeof row);
}
u32 native_probe_mapping(u8 *work,u8 *data,struct Trace *out,u32 mutate,u32 key,u32 length)
{
    active=(ScreenWork *)work;mapped=(ScreenEffect *)data;log_result=out;
    changed=mutate;mapkey=key;mapbytes=length;lookups=0;
    assert(!work || (uintptr_t)work>UINT32_MAX);
    assert(!data || (uintptr_t)data>UINT32_MAX);
    func_8013A530(active);
    return lookups;
}
void native_probe(u8 *work,u8 *data,struct Trace *out,u32 mutate)
{
    assert(native_probe_mapping(work,data,out,mutate,0x2200,56)==1);
}
