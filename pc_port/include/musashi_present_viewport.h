#ifndef MUSASHI_PRESENT_VIEWPORT_H
#define MUSASHI_PRESENT_VIEWPORT_H

/* Center the chosen aspect in the drawable. A positive source height
 * requests whole-number vertical scaling whenever it fits. */
static inline void musashi_present_viewport(int win_w, int win_h, double aspect,
                                           int src_h, int vp[4]) {
    int k;
    if (!vp) return;
    if (win_w <= 0 || win_h <= 0) {
        vp[0] = vp[1] = vp[2] = vp[3] = 0;
        return;
    }
    if (aspect <= 0.0) aspect = 4.0 / 3.0;
    k = src_h > 0 ? win_h / src_h : 0;
    while (k > 0 && (int)(k * src_h * aspect + 0.5) > win_w) k--;
    if (k > 0) {
        vp[3] = k * src_h;
        vp[2] = (int)(vp[3] * aspect + 0.5);
    } else if ((double)win_w / win_h > aspect) {
        vp[3] = win_h;
        vp[2] = (int)(win_h * aspect + 0.5);
    } else {
        vp[2] = win_w;
        vp[3] = (int)(win_w / aspect + 0.5);
    }
    vp[0] = (win_w - vp[2]) / 2;
    vp[1] = (win_h - vp[3]) / 2;
}

#endif
