/* Synthetic guest callback tables exercise dispatch and the state visible at each call. */
#include "forest_player_state.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(void *)==8, "exercise the 64-bit host interface");
_Static_assert(offsetof(ForestPlayerState,flags)==0x44 && offsetof(ForestPlayerState,state)==0x4c &&
               offsetof(ForestPlayerState,phase)==0x4d && offsetof(ForestPlayerState,mode)==0xdd,
               "guest player state fields");

enum { STATE, PHASE, UPDATE };
static const u32 tables[]={0x8018860c,0x801891ac,0x8018925c};
/* Private test-only token namespace. No host function pointer is used as a game address. */
static const u32 targets[]={0xa1000000,0xb2000000,0xc3000000};
/* Uppercase means a table-word read, lowercase means its target ran.
   The numbers identify the four common calls in their required order. */
static const char reads[] = "SPK", calls[] = "spk";
static const char *const chains[]={"SsPp1234Kk","Pp1234Kk","1234Kk"};
typedef struct {
    u16 lookup_kind,final_kind,action;
    u8 phase,mode;
    char stop;
    int zero_target;
} Plan;
ForestPlayerState D_80126B58;
static ForestPlayerState other, expected, baseline_global, baseline_other;
static ForestPlayerState *active;
static Plan plan;
static const char *chain;
static size_t progress;
static u32 destination;
static unsigned role,completed,refused,missing_tables,missing_targets,missing_common;
static jmp_buf cancel;

static void observe(char step,ForestPlayerState *actor)
{
    assert(chain[progress]==step && actor==active);
    assert(memcmp(active,&expected,sizeof(expected))==0);
    ++progress;
}
static void change_state(ForestPlayerState *p,unsigned step)
{
    p->kind=(u16)(p->kind^((0x111u+step)*3));
    p->phase=(u8)(p->phase+17+step);
    p->flags^=0x83054011u>>step;
    p->pad48[step%4]=(u8)(p->pad48[step%4]^0x5a);
    if (step==1) p->phase=plan.phase;
    if (step==5) p->kind=plan.lookup_kind;
}
static void apply_change(unsigned step)
{
    change_state(active, step);
    change_state(&expected, step);
}
static void common(ForestPlayerState *p,char name,unsigned step)
{
    observe(name,p);
    if (plan.stop == name) {
        ++missing_common;
        longjmp(cancel, 1);
    }
    apply_change(step);
}
void func_80150B28(ForestPlayerState *p) {common(p,'1',2);}
void func_80158638(ForestPlayerState *p) {common(p,'2',3);}
void func_8015126C(ForestPlayerState *p) {common(p,'3',4);}
void func_8014900C(ForestPlayerState *p) {common(p,'4',5);}

u32 bfm_forest_player_table_word(u32 table,u32 index)
{
    unsigned r = 0;
    while (r < 3 && tables[r] != table)
        ++r;
    assert(r<3);
    observe(reads[r],active);
    if (plan.stop == reads[r]) {
        ++missing_tables;
        longjmp(cancel, 1);
    }
    u32 want=r==STATE?expected.state:r==PHASE?expected.phase:((u32)expected.kind&0x7fff);
    assert(index==want);
    role=r;
    destination=targets[r]+4*index;
    /* Change the consumed selector after the 32-bit word has been read. */
    if (r==STATE) {active->state^=0xfe;expected.state^=0xfe;}
    else if (r==PHASE) {active->phase^=0xf3;expected.phase^=0xf3;}
    else {active->kind^=0x5555;expected.kind^=0x5555;}
    if (plan.stop==calls[r]) return plan.zero_target?0:0xee002310u;
    return destination;
}
static void final_callback(ForestPlayerState *p)
{
    p->flags^=0x08d91bd1u;
    p->kind = plan.final_kind;
    p->action = plan.action;
    p->mode = plan.mode;
}
void bfm_forest_player_call(u32 address,ForestPlayerState *p)
{
    assert(role<3);
    unsigned r=role;
    observe(calls[r],p);
    if (address!=destination) {
        assert(plan.stop==calls[r] && address==(plan.zero_target?0:0xee002310u));
        ++missing_targets;longjmp(cancel,1);
    }
    assert(plan.stop!=calls[r]);
    if (r==STATE) {
        apply_change(1);
        func_80159968(p);
    } else if (r==PHASE) {
        apply_change(0);
        func_80159A20(p);
    } else {
        final_callback(p);
        final_callback(&expected);
    }
}
static int flag_cleared(void)
{
    static const u16 always_clear[]={2,3,11,12};
    for (unsigned i=0;i<sizeof(always_clear)/sizeof(*always_clear);++i)
        if (plan.final_kind==always_clear[i]) return 1;
    return plan.final_kind==25 && (plan.mode==4 || (plan.mode==1 && plan.action==3));
}
static void exercise(unsigned entry,unsigned alternate,Plan configuration)
{
    assert(entry!=STATE || !alternate);
    unsigned sequence=completed+refused;
    chain = chains[entry];
    plan = configuration;
    progress = 0;
    memset(&D_80126B58, 0x81, sizeof(D_80126B58));
    memset(&other, 0x76, sizeof(other));
    active=alternate?&other:&D_80126B58;
    active->state=(u8)(37*sequence);
    active->phase=(u8)(63*sequence+9);
    active->kind=(u16)(plan.lookup_kind^0xbac1);
    active->mode=(u8)~plan.mode;
    active->action=(u16)(plan.action^0x1111);
    active->flags=0x10229bf3u+(u32)sequence*0x693cb075u;
    memcpy(&expected, active, sizeof(expected));
    memcpy(&baseline_global, &D_80126B58, sizeof(baseline_global));
    memcpy(&baseline_other, &other, sizeof(baseline_other));
    if (setjmp(cancel)==0) {
        if (entry==STATE) func_80146360();
        else if (entry==PHASE) func_80159968(active);
        else func_80159A20(active);
        assert(!plan.stop && chain[progress]=='\0');
        const u32 flag=1u<<27;
        expected.flags=(expected.flags&~flag)|(flag_cleared()?0:flag);
        assert(memcmp(active,&expected,sizeof(expected))==0);
        ++completed;
    } else {
        assert(plan.stop && chain[progress-1]==plan.stop);
        assert(memcmp(active,&expected,sizeof(expected))==0);
        /* The mocked owner discards earlier effects at a missing provider. */
        memcpy(&D_80126B58, &baseline_global, sizeof(baseline_global));
        memcpy(&other, &baseline_other, sizeof(baseline_other));
        assert(memcmp(&D_80126B58,&baseline_global,sizeof(expected))==0 &&
               memcmp(&other,&baseline_other,sizeof(expected))==0);
        ++refused;
    }
    if (alternate) assert(memcmp(&D_80126B58,&baseline_global,sizeof(expected))==0);
    else assert(memcmp(&other,&baseline_other,sizeof(expected))==0);
}

int main(void)
{
    const u16 actions[]={0,2,3,4,0x103,0xffff};
    for (unsigned entry=0;entry<3;++entry) {
        for (unsigned alternate=0;alternate<=(entry!=STATE);++alternate) {
            for (unsigned kind=0;kind<=26;++kind) {
                for (unsigned high=0;high<=1;++high) {
                    for (unsigned edge=0;edge<6;++edge) {
                        static const u8 modes[]={0,1,1,4,4,255};
                        u16 current=(u16)(kind+(high<<15));
                        Plan cfg={(u16)(current^0x8000),current,actions[edge],(u8)(255-kind),modes[edge],0,0};
                        exercise(entry,alternate,cfg);
                    }
                }
            }
            for (unsigned mode=0;mode<256;++mode) {
                for (unsigned j=0;j<6;++j) {
                    Plan cfg={0xffff,25,actions[j],(u8)mode,(u8)mode,0,0};
                    exercise(entry,alternate,cfg);
                }
            }
            Plan max={0x7fff,0xffff,3,255,4,0,0};
            exercise(entry,alternate,max);
            for (const char *p=chains[entry];*p;++p) {
                for (int zero=0;zero<=((*p=='s')||(*p=='p')||(*p=='k'));++zero) {
                    Plan fail={0x8002,3,3,255,4,*p,zero};
                    exercise(entry,alternate,fail);
                }
            }
        }
    }
    printf("completed=%u refused=%u table_misses=%u target_misses=%u common_misses=%u\n",
           completed,refused,missing_tables,missing_targets,missing_common);
    return 0;
}
