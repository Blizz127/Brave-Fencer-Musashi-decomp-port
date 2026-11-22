#include "musashi_mdec_controller.h"
#include <string.h>
#include "musashi_mdec_decode.h"

/* Register/command authority:
 * https://psx-spx.consoledev.net/macroblockdecodermdec/
 * DMA0 request-mode transport uses shared DPCR enable and DICR completion.
 * Color decode consumes real RLE words and provides DMA1 pixel output.
 */
static int channel_register(uint32_t a, unsigned *ch, unsigned *reg) {
    if (a<0x1f801080u || a>0x1f801098u || (a&3u) || (a&15u)>8u) return 0;
    *ch=(a-0x1f801080u)/16u; *reg=(a&15u)/4u; return 1;
}
void musashi_mdec_init(MusashiMdecController *d, MusashiBootMemory *ram,
                        MusashiDmaController *dma) {
    memset(d,0,sizeof(*d)); d->ram=ram; d->dma=dma;
}
int musashi_mdec_read32(MusashiMdecController *d,uint32_t a,uint32_t *v) {
    unsigned ch,reg;
    if (!d || !v) return 0;
    if (a==0x1f801824u) {
        *v=0x80040000u | ((d->command>>2)&0x07800000u) | d->count;
        if(d->output_cursor<d->output_size) {
            *v&=~0x80000000u;
            if(d->control&0x20000000u)*v|=0x08000000u;
        }
        if(d->remaining) *v|=0x20000000u;
        if(d->control&0x40000000u) *v|=0x10000000u;
        return 1;
    }
    if(!channel_register(a,&ch,&reg))return 0;
    *v=reg==0?d->madr[ch]:reg==1?d->bcr[ch]:d->chcr[ch]; return 1;
}
/* DMA1 advances its visible address/count even when armed before decode. The
 * pixel copy waits for data; its later completion survives a CHCR clear. */
static int submit_output(MusashiMdecController *d) {
    uint32_t bytes = d->output_request_bytes;
    if (!bytes) return 1;
    if (bytes > d->output_size - d->output_cursor)
        return d->remaining != 0 || d->output_cursor == d->output_size;
    uint64_t ticks = (uint64_t)bytes * 2u; /* Eight cycles per 32-bit word. */
    if (d->cycle > UINT64_MAX - ticks) return 0;
    memcpy(d->ram->bytes + d->output_request_address, d->output + d->output_cursor, bytes);
    d->output_cursor += bytes;
    d->output_request_bytes = 0;
    d->output_due = d->cycle + ticks;
    d->output_event_pending = 1;
    d->output_finishes_frame = d->output_cursor == d->output_size;
    return 1;
}

static int parameter(MusashiMdecController *d,uint32_t v) {
    unsigned i;
    if(!d->remaining) {
        unsigned cmd=v>>29;
        if(cmd==1 && (((v>>27)&3u)<2u || !(v&0xffffu)))return 0;
        if(d->output_cursor<d->output_size)return 0;
        d->output_cursor=d->output_size=0;
        d->command=v; d->received=0;
        d->remaining=cmd==1?(v&0xffffu):cmd==2?((v&1u)?32u:16u):cmd==3?32u:0u;
        d->count=d->remaining?(d->remaining-1u):(v&0xffffu);
        return 1;
    }
    if((d->command>>29)==2) {
        for(i=0;i<4;i++)d->quant[d->received*4u+i]=(uint8_t)(v>>(i*8u));
    } else if((d->command>>29)==3) {
        d->scale[d->received*2u]=(int16_t)v;
        d->scale[d->received*2u+1u]=(int16_t)(v>>16);
    } else if((d->command>>29)==1) {
        d->input[d->received*2u]=(uint16_t)v;
        d->input[d->received*2u+1u]=(uint16_t)(v>>16);
    } else return 0;
    ++d->received; --d->remaining; d->count=(d->remaining-1u)&0xffffu;
    if(!d->remaining && (d->command>>29)==1)return mdec_decode_frame(d) && submit_output(d);
    return 1;
}
static int submit_input(MusashiMdecController *d, uint32_t words) {
    const uint8_t *p = d->ram->bytes + d->madr[0];
    if (words > d->remaining) return 0;
    for (uint32_t i = 0; i < words; ++i, p += 4) {
        uint32_t value = p[0] | (uint32_t)p[1] << 8 |
                         (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
        if (!parameter(d, value)) return 0;
    }
    d->madr[0] += words * 4u;
    d->bcr[0] &= 0xffffu;
    return 1;
}

int musashi_mdec_write32(MusashiMdecController *d,uint32_t a,uint32_t v) {
    unsigned ch,reg; uint32_t words,addr,segment;
    if(!d || !d->ram || !d->dma)return 0;
    if(a==0x1f801824u) {
        if(v&0x80000000u) {
            d->command=0; d->remaining=0; d->received=0; d->count=0;
            d->output_size=d->output_cursor=0;
        }
        d->control=v&0x60000000u; return 1;
    }
    if(a==0x1f801820u) {
        if(d->chcr[0]&0x01000000u)return 0;
        return parameter(d,v);
    }
    if(!channel_register(a,&ch,&reg))return 0;
    if((d->chcr[ch]&0x01000000u) && !(reg==2 && v==0))return 0;
    if(reg==0){d->madr[ch]=v&0x00ffffffu;return 1;}
    if(reg==1){d->bcr[ch]=v;return 1;}
    if (!(v & 0x01000000u)) {
        d->chcr[ch] = v;
        if (ch == 0) {
            d->input_waits_for_output = 0;
            d->input_dma_error = 0;
            d->output_finishes_frame = 0;
        }
        /* Clearing DMA1's visible CHCR leaves its reserved transfer and
         * any scheduled completion in the decoder. */
        return 1;
    }
    /* Request-mode input/output share the real DMA enable and IRQ owner. */
    if(v!=(ch?0x01000200u:0x01000201u) || !(d->dma->control&(8u<<(ch*4u))) ||
       !(d->control&(ch?0x20000000u:0x40000000u)) || !d->dma->irq)return 0;
    words=(d->bcr[ch]&0xffffu)*(d->bcr[ch]>>16);
    addr=d->madr[ch]&0x1fffffffu; segment=d->madr[ch]&0xe0000000u;
    if(!words || (!ch && words>d->remaining) ||
       (!ch && (d->command>>29)!=1 && words>32u) || (addr&3u) ||
       segment || addr>=MUSASHI_RAM_SIZE || words>(MUSASHI_RAM_SIZE-addr)/4u ||
       d->cycle>UINT64_MAX-1u-words)return 0;
    if (ch) {
        uint32_t bytes = words * 4u;
        uint32_t available = d->output_size - d->output_cursor;
        /* Pre-arming a future frame is supported. Reading past the active
         * compressed frame has no known data source in this decoder. */
        if (d->cycle > UINT64_MAX - (uint64_t)words * 8u ||
            (bytes > available && (available || (d->output_size && d->input_waits_for_output))))
            return 0;
        d->chcr[1] = v;
        d->output_request_address = addr;
        d->madr[1] += bytes;
        d->bcr[1] &= 0xffffu;
        d->output_request_bytes = bytes;
        if (!submit_output(d)) return 0;
    } else {
        d->chcr[0] = v;
        d->due = d->cycle + 1u + words;
        if ((d->command >> 29) == 1u) {
            /* Decode DMA can serve output before DMA0 retires. A malformed
             * payload fails on advancement of the accepted request. */
            d->input_dma_error = !submit_input(d, words);
            d->input_waits_for_output = !d->input_dma_error && d->remaining == 0 && d->output_size != 0;
        }
    }
    return 1;
}
int musashi_mdec_advance(MusashiMdecController *d,uint64_t cycle) {
    uint32_t words;
    if(!d || cycle<d->cycle)return 0;
    d->cycle=cycle;
    if ((d->chcr[0] & 0x01000000u) && !d->input_waits_for_output && cycle >= d->due) {
        words=(d->bcr[0]&0xffffu)*(d->bcr[0]>>16);
        if (d->input_dma_error || (words && !submit_input(d, words))) return 0;
        d->chcr[0]&=~0x01000000u; ++d->transfers;
        if(!musashi_dma_controller_complete(d->dma,0))return 0;
    }
    if (d->output_event_pending && cycle >= d->output_due) {
        d->output_event_pending = 0;
        d->chcr[1] &= ~0x01000000u;
        ++d->transfers;
        if(!musashi_dma_controller_complete(d->dma,1))return 0;
        if (d->input_waits_for_output && d->output_finishes_frame) {
            d->input_waits_for_output = 0;
            d->chcr[0] &= ~0x01000000u;
            ++d->transfers;
            if (!musashi_dma_controller_complete(d->dma, 0)) return 0;
        }
    }
    return 1;
}
