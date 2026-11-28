/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_ARRIVAL_MOTION_H
#define BFM_VILLAGE_ARRIVAL_MOTION_H

#include "bedroom_motion.h"
#include "forest_maze_gate.h"
#include "bedroom_travel.h"
#include "forest_dust.h"
#include "forest_bank_hop.h"
#include "forest_bank_thorn_hurt.h"
#include "forest_bank_hop_pose.h"
#include "field_input.h"
#include "field_rest.h"
#include "woods_3026_box_hit.h"
#include "village_mine_shaft_fan.h"
#include "village_mine_shaft_jump_blade.h"

typedef struct BfmVillageMotion BfmVillageMotion;
struct BfmVillageMineGate;

typedef enum {
    BFM_VILLAGE_RIDE, BFM_VILLAGE_DROP, BFM_VILLAGE_IMPACT,
    BFM_VILLAGE_SETTLE, BFM_VILLAGE_WAIT_MUSIC, BFM_VILLAGE_TURN,
    BFM_VILLAGE_FIRST_WALK, BFM_VILLAGE_FIRST_MESSAGE,
    BFM_VILLAGE_SECOND_WALK, BFM_VILLAGE_SECOND_MESSAGE, BFM_VILLAGE_CONTROL
} BfmVillagePhase;

enum {
    BFM_VILLAGE_HIT = 1u, BFM_VILLAGE_CAMERA = 2u, BFM_VILLAGE_OPEN_MESSAGE = 4u,
    BFM_VILLAGE_SHOW_HUD = 8u, BFM_VILLAGE_RELEASE = 16u, BFM_VILLAGE_TRANSFER = 32u,
    BFM_VILLAGE_WELL_SOUND_EVENT = 64u, BFM_VILLAGE_WATER_RETURN = 128u,
    BFM_VILLAGE_WATER_SPLASH = 256u, BFM_VILLAGE_WATER_HIT = 512u,
    BFM_VILLAGE_POLE_CATCH = 1024u /* First field of a successful mine grip. */
};

typedef struct {
    BfmBedroomRoot root;
    BfmBedroomPose pose;
    BfmVillagePhase phase;
    unsigned controlled, events;
    uint32_t message;
    unsigned movement; /* Cable updates since mounting, retained through impact. */
    uint16_t destination, wall_contact;
    uint8_t entrance;
    /* The logical root still drives the camera while its submerged body is hidden. */
    unsigned water_hidden;
} BfmVillagePlayer;

/* Borrow the unpacked SC03/2 room and the resident MAIN/10 type-0x101 payload. */
BfmVillageMotion *bfm_village_motion_open(const uint8_t *scene, size_t scene_bytes,
    const uint8_t *main, size_t main_bytes, uint32_t room, unsigned entrance);
/* The marked floor queues a descriptor. The destination restores integer XYZ
 * and heading captured before the final movement pass in the departing field. */
const uint8_t *bfm_village_motion_transfer(const BfmVillageMotion *motion);
/* Publish a blocking room actor's corrected endpoint without advancing the pose again. */
int bfm_village_motion_maze_contact(BfmVillageMotion *motion, const int16_t endpoint[3]);
const BfmForestMazeGate *bfm_village_motion_maze_gate(const BfmVillageMotion *motion);
/* Bank entrance 2 returns from the maze. Entrances 0 and 1 lead in from town. */
BfmVillageMotion *bfm_village_motion_bank(const uint8_t *scene, size_t bytes,
                                          const uint8_t *main, size_t main_bytes,
                                          unsigned entrance, unsigned story);
BfmVillageMotion *bfm_village_motion_woods(const uint8_t *scene, size_t bytes,
                                           const uint8_t *main, size_t main_bytes);
/* Open Cleft's mountain-side entrance or the adjoining mountain field. A
 * recorded box destination resumes at its own marker without an entry walk. */
BfmVillageMotion *bfm_village_motion_box_field(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36]);
/* Hell's Valley entrance zero walks inward from its Cleft-side gate, then
 * hands walking and jumping back to the normal field controller. */
BfmVillageMotion *bfm_village_motion_valley(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36], const MusashiDiscMedia *disc);
/* From the lower Grillin gate, the mine's own entrance row positions Musashi. */
BfmVillageMotion *bfm_village_motion_mine(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, unsigned story,
    const MusashiDiscMedia *disc);
/* Enter the switchback from the western gallery or the eastern shaft. */
BfmVillageMotion *bfm_village_motion_switchback(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc);
/* The upper west passage and the recorded Memory Box have separate stations. */
BfmVillageMotion *bfm_village_motion_mine_shaft(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc);
BfmVillageMotion *bfm_village_motion_mine_lake(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc);
/* Enable the shaft's second numbered passage only when its lake receiver is installed. */
void bfm_village_motion_mine_shaft_lake_exit(BfmVillageMotion *motion, int enabled);
struct BfmVillageMineLakeFields;
/* Borrow crystal shapes from the last completed actor turn for the player sweep. */
void bfm_village_motion_mine_lake_fields(BfmVillageMotion *motion,
    const struct BfmVillageMineLakeFields *fields, unsigned actor_tick);
/* Supply the current posted by the preceding actor turn, or NULL. */
void bfm_village_motion_mine_lake_begin_field(BfmVillageMotion *motion, const int16_t current[3]);
struct BfmVillageMinePit;
uint16_t bfm_village_motion_mine_pit_material(const BfmVillageMotion *motion);
/* After lake_begin_field, service the preceding foot's reason before ordinary controls.
 * A locked frame owns this player update, including the field which debits HP. */
int bfm_village_motion_mine_pit(BfmVillageMotion *motion, struct BfmVillageMinePit *pit,
    struct BfmGameplayRun *run, BfmForestBankThornInput input, int accept_floor,
    BfmForestBankThornFrame *frame);
/* Publish the raw foot for the defeat view and release the regular shadow. */
int bfm_village_motion_mine_release_rider(BfmVillageMotion *motion);
BfmVillageMotion *bfm_village_motion_mine_shaft_saved(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, const uint8_t destination[36],
    const MusashiDiscMedia *disc);
int bfm_village_motion_mine_shaft_box(BfmVillageMotion *motion, const BfmWoods3026BoxHit *box,
                                      unsigned field_tick, const int16_t previous[3]);
/* Begin before ordinary movement and turn the rotor after the player is published. */
void bfm_village_motion_mine_shaft_begin_field(BfmVillageMotion *motion, BfmMineShaftFan *fan,
                                               unsigned field_tick);
int bfm_village_motion_mine_shaft_turn_fan(BfmVillageMotion *motion, BfmMineShaftFanFrame *frame);
int bfm_village_motion_mine_orb(const BfmVillageMotion *motion, unsigned orb,
                                uint16_t tiles[512][1024]);
struct BfmVillageMineProps;
void bfm_village_motion_mine_props(BfmVillageMotion *motion, const struct BfmVillageMineProps *props,
                                   unsigned actor_tick);
/* The grounded lake blade receives the standing or walking action slot. */
unsigned bfm_village_motion_mine_blade_slot(const BfmVillageMotion *motion);
int bfm_village_motion_mine_blade_hold(BfmVillageMotion *motion, int finishing, int redirect_yaw);
int bfm_village_motion_step_mine_lake_field(BfmVillageMotion *motion, BfmFieldInput input,
    int camera_yaw, const BfmSaveRecord *live, struct BfmGameplayRun *run);
const BfmShaftBladeFrame *bfm_village_motion_mine_shaft_blade(const BfmVillageMotion *motion);
/* The western walkway tests actual support for the stationary pose and strafe. */
int bfm_village_motion_switchback_stance(BfmVillageMotion *m, unsigned facing, unsigned stride_yaw,
    int moving, int tired, int *supported);
struct BfmVillageMineWheels;
void bfm_village_motion_mine_begin_field(BfmVillageMotion *motion, struct BfmVillageMineWheels *wheels);
int bfm_village_motion_mine_turn_wheels(BfmVillageMotion *motion, int gain[3]);
int bfm_village_motion_mine_rider(const BfmVillageMotion *motion);
int bfm_village_motion_step_mine_field(BfmVillageMotion *motion, int x, int y, int camera_yaw,
    int cross_pressed, int cross_held, struct BfmGameplayRun *run);
struct BfmValleyBincho;
struct BfmValleyGateDoor;
int bfm_village_motion_can_examine(const BfmVillageMotion *motion);
void bfm_village_motion_mine_gate(BfmVillageMotion *motion, const struct BfmVillageMineGate *gate,
                                 unsigned actor_tick);
void bfm_village_motion_valley_obstacles(BfmVillageMotion *motion,
    const struct BfmValleyBincho *bincho, const struct BfmValleyGateDoor *door);
struct BfmHellsValleyBoulders;
void bfm_village_motion_valley_boulders(BfmVillageMotion *motion,
    const struct BfmHellsValleyBoulders *rocks, unsigned actor_tick);
int bfm_village_motion_step_valley_field(BfmVillageMotion *motion, int x, int y, int camera_yaw,
    int cross_pressed, int cross_held, struct BfmGameplayRun *run);
int bfm_village_motion_valley_orb(const BfmVillageMotion *motion, unsigned orb,
                                 uint16_t tiles[512][1024]);
/* True during the free-running tail of an ordinary room entrance walk. */
int bfm_village_motion_ingress_clock_running(const BfmVillageMotion *motion);
int bfm_village_motion_woods_departing(const BfmVillageMotion *motion);
/* Bind the mounted exit trigger for the current woods story visit. */
int bfm_village_motion_trail_gate(BfmVillageMotion *motion, unsigned story);
struct BfmVillageGround;
const struct BfmVillageGround *bfm_village_motion_ground(const BfmVillageMotion *motion);
BfmVillageMotion *bfm_village_motion_arrive(const BfmVillageMotion *departed, const uint8_t *scene,
                                            size_t bytes, uint32_t room_address);
BfmVillageMotion *bfm_village_motion_resume(const uint8_t *scene, size_t bytes,
                                            uint32_t room_address,
                                            const uint8_t *main, size_t main_bytes);
/* Split the east arch from the ordinary inter-street and bridge transfers.
 * The castle owner decides which menu destinations it can receive. */
const uint8_t *bfm_village_motion_castle_exit(const BfmVillageMotion *motion);
/* Indoor returns use the receiving street's mounted anchor and heading. */
BfmVillageMotion *bfm_village_motion_return_to_street(const uint8_t *scene, size_t bytes,
                                    uint32_t room_address, const uint8_t *main, size_t main_bytes,
                                    unsigned entrance);
/* Submit one scripted root and advance the player model once, then use place
 * on release to resume ordinary movement from the script endpoint. */
int bfm_village_motion_porch(BfmVillageMotion *motion, const BfmBedroomRoot *root, int knock);
/* Issue DRAW_WATER, RECEIVE or LOWER once at the corresponding scene edge.
 * NONE continues the gesture or follows the root's locomotion edge. DRAW_WATER
 * and LOWER pulse pose.finished at completion. RECEIVE holds until dismissal. */
int bfm_village_motion_well(BfmVillageMotion *motion, const BfmBedroomRoot *root,
                            BfmBedroomAction action);
int bfm_village_motion_staged(BfmVillageMotion *motion, const BfmBedroomRoot *root);
struct BfmRootrickHero;
struct BfmRootrickHeroObstacle;
/* Bind to the root from the last staged challenge field, then publish Musashi
 * from his own wall controller using the same mounted motion, pose and shadow. */
int bfm_village_motion_rootrick_begin(BfmVillageMotion *motion, struct BfmRootrickHero *hero,
                                      const int64_t staged_position[3]);
int bfm_village_motion_rootrick_step(BfmVillageMotion *motion, struct BfmRootrickHero *hero,
    const struct BfmRootrickHeroObstacle *before, const struct BfmRootrickHeroObstacle *after,
    BfmFieldInput input, int camera_yaw,
    unsigned field_tick, int can_climb, int32_t *player_y_q16);
/* Advance a mounted body program and the current face once. Repeating the
 * program retains its body cursor. ended reports the published end marker. */
int bfm_village_motion_staged_program(BfmVillageMotion *motion, const BfmBedroomRoot *root,
                                      uint32_t program, int *ended);
/* Start the body again even when the previous speech uses the same program. */
int bfm_village_motion_restart_staged_program(BfmVillageMotion *motion, const BfmBedroomRoot *root,
                                              uint32_t program, int *ended);
int bfm_village_motion_finish_staged_program(BfmVillageMotion *motion);
int bfm_village_motion_staged_action(BfmVillageMotion *motion, const BfmBedroomRoot *root,
                                    BfmBedroomAction action);
/* Transfer the complete live coordinate to and from a scripted field mover.
 * Updating position also republishes the whole root and its shadow. */
int bfm_village_motion_position_q16(const BfmVillageMotion *motion, int32_t xyz[3]);
int bfm_village_motion_staged_position(BfmVillageMotion *motion, const int32_t xyz[3]);
/* Submit a mounted external key and its face/weapon state at the grounded
 * scripted position. pose addresses the first joint, after the packet header. */
int bfm_village_motion_staged_model(BfmVillageMotion *motion, const BfmBedroomRoot *root,
    uint32_t pose, unsigned face_slot, int show_fusion, int show_lumina);
int bfm_village_motion_place(BfmVillageMotion *motion, const BfmBedroomRoot *root);
/* Return a staged actor to ordinary control at his current physical position.
 * Unlike a new placement, this retains fractions and the last fall velocity. */
int bfm_village_motion_release_staged(BfmVillageMotion *motion);
/* A director after the player's actor pass releases input without a second animation tick. */
int bfm_village_motion_release_staged_field(BfmVillageMotion *motion);
const BfmVillagePlayer *bfm_village_motion_player(const BfmVillageMotion *motion);
/* Once per gameplay update. Pad axes are -127..127, right and down positive.
 * Camera yaw is target minus eye in X/Z, with +Z at zero. */
int bfm_village_motion_step(BfmVillageMotion *motion, int stick_x, int stick_y, int camera_yaw);
/* Called instead of the ordinary step while Hop owns the controlled bank
 * player. Returns 1 for a consumed field, 0 for ordinary control, -1 on error.
 * Each takeoff and terrain rebound emits one launch event. */
int bfm_village_motion_bank_hop(BfmVillageMotion *motion, BfmForestBankHop *hop,
                                BfmForestBankHopPose *poses, BfmForestStatus *player,
                                BfmFieldInput field, int camera_yaw, uint32_t actor_flags,
                                int *takeoff);
uint16_t *bfm_village_motion_bank_pending(BfmVillageMotion *motion);
/* Sample ordinary safe footing before the previous field's material is received.
 * A thorn owns the actual Q16 foot and body through the recovery update. */
int bfm_village_motion_bank_thorn(BfmVillageMotion *motion, BfmForestBankThornHurt *hurt,
                                  BfmGameplayRun *run, BfmForestBankThornInput input,
                                  BfmForestBankThornFrame *frame);
struct BfmGameplayRun;
int bfm_village_motion_step_field(BfmVillageMotion *motion, int stick_x, int stick_y,
                                  int camera_yaw, struct BfmGameplayRun *run);
/* Consume an already accepted grounded examine instead of ordinary movement.
 * The same field advances the body and face once before scripted takeover. */
int bfm_village_motion_examine_tick(BfmVillageMotion *motion, struct BfmGameplayRun *run);
/* The western mine exit starts its pad script when the outgoing room station attaches it. */
void bfm_village_motion_woods_exit_script(BfmVillageMotion *motion, int enabled);
/* After contact at the reciprocal woods passage, follow its mounted facing
 * and scripted pad until the room transition replaces the departing field. */
int bfm_village_motion_woods_departure_step(BfmVillageMotion *motion, int camera_yaw,
                                           struct BfmGameplayRun *run);
/* Mount Twinpeak's body programs when opening its playable field. Consume
 * a fresh Cross and the held pad independently on ordinary field updates. */
int bfm_village_motion_box_controls(BfmVillageMotion *motion, const MusashiDiscMedia *disc);
/* The attached type-220 actor's one-based row, or zero during free movement. */
/* The active jumping or falling action, including the summit race handoff. */
int bfm_village_motion_box_air_action(const BfmVillageMotion *motion);
/* Set by the ordinary supported standing action on this field. */
int bfm_village_motion_box_reported_stand(const BfmVillageMotion *motion);
unsigned bfm_village_motion_box_pole(const BfmVillageMotion *motion);
int bfm_village_motion_box_step(BfmVillageMotion *motion, int stick_x, int stick_y,
                                int camera_yaw, int cross_pressed, int cross_held,
                                struct BfmGameplayRun *run);
/* A grounded Twinpeak blade owns the feet and yaw even when the original
 * walking controller was braking. Recheck stationary terrain and advance the
 * face. On a normal finish enter stance at zero speed. A lost support or a
 * steep face instead yields the ordinary fall/slide to the caller. */
int bfm_village_motion_box_blade_hold(BfmVillageMotion *motion, int finishing);
struct BfmWoodsLilies;
int bfm_village_motion_step_woods(BfmVillageMotion *motion, int stick_x, int stick_y,
                                 int camera_yaw, int cross_pressed, int cross_held,
                                 int actors_running, struct BfmWoodsLilies *lilies);
/* The room owns competing actions such as a cast or guard. Ordinary grounded
 * movement decides whether the standing action actually ran this field. */
int bfm_village_motion_step_woods_field(BfmVillageMotion *motion, int stick_x, int stick_y,
    int camera_yaw, int cross_pressed, int cross_held, int actors_running,
    struct BfmWoodsLilies *lilies, struct BfmGameplayRun *run, int idle_allowed);
/* During Leno's greeting, replace the ordinary woods step with one stationary
 * turn or gesture update. End when the dog returns control after its last page. */
int bfm_village_motion_leno_greeting(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
    int actors_running, const int16_t target[3], int speaking);
int bfm_village_motion_end_leno_greeting(BfmVillageMotion *motion);
struct BfmWoodsCarryFrame;
/* Replaces the ordinary woods step for one lift/overhead/throw field. */
int bfm_village_motion_carry(BfmVillageMotion *motion, int stick_x, int stick_y, int camera_yaw,
                             int actors_running, struct BfmWoodsLilies *lilies,
                             const struct BfmWoodsCarryFrame *frame);
/* Apply the push pose after the existing ordinary walking and contact pass. */
int bfm_village_motion_brace(BfmVillageMotion *motion, const struct BfmWoodsCarryFrame *frame);

typedef enum {
    BFM_WOODS_HURT_START, BFM_WOODS_HURT_HOLD, BFM_WOODS_GUARD_RECOIL
} BfmWoodsReactionMotion;

typedef struct {
    int lost_support, landed;
    uint16_t slope, material;
} BfmWoodsMotionContact;

/* Replace step_woods for one reaction field. A hurt holds the player's
 * root. Guard recoil supplies a signed world X/Z displacement in Q16.
 * The bank supplies no lily platforms. In Cleft Woods they advance once.
 * Solid actors use the resulting foot segment.
 * Slope and material describe the swept floor. An ordinary wall alone
 * changes neither support edge. */
int bfm_village_motion_woods_reaction(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
    BfmWoodsReactionMotion kind, int32_t dx_q16, int32_t dz_q16, BfmWoodsMotionContact *contact);
/* Carry the planted hero with the platform and classify the support beneath a
 * grounded sword cut. A missing or steep support selects fall or slide at once.
 * The finishing field returns to ordinary stance without another support test. */
int bfm_village_motion_woods_cut(BfmVillageMotion *motion, struct BfmWoodsLilies *lilies,
                                unsigned cancel_countdown, int finishing, int *supported);
int bfm_village_motion_ability_pose(BfmVillageMotion *motion, unsigned face_slot, int finished);
int bfm_village_motion_combat_face(BfmVillageMotion *motion, unsigned slot);
/* On a surviving ordinary hurt, select idle or the first fall pose at the
 * current root. The next ordinary field samples input and integrates gravity. */
int bfm_village_motion_woods_recover(BfmVillageMotion *motion, int resume_fall);
/* Recheck actor and stationary terrain support at the current feet after a
 * stocked recovery. The normal field step still consumes its neutral pad. */
/* Recheck terrain and select the ordinary falling, sliding or standing action. */
int bfm_village_motion_woods_finish_hurt(BfmVillageMotion *motion, int tired);
int bfm_village_motion_woods_revive(BfmVillageMotion *motion, int tired);
void bfm_village_motion_woods_condition(BfmVillageMotion *motion, int tired);
int bfm_village_motion_woods_sliding(const BfmVillageMotion *motion);
int bfm_village_motion_grounded(const BfmVillageMotion *motion);
/* Replace one ordinary field update while charging, lying down or waking.
 * The actor stays at the field root. A completed drop or wake returns 2,
 * a continuing phase returns 1 and an invalid update returns 0. */
int bfm_village_motion_field_rest(BfmVillageMotion *motion, BfmFieldRestPose phase,
                                  unsigned tick);

/* The room sets this from its active Water Scroll and story/control state
 * before moving. A previously accepted plunge continues to its shore. */
void bfm_village_motion_water_bypass(BfmVillageMotion *motion, int bypass);
/* Read after the ordinary woods step, using the world field that ran it. */
BfmForestDustEmission bfm_village_motion_foot_dust(const BfmVillageMotion *motion,
                                                   unsigned actor_tick);
struct BfmVillageResidents;
/* Resolve contact against the actor transform from the previous field update. */
int bfm_village_motion_correct_contact(BfmVillageMotion *motion, const int16_t xyz[3]);
int bfm_village_motion_resident_contact(BfmVillageMotion *motion,
    const struct BfmVillageResidents *residents, unsigned field_tick, const int16_t previous[3]);
/* Signal music readiness or closure of the current message before the next step. */
int bfm_village_motion_continue(BfmVillageMotion *motion);
/* Start only after the resident has accepted Talk. The mounted conversation
 * specifies the walking offset in the resident's local coordinates. */
int bfm_village_motion_begin_talk(BfmVillageMotion *motion, const int16_t resident_xyz[3],
                                  unsigned resident_yaw);
/* Start a mounted conversation that stages the player at a world-space
 * approach foot before turning toward the speaker. */
int bfm_village_motion_begin_talk_at(BfmVillageMotion *motion, const int16_t resident_xyz[3],
                                     const int16_t approach_xyz[3]);
/* A repeat line with no staging or facing cue holds the current foot
 * and heading. Release with bfm_village_motion_end_talk on real close. */
int bfm_village_motion_begin_talk_hold(BfmVillageMotion *motion);
/* Pass the previous view's eye and target while its conversation zoom moves. */
int bfm_village_motion_view_yaw(const int32_t eye[3], const int32_t target[3]);
int bfm_village_motion_begin_read(BfmVillageMotion *motion, const int16_t point[3]);
int bfm_village_motion_end_read(BfmVillageMotion *motion);
int bfm_village_motion_talk_ready(const BfmVillageMotion *motion);
int bfm_village_motion_end_talk(BfmVillageMotion *motion);
int bfm_village_motion_face(const BfmVillageMotion *motion, uint16_t tiles[512][1024]);
int bfm_village_motion_lower_orb(const BfmVillageMotion *motion, unsigned orb,
                                uint16_t tiles[512][1024]);
int bfm_village_motion_twinpeak_orb(const BfmVillageMotion *motion, unsigned orb,
                                   uint16_t tiles[512][1024]);
void bfm_village_motion_free(BfmVillageMotion *motion);

/* Somnolent Forest's south clearing, entered from the bank. Both south
 * arrivals run their mounted entry before accepting walking and Cross. */
BfmVillageMotion *bfm_village_motion_maze(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, unsigned entrance, const MusashiDiscMedia *disc);
int bfm_village_motion_step_maze_field(BfmVillageMotion *motion, int x, int y, int camera_yaw,
                                       int cross_pressed, int cross_held, struct BfmGameplayRun *run);

/* Advance the outgoing bank/clearing hero until the field operator changes
 * rooms. The queued real door and the original point of contact are retained. */
int bfm_village_motion_maze_departing(BfmVillageMotion *motion);

/* The maze entrance terminator resumes an equipped passive in place,
 * after this field has already stepped the ordinary body and face. */
int bfm_village_motion_maze_resume_hop(BfmVillageMotion *motion, BfmForestBankHop *hop,
                                       BfmForestBankHopPose *poses);

#endif
