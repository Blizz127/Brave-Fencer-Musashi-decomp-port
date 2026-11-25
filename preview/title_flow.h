#ifndef BFM_TITLE_FLOW_H
#define BFM_TITLE_FLOW_H

#include <stdint.h>

typedef enum BfmTitlePhase {
    BFM_TITLE_PROMPT,
    BFM_TITLE_MENU,
    BFM_TITLE_LOADING,
    BFM_TITLE_OPENING,
    BFM_TITLE_CONTINUE_UNAVAILABLE
} BfmTitlePhase;

typedef enum BfmTitleChoice {
    BFM_CHOICE_START,
    BFM_CHOICE_CONTINUE
} BfmTitleChoice;

typedef struct BfmTitleFlow {
    BfmTitlePhase phase;
    BfmTitleChoice choice;
    uint16_t held;
    uint16_t room;
    int prompt_visible;
    int idle_ticks;
    int blink_ticks;
} BfmTitleFlow;

/* held uses PlayStation controller wire ordering with 1 for a held button. */
void bfm_title_begin(BfmTitleFlow *flow, uint16_t held);
void bfm_title_input(BfmTitleFlow *flow, uint16_t held);
/* Call with room_ready after the owner-supplied opening content loads. */
void bfm_title_finish_loading(BfmTitleFlow *flow, int room_ready);

#endif
