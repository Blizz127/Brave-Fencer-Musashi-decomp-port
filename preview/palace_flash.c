/* SPDX-License-Identifier: MIT */
#include "palace_flash.h"

/* At 37,800 Hz the fast and slow voices respectively advance 7/12 and
 * 1267/3072 of a source sample per frame. Both stop at the first end block. */
enum { SLOW_NUM = 1267, SLOW_DEN = 3072 };

static size_t one_shot_frames(const BfmDecodedSpuSample *sample,
                              unsigned numerator, unsigned denominator)
{
    if (!sample || !sample->pcm || sample->loop_start != sample->frames)
        return 0;
    size_t whole = sample->frames / numerator;
    size_t tail = (sample->frames % numerator * denominator + numerator - 1) / numerator;
    if (whole > (SIZE_MAX - tail) / denominator)
        return SIZE_MAX;
    return whole * denominator + tail;
}

size_t bfm_palace_flash_frames(const BfmDecodedSpuSample *sample)
{
    return one_shot_frames(sample, SLOW_NUM, SLOW_DEN);
}

static int32_t pitched(const BfmDecodedSpuSample *sample, size_t frame,
                       unsigned numerator, unsigned denominator)
{
    unsigned step = (unsigned)(frame % denominator) * numerator;
    size_t index = frame / denominator * numerator + step / denominator;
    if (index >= sample->frames)
        return 0;
    int32_t a = sample->pcm[index];
    int32_t b = index + 1 < sample->frames ? sample->pcm[index + 1] : 0;
    return a + (b - a) * (int32_t)(step % denominator) / (int32_t)denominator;
}

int16_t bfm_palace_flash_frame(const BfmDecodedSpuSample *sample, size_t frame)
{
    if (!sample || !sample->pcm || sample->loop_start != sample->frames)
        return 0;
    int32_t fast = pitched(sample, frame, 7, 12);
    int32_t slow = pitched(sample, frame, SLOW_NUM, SLOW_DEN);

    /* The slow voice gains four envelope units every eight 44,100-Hz
     * samples. The short initial attack of the faster voice is approximated
     * as immediate. Its first observed device envelope was already full. */
    int32_t envelope = 32767;
    if (frame < 56174)
        envelope = (int32_t)(frame * 7 / 48) * 4;
    return (int16_t)((fast + slow * envelope / 32767) * 6800 / 16384);
}

size_t bfm_palace_after_flash_frames(const BfmDecodedSpuSample *sample)
{
    return one_shot_frames(sample, 7, 12);
}

int16_t bfm_palace_after_flash_frame(const BfmDecodedSpuSample *sample, size_t frame)
{
    if (!sample || !sample->pcm || sample->loop_start != sample->frames)
        return 0;
    return (int16_t)(pitched(sample, frame, 7, 12) * 1915 / 16384);
}
