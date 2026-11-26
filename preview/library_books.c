/* SPDX-License-Identifier: MIT */
#include "library_books.h"
#include "bedroom_score.h"
#include <stdlib.h>
enum { PANELS = 3, MAX_ROWS = 6 };
static const uint32_t base = UINT32_C(0x80128158);
struct BfmLibraryBooks {
    const uint8_t *scene, *panels[PANELS];
    size_t bytes;
    uint8_t font[2][256];
    unsigned depth, row[PANELS], repeat, ink, warning;
    int held;
};
static unsigned half(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static uint32_t word(const uint8_t *p) { return half(p) | (uint32_t)half(p+2) << 16; }
static const uint8_t *data(const BfmLibraryBooks *b, uint32_t address, size_t count)
{
    if (address < base || count > b->bytes || address-base > b->bytes-count) return NULL;
    return b->scene + (address-base);
}
static unsigned count(const BfmLibraryBooks *b, unsigned panel)
{
    if (panel != 2) return b->panels[panel][2];
    const uint8_t *shape = data(b, 0x8019ed0cu + 2*b->row[1], 2);
    return shape && shape[1] == 1 && shape[0] <= MAX_ROWS ? shape[0] : 0;
}
BfmLibraryBooks *bfm_library_books_open(const MusashiDiscMedia *disc,const uint8_t *scene,size_t bytes)
{
    if (!disc || !scene) return NULL;
    BfmLibraryBooks *b = calloc(1,sizeof *b);
    if (!b) return NULL;
    b->scene=scene;
    b->bytes=bytes;
    const uint8_t *root=data(b,0x8019ee94u,4);
    const uint8_t *panels=root ? data(b,word(root),32*PANELS) : NULL;
    if (!panels || !bfm_bedroom_resident_data(disc,0x800636d8u,b->font[0],sizeof b->font[0]) ||
        !bfm_bedroom_resident_data(disc,0x80063818u,b->font[1],sizeof b->font[1])) goto invalid;
    for (unsigned i=0; i<PANELS; ++i) {
        const uint8_t *p=b->panels[i]=panels+32*i;
        if (half(p)!=(i==0 ? 3u : i+6u) || p[3]!=1 || !p[2] || p[2]>MAX_ROWS ||
            half(p+12)!=8 || half(p+14)!=2 || !data(b,word(p+4),96) ||
            !data(b,word(p+8),32) || !data(b,word(p+20),2) ||
            !data(b,word(p+24),4*p[2]) || !data(b,word(p+28),4*p[2])) goto invalid;
    }
    return b;
invalid:
    free(b);
    return NULL;
}
void bfm_library_books_free(BfmLibraryBooks *b) { free(b); }
int bfm_library_books_input(BfmLibraryBooks *b,int direction,int held,int cross,int back,
                            int available,unsigned *effect)
{
    if (b->ink<6) ++b->ink;
    if (b->warning) {
        if (back || cross) b->warning=b->ink=0;
        return BFM_BOOKS_ACTIVE;
    }
    if (cross) {
        if (!b->depth && b->row[0]) {
            *effect=0x474;
            return BFM_BOOKS_QUIT;
        }
        if (b->depth==2) {
            *effect=0x45d;
            return BFM_BOOKS_SELECTED;
        }
        if (b->depth==1 && b->row[1]>=3 && !available) {
            b->warning=1;
            b->ink=0;
        } else {
            ++b->depth;
            b->row[b->depth]=0;
            *effect=0x45d;
        }
    } else if (back && b->depth) {
        --b->depth;
        *effect=0x473;
    } else {
        if (held && b->held==held && !direction && b->repeat && !--b->repeat) {
            direction=held;
            b->repeat=3;
        } else if (direction || held!=b->held) b->repeat=8;
        b->held=held;
        if (direction) {
            unsigned total=count(b,b->depth), prior=b->row[b->depth];
            if (!total) return BFM_BOOKS_ACTIVE;
            if (direction<0 && prior) --b->row[b->depth];
            else if (direction>0 && prior+1<total) ++b->row[b->depth];
            else if (!b->depth) b->row[0]=direction<0 ? total-1 : 0;
            if (prior!=b->row[b->depth]) {
                if (b->depth==1) b->row[2]=0;
                *effect=0x45a;
            }
        }
    }
    return BFM_BOOKS_ACTIVE;
}

static unsigned expand(unsigned five) { return five*255/31; }
static void sprite(const uint16_t tiles[512][1024],uint32_t pixels[240][320],int x,int y,
                   int w,int h,int u,int v,int uw,int vh,int active)
{
    if (w<=0 || h<=0 || w>320 || h>240 || u<0 || v<0 || u+uw>256 || v+vh>256) return;
    for (int j=0;j<h;++j) for (int i=0;i<w;++i) {
        if ((unsigned)(x+i)>=320 || (unsigned)(y+j)>=240) continue;
        int tu=u+i*uw/w, tv=v+j*vh/h;
        unsigned packed=tiles[256+tv][320+tu/4], idx=(packed>>(4*(tu&3)))&15u;
        unsigned c=tiles[257][352+idx];
        if (!c) continue;
        unsigned rgb=0xff000000;
        for (unsigned ch=0;ch<3;++ch) {
            unsigned intensity=(c>>(5*ch))&31u;
            if (!active) intensity=intensity*88/128;
            rgb|=expand(intensity)<<(16-8*ch);
        }
        pixels[y+j][x+i]=rgb;
    }
}
static int label(const BfmLibraryBooks *b,uint32_t address,const uint16_t tiles[512][1024],
                 uint32_t pixels[240][320],int x,int y,int active)
{
    struct Glyph {int x,u,v;} run[48];
    unsigned used=0;
    for (;;) {
        const uint8_t *p=data(b,address,1);
        if (!p) return 0;
        if (!*p) break;
        p=data(b,address,2);
        if (!p || used==48 || (*p!=0x81 && *p!=0x82)) return 0;
        unsigned group=*p==0x81;
        unsigned first=group ? 0x40 : 0x4f;
        if (p[1]<first || p[1]-first>=128) return 0;
        const uint8_t *uv=b->font[group]+2*(p[1]-first);
        if (uv[0]==113 && uv[1]==56) x+=4;
        else { run[used++]=(struct Glyph){x,uv[0],uv[1]}; x+=7; }
        address+=2;
    }
    while (used) {
        struct Glyph g=run[--used];
        sprite(tiles,pixels,g.x,y,8,8,g.u,g.v,8,8,active);
    }
    return 1;
}
static void fill(uint32_t pixels[240][320],const uint8_t *record,int active)
{
    static const int dither[4][4]={{-4,0,-3,1},{2,-2,3,-1},{-3,1,-4,0},{3,-1,2,-2}};
    int x=(int16_t)half(record+8)+160, y=(int16_t)half(record+10)+120;
    int w=half(record+12), h=half(record+14);
    if (w<=0 || h<=0 || w>320 || h>240) return;
    for (int j=0;j<h;++j) for (int i=0;i<w;++i) {
        if ((unsigned)(x+i)>=320 || (unsigned)(y+j)>=240) continue;
        uint32_t rgb=0xff000000u, before=pixels[y+j][x+i];
        for (int ch=0;ch<3;++ch) {
            int slope=ch==2 ? 40 : 0;
            int numerator=record[ch]*(w*h)-slope*i*h+(record[ch+4]-record[ch]+slope)*j*w;
            int level = (numerator + w*h/2) / (w*h);
            unsigned shift = (unsigned)(16 - 8*ch);
            /* The library blends the inactive pane against the existing
             * framebuffer before the final dither and five-bit write. */
            if (!active) level = (level + ((int)(before >> shift) & 248))/2;
            level += dither[(y+j)&3][(x+i)&3];
            int five = (level < 0 ? 0 : level > 255 ? 255 : level) >> 3;
            rgb|=expand((unsigned)five)<<shift;
        }
        pixels[y+j][x+i]=rgb;
    }
}
static int panel(const BfmLibraryBooks *b,unsigned index,const uint16_t tiles[512][1024],
                 uint32_t pixels[240][320])
{
    const uint8_t *p=b->panels[index];
    int active=index==b->depth;
    const uint8_t *frames=data(b,word(p+4),96), *fills=data(b,word(p+8),32);
    fill(pixels,fills+16,active);
    if (active) fill(pixels,fills,1);
    for (int i=0;i<8;++i) {
        const uint8_t *f=frames+12*i;
        sprite(tiles,pixels,160+(int16_t)half(f),120+(int16_t)half(f+2),half(f+4),half(f+6),
               half(f+8),half(f+10),(i==4 || i==5)?7:8,i>=6?7:8,active);
    }
    if (!label(b,word(p+20),tiles,pixels,160+(int16_t)half(p+16),120+(int16_t)half(p+18),active)) return 0;
    unsigned rows=count(b,index);
    uint32_t names=word(p+28);
    if (index==2) {
        const uint8_t *lookup=data(b,0x8019ed18u+4*b->row[1],4);
        if (!lookup) return 0;
        names=word(lookup);
    }
    const uint8_t *labels=data(b,names,4*rows), *places=data(b,word(p+24),4*rows);
    if (!rows || !labels || !places) return 0;
    for (unsigned i=0;i<rows;++i) {
        int x=160+(int16_t)half(places+4*i),y=120+(int16_t)half(places+4*i+2);
        if (!label(b,word(labels+4*i),tiles,pixels,x,y,active)) return 0;
        if (i==b->row[index]) sprite(tiles,pixels,x-8,y,8,8,48,72,8,8,active);
    }
    return 1;
}
int bfm_library_books_draw(const BfmLibraryBooks *b,const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024],uint32_t pixels[240][320])
{
    if (!b || !panel(b,0,tiles,pixels)) return 0;
    if (b->depth && (!panel(b,1,tiles,pixels) || !panel(b,2,tiles,pixels))) return 0;
    uint32_t addr=b->warning ? 0x8019f69cu : 0x8019f660u;
    const uint8_t *question=data(b,addr,3), *box=data(b,0x8019f010u,12);
    const uint8_t *portrait=data(b,0x8019e8b8u,20);
    return question && box && portrait &&
           bfm_library_question_draw(art,box,portrait,tiles,question,b->bytes-(addr-base),b->ink,pixels);
}
