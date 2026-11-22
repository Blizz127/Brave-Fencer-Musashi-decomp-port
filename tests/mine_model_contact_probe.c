#include "floor_cell_map.h"
#include "model_surface_probe.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct Obj8014FE60 {
    u8 pad0[6]; s16 x; u8 pad8[2]; s16 y; u8 padC[2]; s16 z;
    u8 pad10[0x78-0x10]; s16 hx, hy, hz; u16 flags; SceneProbeVector norm;
};
_Static_assert(sizeof(SceneSurfaceBox)==16,"box width");
_Static_assert(offsetof(SceneSurfaceRecord,matrix)==52 && sizeof(SceneSurfaceRecord)==84,"model matrix");
_Static_assert(sizeof(FloorCellMap)==32 && offsetof(FloorCellMap,lists)==20,"six guest keys");
_Static_assert(offsetof(Obj8014A2E4,probe_object_address)==32 && offsetof(Obj8014A2E4,probe_geometry_reference)==88,"pool fields");
Obj8014A2E4 D_801202A0[96];
SceneProbeVector D_801152A8;
SceneSurfaceBox D_8019F9EC;
u8 D_8019F9CC[32];
u32 D_8017F68C, D_8017F690, D_8017F694, D_8019F9B8;
u16 D_8019F9C8;
enum {
    START=0x9000, END=0x9100, HIT=0x9200, WORLD=0x10000, MESH=0x10100,
    CELL=0x13000, LIST=0x14000, WORLD_REC=0x15000, MESH_REC=0x16000, PLANES=0x17200, TBL=0x18000,
    BAD_GRID=0x10200, SHORT_GRID=0x10300, RECORD=0x342200, BAD_RECORD=0x343200,
    FIRST_BOX=0x80000120U, NEXT_BOX=0x80000240U, FIRST_TAG=0xc0000120U, MISSING_TAG=0xd0000ae0U
};
static SceneProbeVector from,to,contact;
static SceneSurfaceRecord model;
static SceneSurfaceBox first_box,next_box;
static FloorCellMap world_map, mesh_map;
static u16 cells[2],elements[1];
static s16 ground_rec[9],mesh_rec[9];
static FloorPlane planes[1];
static s32 table[1];
static u32 tag,record_key;
static int classify_result,world_hit,mesh_hit,box_hit,first_hit_height;
static int record_lookups,grid_lookups,box_lookups,classifies,box_tests,polygon_tests,world_tests,transforms;
static char events[64];static size_t length;
static void note(char c) {assert(length<sizeof events-1);events[length++]=c;}
static void *host(void *p) {assert((uintptr_t)p>UINT32_MAX);return p;}
/* Each field here corresponds to an independently bounded synthetic allocation. */
SceneSurfaceRecord *bfm_model_probe_record(u32 address)
{
    record_lookups++;note('r');
    return address==RECORD?host(&model):NULL;
}
SceneSurfaceBox *bfm_model_probe_box(u32 address)
{
    box_lookups++;note('b');
    if (address==FIRST_BOX) return host(&first_box);
    if (address==NEXT_BOX) return host(&next_box);
    return NULL;
}
FloorCellMap *bfm_model_probe_grid(u32 address)
{
    grid_lookups++;note('g');
    /* SHORT_GRID denotes a deliberate 4-byte region; CELL is the wrong kind. */
    switch(address) {
    case MESH: return host(&mesh_map);
    case SHORT_GRID: case CELL: case BAD_GRID: return NULL;
    default: assert(0 && "unexpected model grid key"); return NULL;
    }
}
void *bfm_floor_address(u32 address)
{
    switch(address) {
    case START:return host(&from); case END:return host(&to); case HIT:return host(&contact);
    case WORLD:return host(&world_map); case CELL:return host(cells); case LIST:return host(elements);
    case WORLD_REC:return host(ground_rec); case MESH_REC:return host(mesh_rec);
    case PLANES:return host(planes); case TBL:return host(table);
    default:assert(0 && "unbacked synthetic geometry address");return NULL;
    }
}
s32 func_80135480(SceneSurfaceRecord *m,u32 geometry,SceneProbeVector *begin,SceneProbeVector *end)
{
    classifies++;note('c');
    assert(m==&model && geometry==tag && (uintptr_t)begin>UINT32_MAX && (uintptr_t)end>UINT32_MAX && begin!=end);
    assert(begin->x==0 && begin->y==48 && begin->z==0 && end->x==0 && end->y==816 && end->z==0);
    from.x=begin->x;from.y=begin->y;from.z=begin->z;
    return classify_result;
}
s32 func_80135EB0(SceneSurfaceBox *box,s32 flag)
{
    box_tests++;note('v');
    assert(flag==0 && (box==&first_box || box==&next_box || box==&D_8019F9EC));
    return (box_hit==1 && box==&first_box) || (box_hit==2 && box==&next_box) || (box_hit==3 && box==&D_8019F9EC);
}
s32 func_801347A0(s16 flag,s16 *rec,FloorPlane *p,s32 *t)
{
    assert(flag==0 && p==planes && t==table);
    if (rec==mesh_rec) {
        note('p');polygon_tests++;
        return mesh_hit;
    }
    assert(rec==ground_rec);note('w');world_tests++;
    if (world_hit) {contact.y=40;from.pad=-2210;}
    return world_hit;
}
void func_80136A94(s32 rotate,SceneSurfaceRecord *m,SceneProbeVector *probe,void *matrix)
{
    transforms++;note('x');
    assert(m==&model && (uintptr_t)probe>UINT32_MAX);
    assert(rotate==(classify_result==2 || classify_result==4));
    assert(matrix==(classify_result==4 ? D_8019F9CC : model.matrix));
    probe->x=-814;probe->y=(s16)first_hit_height;probe->z=721;probe->pad=9;
    D_801152A8=(SceneProbeVector){6,8,5,0};
}
static void reset(int mode)
{
    memset(D_801202A0,0,sizeof D_801202A0);
    memset(events,0,sizeof events);length=0;
    D_801152A8=(SceneProbeVector){3,1,4,0};
    D_8017F68C=START;D_8017F690=END;D_8017F694=HIT;D_8019F9B8=WORLD;D_8019F9C8=19;
    world_map=(FloorCellMap){256,256,1,1,CELL,TBL,PLANES,LIST,WORLD_REC,0};
    mesh_map=world_map;mesh_map.plain=MESH_REC;
    cells[0]=0;cells[1]=1;elements[0]=0;
    ground_rec[0]=41;mesh_rec[0]=74;
    first_box.next=NEXT_BOX;next_box.next=0;D_8019F9EC.next=NEXT_BOX;
    tag=MESH;record_key=RECORD;classify_result=1;mesh_hit=1;box_hit=0;world_hit=1;
    first_hit_height=14;
    switch(mode) {
    case 0:break;
    case 1:first_hit_height=38;classify_result=2;break;
    case 2:mesh_hit=0;break;
    case 3:mesh_hit=0;world_hit=0;break;
    case 4:tag=FIRST_TAG;box_hit=1;break;
    case 5:tag=FIRST_TAG;box_hit=2;classify_result=2;break;
    case 6:tag=FIRST_TAG;box_hit=3;classify_result=3;break;
    case 7:tag=FIRST_TAG;box_hit=2;classify_result=4;break;
    case 8:tag=MISSING_TAG;break;
    case 9:tag=FIRST_TAG;first_box.next=0;break;
    case 10:tag=SHORT_GRID;break;
    case 11:tag=CELL;break;
    case 12:tag=BAD_GRID;break;
    case 13:record_key=BAD_RECORD;break;
    case 14:classify_result=0;break;
    case 15:classify_result=99;break;
    case 16:tag=FIRST_TAG;first_box.next=0x80001ec0;break;
    case 17:world_hit=0;classify_result=4;break;
    default:assert(0);
    }
    D_801202A0[95].active=1;D_801202A0[95].flags=0x400;
    D_801202A0[95].probe_object_address=record_key;
    D_801202A0[95].probe_geometry_reference=tag;
    record_lookups=grid_lookups=box_lookups=classifies=box_tests=polygon_tests=world_tests=transforms=0;
}
int main(void)
{
    const char *order[]={
        "rcgpxw","rcgpxw","rcgpw","rcgpw","rcbvxw","rcbvbvxw","rcvxw","rcvbvxw",
        "rcbw","rcbvw","rcgw","rcgw","rcgw","rw","rcw","rcw","rcbvbw","rcgpxw"
    };
    for (int path=0;path<2;path++) {
        for (int mode=0;mode<18;mode++) {
            reset(mode);
            Obj8014FE60 actor={0};actor.y=64;actor.flags=0xa5;actor.norm.x=89;
            int result=path?func_80150170((u8 *)&actor):func_8014FE60(&actor);
            assert(strcmp(events,order[mode])==0);
            assert(record_lookups==1 && world_tests==1 && classifies==(record_key==RECORD));
            int transformed=strchr(order[mode],'x')!=NULL;
            assert(transforms==transformed && result==(transformed||world_hit));
            if (!result) {
                assert(actor.hy==32767 && actor.flags==0xa5 && actor.norm.x==89);
            } else if (transformed && (!world_hit || first_hit_height<38)) {
                assert(actor.hx==-814 && actor.hy==first_hit_height && actor.hz==721 && actor.norm.x==6 && actor.flags==0xa4);
            } else {
                assert(actor.hx==0 && actor.hy==38 && actor.hz==0 && actor.norm.x==(transformed?6:3) && actor.flags==0xa4);
            }
        }
    }
    puts("36 complete nine-body SC02/0039 paths with the model-surface scanner and real typed map");
    return 0;
}
