/* SPDX-License-Identifier: MIT */
#include "library_books.h"
#include "bedroom_score.h"
#include <stdlib.h>
enum { PANELS = 3, MAX_ROWS = 6 };
static const uint32_t base = UINT32_C(0x80128158);
struct BfmCastleMenuArt {
    const uint8_t *scene;
    size_t bytes;
    uint8_t font[2][256];
};
struct BfmLibraryBooks {
    BfmCastleMenuArt art;
    const uint8_t *panels[PANELS];
    unsigned depth, row[PANELS], repeat, ink, warning;
    int held, command;
};
static unsigned half(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static uint32_t word(const uint8_t *p) { return half(p) | (uint32_t)half(p+2) << 16; }
static const uint8_t *data(const BfmCastleMenuArt *art, uint32_t address, size_t count)
{
    if (address < base || count > art->bytes || address-base > art->bytes-count) return NULL;
    return art->scene + (address-base);
}
static int load_art(BfmCastleMenuArt *art, const MusashiDiscMedia *disc,
                    const uint8_t *scene, size_t bytes)
{
    art->scene = scene;
    art->bytes = bytes;
    return bfm_bedroom_resident_data(disc, 0x800636d8u, art->font[0], sizeof art->font[0]) &&
           bfm_bedroom_resident_data(disc, 0x80063818u, art->font[1], sizeof art->font[1]);
}

BfmCastleMenuArt *bfm_castle_menu_art_open(const MusashiDiscMedia *disc,
                                           const uint8_t *scene, size_t bytes)
{
    if (!disc || !scene || !bytes) return NULL;
    BfmCastleMenuArt *art = malloc(sizeof *art);
    if (art && !load_art(art, disc, scene, bytes)) {
        free(art);
        return NULL;
    }
    return art;
}

void bfm_castle_menu_art_free(BfmCastleMenuArt *art)
{
    free(art);
}

static unsigned count(const BfmLibraryBooks *b, unsigned panel)
{
    if (panel != 2) return b->panels[panel][2];
    const uint8_t *shape = data(&b->art, 0x8019ed0cu + 2*b->row[1], 2);
    return shape && shape[1] == 1 && shape[0] <= MAX_ROWS ? shape[0] : 0;
}
BfmLibraryBooks *bfm_castle_menu_open(const MusashiDiscMedia *disc,const uint8_t *scene,size_t bytes, int command)
{
    if (!disc || !scene) return NULL;
    BfmLibraryBooks *b = calloc(1,sizeof *b);
    if (!b) return NULL;
    if (!load_art(&b->art, disc, scene, bytes)) goto invalid;
    b->command = command;
    unsigned length = command ? 1 : PANELS;
    const uint8_t *root=data(&b->art,command ? 0x8018c12cu : 0x8019ee94u,4);
    const uint8_t *panels=root ? data(&b->art,word(root),32*length) : NULL;
    if (!panels) goto invalid;
    for (unsigned i=0; i<length; ++i) {
        const uint8_t *p=b->panels[i]=panels+32*i;
        if (half(p)!=(command ? 1u : i==0 ? 3u : i+6u) || p[3]!=1 || !p[2] || p[2]>MAX_ROWS ||
            half(p+12)!=8 || half(p+14)!=2 || !data(&b->art,word(p+4),96) ||
            !data(&b->art,word(p+8),32) || !data(&b->art,word(p+20),2) ||
            !data(&b->art,word(p+24),4*p[2]) || !data(&b->art,word(p+28),4*p[2])) goto invalid;
    }
    return b;
invalid:
    free(b);
    return NULL;
}
unsigned bfm_castle_command_selection(const BfmLibraryBooks *b) { return b->row[0]; }
unsigned bfm_library_books_selection(const BfmLibraryBooks *b)
{
    return (b->row[1]+1)*256 + b->row[2]+1;
}
void bfm_library_books_resume(BfmLibraryBooks *b, int contents)
{
    b->depth = contents ? 2 : 0;
    if (!contents) b->row[0] = b->row[1] = b->row[2] = 0;
    b->warning = 0;
    b->ink = b->repeat = 0;
    b->held = 0;
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
        if (b->command) {
            if (b->row[0]!=3) *effect=0x474;
            return BFM_COMMAND_SELECTED;
        }
        if (!b->depth && b->row[0]) {
            *effect=0x474;
            return BFM_BOOKS_QUIT;
        }
        if (b->depth==2) {
            *effect=0x474;
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
                   int w,int h,int u,int v,int uw,int vh,int active,int dim)
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
            if (dim && ch < 2) intensity >>= 1;
            if (!active) intensity=intensity*88/128;
            rgb|=expand(intensity)<<(16-8*ch);
        }
        pixels[y+j][x+i]=rgb;
    }
}
static BfmCastleMenuLabel scene_label(const BfmCastleMenuArt *art, uint32_t address)
{
    const uint8_t *start = data(art, address, 1);
    return (BfmCastleMenuLabel){.bytes=start, .size=start ? art->bytes - (address - base) : 0};
}

static int label(const BfmCastleMenuArt *art,BfmCastleMenuLabel name,const uint16_t tiles[512][1024],
                 uint32_t pixels[240][320],int x,int y,int active)
{
    struct Glyph {int x,u,v;} run[48];
    unsigned used=0;
    if (!name.bytes) return 0;
    for (size_t at=0;;at+=2) {
        if (at>=name.size) return 0;
        const uint8_t *p=name.bytes+at;
        if (!*p) break;
        if (name.size-at<2 || used==48 || (*p!=0x81 && *p!=0x82)) return 0;
        unsigned group=*p==0x81;
        unsigned first=group ? 0x40 : 0x4f;
        if (p[1]<first || p[1]-first>=128) return 0;
        const uint8_t *uv=art->font[group]+2*(p[1]-first);
        if (uv[0]==113 && uv[1]==56) x+=4;
        else { run[used++]=(struct Glyph){x,uv[0],uv[1]}; x+=7; }
    }
    while (used) {
        struct Glyph g=run[--used];
        sprite(tiles,pixels,g.x,y,8,8,g.u,g.v,8,8,active,name.dim);
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
static int paint_panel(const BfmCastleMenuArt *art, const uint8_t *p,
                       const BfmCastleMenuLabel *names, unsigned rows, unsigned selected,
                       int active, int can_scroll_up, int can_scroll_down,
                       const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!rows || rows > MAX_ROWS || selected >= rows || p[3] != 1 ||
        half(p+12) != 8 || half(p+14) != 2) return 0;
    const uint8_t *frames = data(art, word(p+4), 96), *fills = data(art, word(p+8), 32);
    const uint8_t *places = data(art, word(p+24), 4*rows);
    const uint8_t *labels = names ? NULL : data(art, word(p+28), 4*rows);
    if (!frames || !fills || !places || (!names && !labels)) return 0;
    fill(pixels,fills+16,active);
    if (active) fill(pixels,fills,1);
    for (int i=0;i<8;++i) {
        const uint8_t *f=frames+12*i;
        sprite(tiles,pixels,160+(int16_t)half(f),120+(int16_t)half(f+2),half(f+4),half(f+6),
               half(f+8),half(f+10),(i==4 || i==5)?7:8,i>=6?7:8,active,0);
    }
    if (!label(art,scene_label(art,word(p+20)),tiles,pixels,
               160+(int16_t)half(p+16),120+(int16_t)half(p+18),active)) return 0;
    for (unsigned i=0; i<rows; ++i) {
        int x=160+(int16_t)half(places+4*i), y=120+(int16_t)half(places+4*i+2);
        BfmCastleMenuLabel name = names ? names[i] : scene_label(art,word(labels+4*i));
        if (!label(art,name,tiles,pixels,x,y,active)) return 0;
        if (i==selected) sprite(tiles,pixels,x-8,y,8,8,48,72,8,8,active,0);
    }
    int arrow_x = 160 + (int16_t)half(frames+12) - 9;
    if (can_scroll_up)
        sprite(tiles,pixels,arrow_x,118+(int16_t)half(places+2),8,8,120,48,8,8,active,0);
    if (can_scroll_down)
        sprite(tiles,pixels,arrow_x,121+(int16_t)half(places+4*(rows-1)+2),8,8,120,56,8,8,active,0);
    return 1;
}

int bfm_castle_menu_art_draw_panel(const BfmCastleMenuArt *art, uint32_t panel_address,
                                   const BfmCastleMenuLabel *visible_names, unsigned visible_count,
                                   unsigned selected, int active, int can_scroll_up, int can_scroll_down,
                                   const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!art || !tiles || !pixels) return 0;
    const uint8_t *p = data(art, panel_address, 32);
    return p && visible_count <= p[2] &&
           paint_panel(art,p,visible_names,visible_count,selected,active,can_scroll_up,
                       can_scroll_down,tiles,pixels);
}

static int panel(const BfmLibraryBooks *b, unsigned index, const uint16_t tiles[512][1024],
                 uint32_t pixels[240][320])
{
    unsigned rows = count(b,index);
    BfmCastleMenuLabel topics[MAX_ROWS];
    if (index == 2) {
        const uint8_t *lookup = data(&b->art,0x8019ed18u+4*b->row[1],4);
        const uint8_t *labels = lookup ? data(&b->art,word(lookup),4*rows) : NULL;
        if (!labels || rows > MAX_ROWS) return 0;
        for (unsigned i=0; i<rows; ++i) topics[i] = scene_label(&b->art,word(labels+4*i));
    }
    return paint_panel(&b->art,b->panels[index],index == 2 ? topics : NULL,rows,
                       b->row[index],index == b->depth,0,0,tiles,pixels);
}
int bfm_library_books_draw(const BfmLibraryBooks *b,const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024],uint32_t pixels[240][320])
{
    if (!b || !panel(b,0,tiles,pixels)) return 0;
    if (b->depth && (!panel(b,1,tiles,pixels) || !panel(b,2,tiles,pixels))) return 0;
    uint32_t addr=b->command ? 0x8018c300u : b->warning ? 0x8019f69cu : 0x8019f660u;
    const uint8_t *question=data(&b->art,addr,3), *box=data(&b->art,b->command ? 0x8018c2b0u : 0x8019f010u,12);
    const uint8_t *portrait=data(&b->art,b->command ? 0x8018bb6cu : 0x8019e8b8u,20);
    return question && box && portrait &&
           bfm_library_question_draw(art,box,portrait,tiles,question,b->art.bytes-(addr-base),b->ink,pixels);
}
