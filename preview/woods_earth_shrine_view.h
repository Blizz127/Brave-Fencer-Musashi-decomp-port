/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_VIEW_H
#define BFM_WOODS_EARTH_SHRINE_VIEW_H

#include "bedroom_figure.h"

typedef struct BfmWoodsEarthShrineView BfmWoodsEarthShrineView;

/* Load the summit tablet, engraved panel and animated crest from SC02/11.
 * The mounted scene and the field's original texture pages outlive the view. */
BfmWoodsEarthShrineView *bfm_woods_earth_shrine_view_open(const uint8_t *scene, size_t bytes);
void bfm_woods_earth_shrine_view_free(BfmWoodsEarthShrineView *shrine);

/* The room supplies the number of completed child animation updates since
 * spawn. Hide the tablet when the child releases the scroll or on an earned
 * return visit. The field terrain behind it continues to draw in either case. */
int bfm_woods_earth_shrine_view_draw(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, int tablet_visible,
    unsigned animation_field);

/* Only the two stone models shake during an accepted hit. The independent
 * engraving stays at its mounted location on its last idle animation key. */
int bfm_woods_earth_shrine_view_attached(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, int tablet_dx, unsigned crest_age);

typedef enum {
    BFM_EARTH_SCROLL_PANEL, BFM_EARTH_SCROLL_UNFOLDING, BFM_EARTH_SCROLL_ECHO
} BfmWoodsEarthScrollForm;

/* Draw the current child only after the release tiles are resident. The
 * room supplies its actual world position, completed animation updates and
 * visible size (8 initially, dropping to 0 as Musashi receives the scroll). */
int bfm_woods_earth_shrine_view_scroll(const BfmWoodsEarthShrineView *shrine,
    BfmBedroomFigure *figure, const BfmBedroomView *view, unsigned texture_member,
    BfmWoodsEarthScrollForm form, const int16_t xyz[3], unsigned animation_updates,
    unsigned size_eighths);

/* Draw the local light while the primary child owns it. While attached its
 * world origin is 160 units above the tablet; once detached it follows the
 * child at its current position. RGB uses red in the low byte. */
int bfm_woods_earth_shrine_view_glow(BfmBedroomFigure *figure, const BfmBedroomView *view,
    const int16_t xyz[3], int tablet_attached, int scale_x, int scale_y, uint32_t rgb);

#endif
