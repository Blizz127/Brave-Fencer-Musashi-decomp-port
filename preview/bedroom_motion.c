/* SPDX-License-Identifier: MIT */
#include "bedroom_motion.h"
#include "field_rest_pose.h"
#include "woods_slope.h"
#include "gameplay_run.h"
#include <stdlib.h>

enum { PART_POSE_BYTES = 256 };
static const uint32_t scene_base = 0x80128158u, main_base = 0x800cedf8u;

typedef struct {
    uint32_t base;
    const uint8_t *bytes;
    size_t length;
} Region;

typedef struct {
    uint32_t program;
    unsigned cursor, remaining, interval;
    enum { STREAM_ACTIVE, STREAM_LOOP, STREAM_HOLD, STREAM_END } boundary;
} Channel;

enum Activity { QUIET, TILTING, UNROLLING, RISING, YAWNING, STANDING, RECOVERING_STANCE, RETURNING, STARTING_RUN, SHOWING_FUSION, COLLECTING, KNOCKING };

enum RestPhase { RESTING, SHIFTING, AFTER_SHIFT, DRAWING, READY };

struct BfmBedroomMotion {
    Region main, scene;
    uint32_t body_bank, face_bank;
    Channel body, face;
    enum Activity activity;
    enum RestPhase rest_phase;
    unsigned rest_cycles;
    unsigned pitch, angular_speed;
    int tired;
    uint32_t face_upload;
    BfmBedroomPose frame;
};

static const uint8_t *region(const Region *r, uint32_t address, size_t length)
{
    size_t offset;
    if (address < r->base) return NULL;
    offset = address - r->base;
    if (offset > r->length || length > r->length - offset) return NULL;
    return r->bytes + offset;
}

static const uint8_t *data(const BfmBedroomMotion *m, uint32_t address, size_t length)
{
    return region(address >= scene_base ? &m->scene : &m->main, address, length);
}

static int word(const BfmBedroomMotion *m, uint32_t address, uint32_t *out)
{
    const uint8_t *p = data(m, address, 4);
    if (!p || (address & 3u)) return 0;
    *out = p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return 1;
}

static void start(Channel *channel, uint32_t program)
{
    *channel = (Channel){.program=program, .remaining=1, .interval=1};
}

static int select_slot(BfmBedroomMotion *m, Channel *channel, uint32_t bank, unsigned index)
{
    uint32_t program;
    if (bank > UINT32_MAX - index * 4 || !word(m, bank + index * 4, &program) ||
        !data(m, program, 4)) return 0;
    if (channel->program != program) start(channel, program);
    return 1;
}

static int next_word(const BfmBedroomMotion *m, Channel *channel, uint32_t *out)
{
    if (channel->cursor > 255 || channel->program > UINT32_MAX - channel->cursor * 4)
        return 0;
    return word(m, channel->program + channel->cursor++ * 4, out);
}

/* Body animation and facial texture share stop, restart and frame-duration
 * commands. Each channel can consume multiple controls before emitting a frame. */
static int advance(BfmBedroomMotion *m, Channel *channel, int body)
{
    if (!channel->remaining || --channel->remaining) return 1;
    channel->boundary = STREAM_ACTIVE;
    for (unsigned budget = 0; budget < 256; ++budget) {
        uint32_t command, operand;
        if (!next_word(m, channel, &command)) return 0;
        switch (command) {
        case 0: /* End and keep the last frame. */
        case 2: /* Hold; the action can resume at the following command. */
            channel->boundary = command == 2 ? STREAM_HOLD : STREAM_END;
            return 1;
        case 1:
            channel->cursor = 0;
            channel->boundary = STREAM_LOOP;
            break;
        case 3:
            if (!next_word(m, channel, &operand) || !operand || operand > 255) return 0;
            channel->interval = operand;
            break;
        case 4: /* Body resource declaration. */
            if (!body || !next_word(m, channel, &operand) || !data(m, operand, 4)) return 0;
            break;
        case 6: /* Jump to a word in this body program and present its key now. */
            if (!body || !next_word(m, channel, &operand) || operand > 255) return 0;
            channel->cursor = operand;
            break;
        case 7: /* A body event precedes the next pose in the same update. */
            if (!body) return 0;
            if (m->activity == KNOCKING) m->frame.knock_contact = 1;
            break;
        case 8:
        case 9:
            if (!body) return 0;
            m->frame.body_contact = command;
            break;
        case 15: /* Select a facial texture animation with this body action. */
            if (!body || !next_word(m, channel, &operand) || operand > 255 ||
                !select_slot(m, &m->face, m->face_bank, operand)) return 0;
            break;
        case 16: /* Request the held weapon for this pose. */
            if (!body) return 0;
            m->frame.show_weapon = 1;
            break;
        case 17: /* Sheathe equipped weapons for this motion. */
            if (!body) return 0;
            m->frame.show_weapon = 0;
            break;
        case 18: /* Draw Lumina for the cable-riding motion. */
            if (!body) return 0;
            m->frame.show_lumina = 1;
            break;
        case 19: /* Sheathe Lumina independently of Fusion. */
            if (!body) return 0;
            m->frame.show_lumina = 0;
            break;
        default:
            if (body) {
                m->frame.pose = data(m, command, PART_POSE_BYTES);
                if (!m->frame.pose) return 0;
            } else {
                if ((command & 0xfffffff8u) == 0x40000000u) {
                    m->frame.face_word = command;
                    m->face_upload = 0;
                } else {
                    if (!region(&m->main, command, 16)) return 0;
                    m->frame.face_word = 0;
                    m->face_upload = command;
                }
            }
            channel->remaining = channel->interval;
            return 1;
        }
    }
    return 0;
}

/* Selecting a new body pose consumes its first frame immediately. The
 * ordinary player update subsequently advances both channels once more. */
static int select_body(BfmBedroomMotion *m, unsigned index)
{
    uint32_t previous = m->body.program;
    if (!select_slot(m, &m->body, m->body_bank, index)) return 0;
    return previous == m->body.program || advance(m, &m->body, 1);
}

static int restart_body(BfmBedroomMotion *motion, uint32_t program)
{
    if (!motion || !program) return 0;
    BfmBedroomMotion next = *motion;
    start(&next.body, program);
    next.activity = QUIET;
    next.frame.draw_current = 1;
    if (!advance(&next, &next.body, 1)) return 0;
    *motion = next;
    return 1;
}

int bfm_bedroom_motion_direct(BfmBedroomMotion *motion, uint32_t program)
{
    return motion && program && (motion->body.program == program ||
                                 restart_body(motion, program));
}

int bfm_bedroom_motion_step_then_restart(BfmBedroomMotion *motion, uint32_t program,
                                         BfmBedroomPose *frame)
{
    if (!motion || !frame) return 0;
    BfmBedroomMotion next = *motion;
    BfmBedroomPose previous;
    if (!bfm_bedroom_motion_step(&next, BFM_BEDROOM_NONE, &previous) ||
        !restart_body(&next, program)) return 0;
    bfm_bedroom_motion_publish_upright(&next, frame);
    *motion = next;
    return 1;
}

int bfm_bedroom_motion_direct_slot(BfmBedroomMotion *motion, unsigned slot)
{
    uint32_t program;
    return motion && slot < 40 && word(motion, motion->body_bank + 4 * slot, &program) &&
           bfm_bedroom_motion_direct(motion, program);
}

int bfm_bedroom_motion_looped(const BfmBedroomMotion *motion)
{
    return motion && motion->body.boundary == STREAM_LOOP;
}

int bfm_bedroom_motion_finish_program(BfmBedroomMotion *motion, BfmBedroomPose *frame)
{
    if (!frame || bfm_bedroom_motion_marker(motion) != BFM_BODY_END) return 0;
    BfmBedroomMotion next = *motion;
    if (!select_body(&next, 0)) return 0;
    next.activity = QUIET;
    next.frame.draw_current = 0;
    *frame = next.frame;
    *motion = next;
    return 1;
}

void bfm_bedroom_motion_release_draw(BfmBedroomMotion *motion, BfmBedroomPose *frame)
{
    motion->frame.draw_current = 0;
    frame->draw_current = 0;
}

BfmBodyMarker bfm_bedroom_motion_marker(const BfmBedroomMotion *motion)
{
    if (motion && motion->body.boundary == STREAM_HOLD) return BFM_BODY_HOLD;
    if (motion && motion->body.boundary == STREAM_END) return BFM_BODY_END;
    return BFM_BODY_PLAYING;
}

int bfm_bedroom_motion_resume(BfmBedroomMotion *motion)
{
    if (bfm_bedroom_motion_marker(motion) != BFM_BODY_HOLD) return 0;
    motion->body.remaining = motion->body.interval;
    motion->body.boundary = STREAM_ACTIVE;
    return 1;
}

static int pair(BfmBedroomMotion *m, unsigned body, unsigned face)
{
    return select_body(m, body) &&
           select_slot(m, &m->face, m->face_bank, face);
}

int bfm_bedroom_motion_examine_tick(BfmBedroomMotion *motion, int tired, int show_fusion,
                                    BfmBedroomPose *frame)
{
    if (!motion || !frame) return 0;
    BfmBedroomMotion next = *motion;
    next.frame.knock_contact = next.frame.body_contact = next.frame.finished = 0;
    /* The shared animation pass precedes Square admission. The outgoing
     * standing/walking action selects the neutral face without ticking it. */
    if (!advance(&next,&next.body,1) || !advance(&next,&next.face,0) ||
        !pair(&next,tired ? 5 : 4,tired ? 28 : 17) ||
        !select_slot(&next,&next.face,next.face_bank,17)) return 0;
    next.activity = QUIET;
    next.tired = tired;
    next.rest_phase = RESTING;
    next.rest_cycles = 0;
    next.frame.draw_current = 0;
    next.frame.show_weapon = show_fusion;
    next.frame.show_lumina = 0;
    *motion = next;
    *frame = next.frame;
    return 1;
}

BfmBedroomMotion *bfm_bedroom_motion_open(const uint8_t *scene, size_t scene_bytes,
                                          const uint8_t *main, size_t main_bytes, uint32_t room)
{
    if (!scene || !main) return NULL;
    BfmBedroomMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene_base, scene, scene_bytes};
    m->main = (Region){main_base, main, main_bytes};
    if (!region(&m->scene, room, 0x54) ||
        !word(m, room + 0x4c, &m->body_bank) || !word(m, room + 0x50, &m->face_bank) ||
        !region(&m->main, m->body_bank, 40 * 4) || !region(&m->main, m->face_bank, 35 * 4)) {
        free(m);
        return NULL;
    }
    return m;
}

static int start_lying_down(BfmBedroomMotion *m)
{
    if (!pair(m, 38, 30)) return 0;
    m->pitch = 4073;
    m->angular_speed = 0;
    return 1;
}

static int update(BfmBedroomMotion *m, BfmBedroomAction action, int field_standing)
{
    int update_body = 1;
    m->frame.knock_contact = 0;
    m->frame.body_contact = 0;
    m->frame.finished = 0;
    switch (action) {
    case BFM_BEDROOM_LIE:
        if (!start_lying_down(m)) return 0;
        m->activity = TILTING;
        break;
    case BFM_BEDROOM_NIGHTMARE_SLEEP:
        /* The recovery room starts the sleeping loop with the existing face. */
        if (!select_body(m, 39)) return 0;
        m->pitch = 3072;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_NIGHTMARE_HOLD:
        /* Stop on the current body pose while the facial channel continues. */
        m->body.remaining = 0;
        break;
    case BFM_BEDROOM_RISE:
        start(&m->body, 0x80185d9cu);
        if (!advance(m, &m->body, 1)) return 0;
        m->pitch = 3072;
        m->activity = RISING;
        break;
    case BFM_BEDROOM_YAWN:
        m->frame.draw_current = 1;
        if (!pair(m, 36, 29)) return 0;
        m->activity = YAWNING;
        break;
    case BFM_BEDROOM_WALK:
        if (!pair(m, 6, 33)) return 0;
        m->activity = STARTING_RUN;
        break;
    case BFM_BEDROOM_FALL:
        /* Horizontal travel continues as Musashi leaves the mattress. */
        if (!pair(m, 15, 34)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_LAND:
        if (!select_body(m, 16)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_STAND:
        if (!pair(m, 4, 17)) return 0;
        m->activity = STANDING;
        break;
    case BFM_BEDROOM_ENTER:
    case BFM_BEDROOM_REST:
        /* The room releases its direct pose after the player's ordinary
         * motion pass. Keep the first standing pose for this submission. */
        update_body = m->activity != SHOWING_FUSION;
        m->frame.show_weapon = action == BFM_BEDROOM_ENTER;
        m->frame.draw_current = 0;
        if (!pair(m, 0, 17)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_SEARCH:
        m->frame.draw_current = 1;
        if (!select_body(m, 34)) return 0;
        m->activity = STANDING;
        break;
    case BFM_BEDROOM_COLLECT:
        if (!select_body(m, 33)) return 0;
        m->activity = COLLECTING;
        m->frame.draw_current = 1;
        break;
    case BFM_BEDROOM_RECEIVE:
    case BFM_BEDROOM_LOWER:
        start(&m->body, action == BFM_BEDROOM_RECEIVE ? 0x800d5da8u : 0x800d5dc0u);
        if (!advance(m, &m->body, 1)) return 0;
        m->frame.draw_current = 1;
        m->activity = action == BFM_BEDROOM_RECEIVE ? QUIET : COLLECTING;
        break;
    case BFM_BEDROOM_KNOCK:
        /* The village's Knock request selects the resident direct body
         * program. Its two impacts belong to the motion, including holds. */
        start(&m->body, 0x800d5aa0u);
        m->frame.draw_current = 1;
        m->activity = KNOCKING;
        break;
    case BFM_BEDROOM_CABLE:
        start(&m->body, 0x80182168u);
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_FAREWELL:
    case BFM_BEDROOM_NEGATIVE_REPLY:
    case BFM_BEDROOM_DRAW_WATER: {
        uint32_t program = 0x800d5934u;
        if (action == BFM_BEDROOM_FAREWELL) program = 0x800d59ecu;
        if (action == BFM_BEDROOM_NEGATIVE_REPLY) program = 0x800d599cu;
        start(&m->body, program);
        if (!advance(m, &m->body, 1)) return 0;
        m->frame.draw_current = 1;
        m->activity = COLLECTING;
        break;
    }
    case BFM_BEDROOM_SHOW_FUSION:
        start(&m->body, 0x801ca6ecu);
        if (!advance(m, &m->body, 1)) return 0;
        m->frame.draw_current = 1;
        m->activity = SHOWING_FUSION;
        break;
    case BFM_BEDROOM_UPRIGHT:
        m->pitch = 0;
        break;
    case BFM_BEDROOM_NONE:
        break;
    default:
        return 0;
    }

    if (m->activity == TILTING) {
        /* At 1.75 angular units per tick squared, Musashi settles on his side. */
        m->angular_speed += 7;
        unsigned pitch = m->pitch - m->angular_speed / 4;
        if (pitch < 3072) {
            m->pitch = 3072;
            m->body.remaining = 1;
            m->body.boundary = STREAM_ACTIVE;
            m->activity = UNROLLING;
        } else m->pitch = pitch;
    } else if (m->activity == STARTING_RUN && action == BFM_BEDROOM_NONE) {
        /* The scripted full analog input reaches running speed on the
         * first ground update following locomotion entry. */
        if (!select_body(m, 8)) return 0;
        m->activity = QUIET;
    } else if (m->activity == RETURNING) {
        /* The scene marks the scripted player done. Normal stance selection
         * resumes on the following player update. */
        if (!pair(m, 0, 17)) return 0;
        m->activity = QUIET;
    } else if (m->activity == KNOCKING && m->body.boundary) {
        /* Keep the lowered hand until the scene orders the backward walk. */
        m->frame.draw_current = 0;
        m->frame.finished = 1;
        m->activity = QUIET;
    } else if (m->body.boundary && m->activity == RECOVERING_STANCE && m->tired) {
        if (!pair(m, 5, 28)) return 0;
    } else if (m->body.boundary && m->activity != QUIET && m->activity != SHOWING_FUSION &&
               !(field_standing && m->activity == STANDING)) {
        if (m->activity == UNROLLING && !pair(m, 39, 31)) return 0;
        if (m->activity == YAWNING && !select_body(m, 37)) return 0;
        if (m->activity == RECOVERING_STANCE) m->activity = STANDING;
        if (m->activity == STANDING) {
            if (!select_body(m, 0)) return 0;
            m->frame.draw_current = 0;
        }
        if (m->activity == COLLECTING) {
            if (!pair(m, 0, 17)) return 0;
            m->frame.draw_current = 0;
        }
        m->frame.finished = m->activity != STANDING;
        m->activity = m->activity == RISING ? RETURNING : QUIET;
    }
    if ((update_body && !advance(m, &m->body, 1)) || !advance(m, &m->face, 0)) return 0;
    m->frame.pitch = m->pitch;
    return 1;
}

int bfm_bedroom_motion_reenter_stance(BfmBedroomMotion *m, int tired)
{
    if (!m) return 0;
    if (tired < 0) tired = m->tired;
    if (!pair(m, tired ? 5 : 4, tired ? 28 : 17)) return 0;
    m->activity = RECOVERING_STANCE;
    m->tired = tired;
    return 1;
}

int bfm_bedroom_motion_field_rest(BfmBedroomMotion *motion, BfmFieldRestPose phase,
                                  unsigned tick, BfmBedroomPose *frame)
{
    if (!motion || !frame) return 0;
    BfmBedroomMotion next = *motion;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    switch (phase) {
    case BFM_FIELD_REST_CHARGE:
        /* The standing stretch and its held continuation are the same mounted
         * body and face programs that the room uses for a yawn. */
        if (!tick) action = BFM_BEDROOM_YAWN;
        break;
    case BFM_FIELD_REST_DROP:
        if (!tick) {
            if (!start_lying_down(&next)) return 0;
            /* The field initializer submits the first pose at the initial
             * pitch. Angular acceleration begins on the following update. */
            next.activity = QUIET;
        }
        break;
    case BFM_FIELD_REST_ASLEEP:
        /* The end of the drop already selected the looping sleeping pose. */
        break;
    case BFM_FIELD_REST_WAKE:
        if (!bfm_bedroom_motion_reenter_stance(&next, -1)) return 0;
        action = BFM_BEDROOM_UPRIGHT;
        next.frame.draw_current = 0;
        break;
    default:
        return 0;
    }
    if (!update(&next, action, 0)) return 0;
    if (phase == BFM_FIELD_REST_DROP && !tick) next.activity = TILTING;
    next.rest_phase = RESTING;
    next.rest_cycles = 0;
    *motion = next;
    *frame = next.frame;
    if (phase == BFM_FIELD_REST_WAKE ||
        (phase == BFM_FIELD_REST_DROP && frame->finished)) return 2;
    return 1;
}

int bfm_bedroom_motion_ability_pose(BfmBedroomMotion *m, unsigned face_slot, int finished,
                                    BfmBedroomPose *frame)
{
    if (!m || !frame || (face_slot && !select_slot(m, &m->face, m->face_bank, face_slot))) return 0;
    if (!finished) return 1;
    /* The ability used its own body program. Select the ordinary entry clip
     * even if the old locomotion channel happened to remember that clip.
     * The face is shared; selecting neutral twice preserves its hold. */
    m->body.program = 0;
    m->rest_phase = RESTING;
    m->rest_cycles = 0;
    if (!bfm_bedroom_motion_reenter_stance(m, -1)) return 0;
    *frame = m->frame;
    return 1;
}

void bfm_bedroom_motion_publish_upright(BfmBedroomMotion *m, BfmBedroomPose *frame)
{
    m->pitch = m->frame.pitch = 0;
    *frame = m->frame;
}

int bfm_bedroom_motion_reenter_fall(BfmBedroomMotion *m, int moving)
{
    if (!m || !pair(m, moving ? 15 : 13, 34)) return 0;
    m->activity = QUIET;
    return 1;
}

void bfm_bedroom_motion_descend(BfmBedroomMotion *m)
{
    /* Both ordinary falling programs hold near the apex, then play their
     * descent. Other body programs have independent hold/release conditions. */
    if (bfm_bedroom_motion_marker(m) == BFM_BODY_HOLD &&
        (m->body.program == UINT32_C(0x800d4a9c) || m->body.program == UINT32_C(0x800d4b48)))
        bfm_bedroom_motion_resume(m);
}

int bfm_bedroom_motion_reenter_slide(BfmBedroomMotion *m)
{
    if (!m || !select_body(m, BFM_WOODS_SLOPE_BODY)) return 0;
    m->activity = QUIET;
    return 1;
}

int bfm_bedroom_motion_leave_hop(BfmBedroomMotion *m, unsigned support,
                                 int moving, BfmBedroomPose *frame)
{
    if (!m || !frame) return 0;
    /* The active body belonged to the ability archive. Re-enter even if the
     * local ordinary channel happened to retain the same old program. */
    m->body.program = 0;
    if (!select_slot(m,&m->face,m->face_bank,17)) return 0;
    if (!support || support == 0x8000) {
        if (!bfm_bedroom_motion_reenter_fall(m,moving)) return 0;
    } else if (support & 0x4000) {
        if (!bfm_bedroom_motion_reenter_slide(m)) return 0;
    } else if (!bfm_bedroom_motion_reenter_stance(m,-1)) return 0;
    *frame = m->frame;
    return 1;
}

void bfm_bedroom_motion_stance_condition(BfmBedroomMotion *m, int tired)
{
    if (m) m->tired = tired;
}

/* The room advances both mounted channels before running the standing
 * action. Its facial choice and its body loop counter are independent. */
static int standing_field(BfmBedroomMotion *m, BfmGameplayRun *run, BfmBedroomStance stance)
{
    if (m->tired) return m->body.boundary == STREAM_ACTIVE || pair(m, 5, 28);
    if (m->face.boundary && !m->face.remaining) {
        unsigned neutral = m->rest_phase < DRAWING ? 17 : 19;
        uint32_t blink;
        if (!word(m, m->face_bank + 4 * (neutral + 1), &blink)) return 0;
        if (m->face.program == blink) {
            if (!select_slot(m, &m->face, m->face_bank, neutral)) return 0;
        } else if (!(bfm_gameplay_random(run) & 15) &&
                   !select_slot(m, &m->face, m->face_bank, neutral + 1)) return 0;
    }
    if (!m->body.boundary) return 1;
    if (m->activity == STANDING) {
        m->activity = QUIET;
        m->frame.draw_current = 0;
    }
    if (stance == BFM_BEDROOM_STANCE_SCRIPTED) {
        m->rest_phase = RESTING;
        m->rest_cycles = 0;
    }
    switch (m->rest_phase) {
    case RESTING:
        if (++m->rest_cycles == 5) {
            m->rest_cycles = 0;
            m->rest_phase = SHIFTING;
            return select_body(m, 1);
        }
        break;
    case SHIFTING:
        m->rest_phase = AFTER_SHIFT;
        return select_body(m, 0);
    case AFTER_SHIFT:
        if (++m->rest_cycles == 3) {
            m->rest_cycles = 0;
            if (stance == BFM_BEDROOM_STANCE_SHEATHED) {
                m->rest_phase = RESTING;
                return select_body(m, 0);
            }
            m->rest_phase = DRAWING;
            return select_body(m, 2);
        }
        break;
    case DRAWING:
        m->rest_phase = READY;
        m->frame.show_weapon = 0;
        return pair(m, 3, 19);
    case READY:
        return 1;
    }
    /* Repeated base cycles restart their clip at its first key. */
    m->body.program = 0;
    return select_body(m, 0);
}

int bfm_bedroom_motion_step_field(BfmBedroomMotion *motion, BfmBedroomAction action,
                                  BfmGameplayRun *run, BfmBedroomStance stance,
                                  BfmBedroomPose *frame)
{
    if (!motion || !frame || (unsigned)stance > BFM_BEDROOM_STANCE_SHEATHED) return 0;
    BfmBedroomMotion next = *motion;
    int standing = run && stance != BFM_BEDROOM_STANCE_INACTIVE;
    if (!update(&next, action, standing)) return 0;
    if (standing) {
        uint32_t saved_random = run->random_state;
        if (!standing_field(&next, run, stance)) {
            run->random_state = saved_random;
            return 0;
        }
    } else {
        next.rest_phase = RESTING;
        next.rest_cycles = 0;
    }
    *motion = next;
    *frame = next.frame;
    return 1;
}

int bfm_bedroom_motion_step(BfmBedroomMotion *motion, BfmBedroomAction action,
                            BfmBedroomPose *frame)
{
    return bfm_bedroom_motion_step_field(motion, action, NULL, BFM_BEDROOM_STANCE_INACTIVE, frame);
}

static unsigned halfword(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

typedef struct {
    unsigned x, y, width, height;
    const uint8_t *pixels;
} FaceUpload;

static int upload(const BfmBedroomMotion *m, uint32_t address, unsigned count, uint16_t tiles[512][1024])
{
    const uint8_t *header = region(&m->main, address, count * 16 + 4);
    if (!header || header[count * 16] != 0xff) return 0;
    FaceUpload items[2];
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *h = header + i * 16;
        FaceUpload *u = &items[i];
        u->x = halfword(h+4); u->y = halfword(h+6);
        u->width = halfword(h+8); u->height = halfword(h+10);
        uint32_t pixels;
        if (!word(m, address + i * 16 + 12, &pixels) ||
            u->width == 0 || u->height == 0 ||
            u->width > 1024 || u->x > 1024 - u->width ||
            u->height > 512 || u->y > 512 - u->height) return 0;
        u->pixels = region(&m->main, pixels, 2 * u->width * u->height);
        if (!u->pixels) return 0;
    }
    for (unsigned i = 0; i < count; ++i) {
        const FaceUpload *u = &items[i];
        for (unsigned y = 0; y < u->height; ++y)
            for (unsigned x = 0; x < u->width; ++x)
                tiles[u->y + y][u->x + x] = halfword(u->pixels + 2 * (y*u->width + x));
    }
    return 1;
}

int bfm_bedroom_motion_external_face(BfmBedroomMotion *motion, unsigned slot,
                                     int already_stepped, BfmBedroomPose *frame)
{
    if (!motion || !frame) return 0;
    BfmBedroomMotion next = *motion;
    uint32_t prior = next.face.program;
    if (slot && !select_slot(&next,&next.face,next.face_bank,slot)) return 0;
    if ((!already_stepped || prior != next.face.program) && !advance(&next,&next.face,0)) return 0;
    frame->face_word = next.frame.face_word;
    *motion = next;
    return 1;
}

int bfm_bedroom_motion_face(const BfmBedroomMotion *m, uint16_t tiles[512][1024])
{
    return m && tiles && (!m->face_upload || upload(m, m->face_upload, 2, tiles));
}

int bfm_bedroom_motion_upload_palette(const BfmBedroomMotion *motion, uint32_t descriptor,
                                      uint16_t tiles[512][1024])
{
    return motion && tiles && upload(motion, descriptor, 1, tiles);
}

int bfm_bedroom_orb_palette(const BfmBedroomMotion *m, uint32_t table, unsigned orb, uint16_t tiles[512][1024])
{
    return bfm_field_blade_palette(m, table, orb, tiles);
}

int bfm_field_blade_palette(const BfmBedroomMotion *motion, uint32_t normal_table, unsigned orb,
                            uint16_t tiles[512][1024])
{
    /* The room owns seven ordinary entries. The special selection has its own resident palette. */
    if (orb & 0x80)
        return bfm_bedroom_motion_upload_palette(motion, UINT32_C(0x800d45d4), tiles);
    uint32_t address;
    return motion && tiles && orb < 7 && word(motion, normal_table + 4*orb, &address) &&
           upload(motion, address, 1, tiles);
}

int bfm_grillin_blade_palette(const BfmBedroomMotion *motion, unsigned orb, uint16_t tiles[512][1024])
{
    return bfm_field_blade_palette(motion, UINT32_C(0x80190cd4), orb, tiles);
}

void bfm_bedroom_motion_free(BfmBedroomMotion *m)
{
    free(m);
}
