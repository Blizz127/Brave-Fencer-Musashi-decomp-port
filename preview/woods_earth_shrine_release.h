/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_RELEASE_H
#define BFM_WOODS_EARTH_SHRINE_RELEASE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BFM_EARTH_SEALED, BFM_EARTH_SHAKING, BFM_EARTH_RISING, BFM_EARTH_HOVERING,
    BFM_EARTH_UNROLLING, BFM_EARTH_SETTLING, BFM_EARTH_FLOATING, BFM_EARTH_TAKEN,
    BFM_EARTH_RESTORING, BFM_EARTH_WAITING_EXIT, BFM_EARTH_RELEASED
} BfmEarthReleaseStage;

typedef enum {
    BFM_EARTH_SOUND, BFM_EARTH_SCORE, BFM_EARTH_SPECIAL_PALETTE,
    BFM_EARTH_CREST_STAGE, BFM_EARTH_DUST, BFM_EARTH_SHATTER, BFM_EARTH_TRAIL,
    BFM_EARTH_RELEASE_SHAKE,
    BFM_EARTH_LOAD_RELEASE, BFM_EARTH_LOAD_RETURN, BFM_EARTH_TOUCH_SCROLL,
    BFM_EARTH_FIELD_COMMAND, BFM_EARTH_AWARD_ACTION, BFM_EARTH_BEFORE_AWARD,
    BFM_EARTH_AFTER_AWARD, BFM_EARTH_PLAYER_READY, BFM_EARTH_AWARD_CONFIRMED,
    BFM_EARTH_MESSAGE_CLOSED, BFM_EARTH_CAMERA_READY
} BfmEarthReleaseEvent;

typedef struct {
    const uint8_t *scene;
    size_t bytes, cursor;
    BfmEarthReleaseStage stage;
    int16_t xyz[3], original_y, tablet_dx, scale[2];
    int32_t y_fraction, speed_y, acceleration_y;
    unsigned timer, shake_ticks, animation_age, delay, animation_duration, script_done;
    unsigned release_loaded, waiting, award_pending, light_override, crest_present;
} BfmEarthShrineRelease;

/* Notifications SOUND through RELEASE_SHAKE are delivered once. LOAD_RELEASE
 * and LOAD_RETURN poll the actual mounted resource operations. TOUCH_SCROLL
 * tests the ordinary player volume against the two mounted child cross-sections
 * at release->xyz. FIELD_COMMAND uses the original room command kind and
 * operand, with borrowed scene bytes for messages and relative camera records.
 * Its return value means the command was accepted; repeat a busy request
 * without applying its effects twice. Readiness predicates return actual status.
 * AWARD_ACTION queues the distinct player action in the mounted descriptor.
 * AWARD_CONFIRMED must report its successful completion marker for this visit.
 * For notifications kind is the cue, palette row (0 restores), or crest stage.
 * CREST_STAGE 1 returns whether the optional effect was allocated.
 * SCORE instead supplies the stream in kind and selector in operand. The
 * score notification requests selector 0x30; the original child does not
 * wait for that sound request in place of its independent resource operation. */
typedef int (*BfmEarthReleaseHost)(void *context, const BfmEarthShrineRelease *release,
    BfmEarthReleaseEvent event, unsigned kind, uint32_t operand,
    const uint8_t *data, size_t bytes);

/* Begin once, from BFM_EARTH_SHRINE_ACCEPTED on the ordinary weapon receiver.
 * Start with a zero-initialized state and the mounted child's world XYZ.
 * The scene remains borrowed for the lifetime of the room visit. */
int bfm_earth_shrine_release_begin(BfmEarthShrineRelease *release,
    const uint8_t *scene, size_t bytes, const int16_t child_xyz[3],
    BfmEarthReleaseHost host, void *context);

/* Exactly once per effective actor update, including when a player action or
 * message is outstanding. Supply Musashi's current unprojected world XYZ.
 * Returns 0 for invalid state or an unsupported mounted program. */
int bfm_earth_shrine_release_step(BfmEarthShrineRelease *release,
    const int16_t player_xyz[3], BfmEarthReleaseHost host, void *context);

#endif
