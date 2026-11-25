/* Host services for the independently matched US title update in 800cf104.c. */
#include "title_flow.h"
#include "psx_types.h"


void func_800CF104(void);

/* The matched update uses one scene instance and calls its pad and scene
 * services synchronously on the application thread. */
s16 D_800EC678;
s32 D_800EC67C, D_800EC680, D_800EC690, D_800EC694, D_800EC698;
static uint16_t pressed;
static int advance_scene;

s32 func_800149E0(s32 port)
{
    return port == 0 ? pressed : 0;
}

void func_80014B10(s32 port)
{
    if (port == 0)
        pressed = 0;
}

void func_800118AC(void)
{
    advance_scene = 1;
}

/* The preview presents the disc-backed scene after each update. Title menu
 * tones and its color setup have no corresponding audio/graphics command. */
void func_8002D4C8(s32 sound, s32 parameter)
{
    (void)sound;
    (void)parameter;
}

void func_800CF3E8(void) {}
void func_800CFE00(void) {}

void bfm_title_begin(BfmTitleFlow *flow, uint16_t held)
{
    *flow = (BfmTitleFlow){
        .phase = BFM_TITLE_PROMPT,
        .choice = BFM_CHOICE_START,
        .held = held,
        .prompt_visible = 1
    };
}

void bfm_title_input(BfmTitleFlow *flow, uint16_t held)
{
    uint16_t edges = (uint16_t)(held & ~flow->held);
    flow->held = held;
    if (flow->phase == BFM_TITLE_CONTINUE_UNAVAILABLE) {
        if (edges & ((1u << 3) | (1u << 13) | (1u << 14))) {
            flow->phase = BFM_TITLE_MENU;
            flow->idle_ticks = 0;
        }
        return;
    }
    if (flow->phase != BFM_TITLE_MENU && flow->phase != BFM_TITLE_PROMPT)
        return;

    /* The title routine reads the two digital pad bytes in big-endian order. */
    pressed = (uint16_t)((edges >> 8) | (edges << 8));
    D_800EC678 = (s16)flow->choice;
    D_800EC67C = flow->idle_ticks;
    D_800EC680 = 0;
    D_800EC690 = flow->phase == BFM_TITLE_PROMPT ? 1 : 2;
    D_800EC694 = flow->prompt_visible;
    D_800EC698 = flow->blink_ticks;
    advance_scene = 0;
    func_800CF104();
    flow->phase = D_800EC690 == 1 ? BFM_TITLE_PROMPT : BFM_TITLE_MENU;
    flow->choice = (BfmTitleChoice)D_800EC678;
    flow->idle_ticks = D_800EC67C;
    flow->blink_ticks = D_800EC698;
    flow->prompt_visible = D_800EC694;

    if (advance_scene) {
        if (D_800EC680 == BFM_CHOICE_START) {
            flow->room = 0x3005;
            flow->phase = BFM_TITLE_LOADING;
        } else if (D_800EC680 == BFM_CHOICE_CONTINUE) {
            flow->phase = BFM_TITLE_CONTINUE_UNAVAILABLE;
        } else {
            /* Retail returns to an attract sequence. This preview reopens
             * the title without starting a new game or accepting held input. */
            bfm_title_begin(flow, held);
        }
    }
}

void bfm_title_finish_loading(BfmTitleFlow *flow, int room_ready)
{
    if (flow->phase == BFM_TITLE_LOADING && room_ready)
        flow->phase = BFM_TITLE_OPENING;
}
