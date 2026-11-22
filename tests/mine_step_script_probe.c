/* Compare generated duration scripts, signed output and exact read/write ordering. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include "mine_step_script.h"

u32 D_8019FA30;
u8 D_8019FA34, D_8019FA35;
s32 D_801269D4, D_801269C8;
static u32 base, entry, rng = 0x286a51e9u;
static unsigned reads, fault_after;
static u8 sample, duration;
static s32 before_a, before_b;
static jmp_buf caught;
static u32 next(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}
static s32 signed_byte(u8 b) { return b < 128 ? b : (s32)b - 256; }
u8 bfm_mine_script_read(u32 address)
{
    if (!reads) {
        assert(address==entry && D_801269D4==before_a && D_801269C8==before_b);
        reads++;
        if (fault_after==1) longjmp(caught,1);
        return sample;
    }
    assert(reads==1 && address==(u32)(entry+1));
    assert(D_801269D4==signed_byte(sample) && D_801269C8==signed_byte(sample));
    reads++;
    if (fault_after==2) longjmp(caught,2);
    return duration;
}
static void reset(u32 address,u8 index,u8 time)
{
    D_8019FA30=base=address; D_8019FA34=index; D_8019FA35=time;
    entry=base+(u32)index*2;reads=fault_after=0;
    D_801269D4=before_a=(s32)next(); D_801269C8=before_b=(s32)next();
}
static void refusal(unsigned after)
{
    reset(0xfffffffdu,1,0); sample=(u8)-37; duration=3; fault_after=after;
    if (!setjmp(caught)) { func_8013CA14(); assert(0); }
    assert(reads==after && D_8019FA30==base && D_8019FA34==1 && !D_8019FA35);
    assert(D_801269D4==(after==1 ? before_a : -37));
    assert(D_801269C8==(after==1 ? before_b : -37));
}
int main(void)
{
    for(unsigned i=0;i<16384;i++) {
        u8 index=(u8)next();u8 time=(i&3)==1 ? (u8)(1+next()%255) : 0;
        u32 address=(i&3)==0 ? 0 : (i%5)==0 ? 0xffffffc0u : (next()|1u);
        sample=(u8)next();duration=(i%7)==0 ? 0 : (u8)(1+next()%255);
        reset(address,index,time);
        func_8013CA14();
        if (!address || time) {
            assert(reads==0 && D_8019FA30==base && D_8019FA34==index);
            assert(D_8019FA35==(!address ? time : time-1));
            assert(D_801269D4==before_a && D_801269C8==before_b);
        } else {
            assert(reads==2 && D_8019FA35==duration);
            assert(D_8019FA30==(duration ? base : 0));
            assert(D_8019FA34==(duration ? (u8)(index+1) : 0));
            assert(D_801269D4==(duration ? signed_byte(sample):0));
            assert(D_801269C8==D_801269D4);
        }
    }
    refusal(1);refusal(2);
    puts("16384 script states, unsigned address wrap, counter and signed output checks, both partial-read failures");
}
