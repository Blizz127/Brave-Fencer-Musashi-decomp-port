/* Synthetic CD FIFO, completed on the same DMA/IRQ bus that dispatches the
 * mounted guest. Only the mounted program may publish a completed ring entry. */
#include "musashi_cd_dma3.h"

enum { CD_TEST_BASE=0x800f0000u, CD_TEST_PAYLOAD=CD_TEST_BASE+0x600u,
       CD_TEST_CALLBACK=0x800469ccu, CD_TEST_DICR=0x1f8010f4u };
typedef struct CdCompletion {
    MusashiCdDma3 *channel;
    MusashiCdIrqFrame frame;
    MusashiResetGraphPrefixStop stop;
    MusashiCdRegisterDevice registers;
    MusashiCdIrqServices services;
    MusashiGuestIrqCd cd;
    uint8_t fifo[12];
    size_t cursor;
    uint32_t slot;
    int request;
} CdCompletion;

static int cd_test_available(void *p,size_t *out) {
    CdCompletion *c=p; *out=sizeof(c->fifo)-c->cursor; return 1;
}
static int cd_test_read(void *p,uint8_t *out,size_t bytes) {
    CdCompletion *c=p;
    if (bytes>sizeof(c->fifo)-c->cursor) return 0;
    memcpy(out,c->fifo+c->cursor,bytes);
    c->cursor+=bytes;
    return 1;
}
static int cd_test_request(void *p,int *out) {
    CdCompletion *c=p; *out=c->request; return 1;
}
static int cd_test_healthy(void *p) { return p!=NULL; }

static void cd_test_begin(CdCompletion *c,MusashiBootMemory *ram,MusashiDmaController *dma,
                          const char *mode,uint64_t (*identity)(void *),uint32_t *cycles) {
    MusashiCdDma3Device dev={c,identity,cd_test_healthy,cd_test_available,cd_test_read,cd_test_request};
    c->request=1;
    c->channel=musashi_cd_dma3_open(ram,dma,&dev,*cycles); assert(c->channel);
    c->registers=(MusashiCdRegisterDevice){.userdata=c->channel,.read32=musashi_cd_dma3_read32,
                                          .write32=musashi_cd_dma3_write32};
    c->cd=(MusashiGuestIrqCd){ram,&c->registers,&c->services,&c->frame,&c->stop};
    for (size_t i=0;i<sizeof(c->fifo);++i) c->fifo[i]=(uint8_t)(0x65u+17u*i);
    uint8_t *dst=musashi_boot_ram_span(ram,CD_TEST_PAYLOAD,sizeof(c->fifo)); assert(dst);
    memset(dst,0xd9,sizeof(c->fifo));
    assert(musashi_dma_controller_write32(dma,0x1f8010f0u,dma->control|0x8000u));
    assert(musashi_dma_controller_write32(dma,CD_TEST_DICR,0x00880000u));
    assert(musashi_cd_dma3_write32(c->channel,0x1f801018u,0x21020843u));
    assert(musashi_cd_dma3_write32(c->channel,0x1f8010b0u,CD_TEST_PAYLOAD));
    assert(musashi_cd_dma3_write32(c->channel,0x1f8010b4u,3u));
    assert(musashi_cd_dma3_write32(c->channel,0x1f8010b8u,0x11400100u));
    *cycles+=34u;
    assert(musashi_cd_dma3_advance(c->channel,*cycles) && c->cursor==8u && !dma->irq->status);
    ++*cycles;
    assert(musashi_cd_dma3_advance(c->channel,*cycles) && c->cursor==12u);
    assert(!memcmp(dst,c->fifo,sizeof(c->fifo)) && dma->irq->status==8u && dma->interrupt==0x88880000u);

    c->slot=!strcmp(mode,"cd3-next") ? 2u : 0;
    uint32_t record=CD_TEST_BASE+32u*c->slot;
    assert(musashi_boot_write32(ram,0x800c6d30u,8));
    assert(musashi_boot_write32(ram,0x800c7c94u,CD_TEST_BASE));
    assert(musashi_boot_write32(ram,0x800c7c98u,0x1000u));
    assert(musashi_boot_write32(ram,0x800c6d3cu,c->slot));
    assert(musashi_boot_write32(ram,0x800c6d38u,3));
    assert(musashi_boot_write32(ram,0x800a5e80u,0));
    assert(musashi_boot_write32(ram,0x800ae644u,1));
    assert(musashi_boot_write32(ram,0x80078800u,0x789cdadeu));
    assert(musashi_boot_write32(ram,0x80078804u,0xc332dfe4u));
    assert(musashi_boot_write32(ram,record,0x76850003u));
    assert(musashi_boot_write32(ram,record+8,0x74a826u));
    assert(musashi_boot_write32(ram,record+28,0xb10474c8u));
    uint32_t target=!strcmp(mode,"cd3-slot") ? CD_TEST_CALLBACK+4u : CD_TEST_CALLBACK;
    assert(musashi_boot_write32(ram,0x8006cbd0u,target));
    if (!strcmp(mode,"cd3-image")) ram->bytes[CD_TEST_CALLBACK&0x1fffffu]^=1u;
    if (!strcmp(mode,"cd3-other")) {
        /* A real earlier channel-three transfer remains in the ledger. */
        assert(musashi_irq_controller_write16(dma->irq,0x1f801070u,0xfff7u));
        assert(musashi_dma_controller_write32(dma,CD_TEST_DICR,0x08840000u));
        assert(musashi_boot_write32(ram,0x8006cbccu,CD_TEST_CALLBACK));
        assert(musashi_dma_controller_complete(dma,2));
        assert(dma->irq->status==8u && dma->interrupt==0x84840000u);
    }
    if (!strcmp(mode,"cd3-active")) {
        /* Preserve the first IRQ flag while a fresh chopped request waits on DREQ. */
        c->cursor=0; c->request=0;
        assert(musashi_cd_dma3_write32(c->channel,0x1f8010b0u,CD_TEST_PAYLOAD+32));
        assert(musashi_cd_dma3_write32(c->channel,0x1f8010b4u,1));
        assert(musashi_cd_dma3_write32(c->channel,0x1f8010b8u,0x11400100u));
        ++*cycles;
        assert(musashi_cd_dma3_advance(c->channel,*cycles));
    }
}
static void cd_test_check(CdCompletion *c,MusashiBootMemory *ram,MusashiDmaController *dma,
                          const char *mode,int success) {
    uint32_t record=CD_TEST_BASE+32u*c->slot, value;
    assert(musashi_boot_read32(ram,record,&value) && value==(success ? 0x76850002u : 0x76850003u));
    assert(musashi_boot_read32(ram,0x80078800u,&value) && value==(success ? 0xb10474c8u : 0x789cdadeu));
    assert(musashi_boot_read32(ram,0x80078804u,&value) && value==(success ? 0x74a826u : 0xc332dfe4u));
    assert(musashi_boot_read32(ram,0x800c6d3cu,&value) && value==(success ? 3u : c->slot));
    assert(musashi_boot_read32(ram,0x800ae644u,&value) && value==(success ? 0u : 1u));
    assert(musashi_boot_read32(ram,record+8,&value) && value==0x74a826u);
    assert(musashi_boot_read32(ram,record+28,&value) && value==0xb10474c8u);
    MusashiCdDma3State state;
    assert(musashi_cd_dma3_get_state(c->channel,&state) && !state.fault && state.transfers==1 && state.bytes==12);
    int active=!strcmp(mode,"cd3-active");
    assert(state.chcr==(active ? 0x01400100u : 0x00400100u));
    assert(c->cursor==(active ? 0u : 12u));
    assert(dma->interrupt==(!strcmp(mode,"cd3-other") ? 0x00840000u : 0x00880000u));
    assert(musashi_cd_dma3_close(c->channel));
}
