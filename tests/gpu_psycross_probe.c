/* Real PsyCross context; synthetic VRAM/environment fixtures, not game proof. */
#include "musashi_gpu_psycross.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../tools/third_party/psycross/src/gpu/PsyX_GPU.h"
#include "PsyX/PsyX_render.h"
#include "PsyX/PsyX_public.h"
void musashi_psyx_shutdown(void); /* pc_port/psyx_shutdown.cpp: GL teardown before window */

extern int g_vertexIndex;
extern char begin_scene_flag;

typedef struct Foreign { MusashiGpuPsyCross *gpu; MusashiGpuBackend *backend; } Foreign;
static int foreign(void *userdata) {
    Foreign *f = userdata;
    uint32_t before_window = f->gpu->texture_window;
    uint16_t read = 0x1234;
    MusashiGpuFill fill = {.command=0x020000ffu,.xy=0,.wh=0x00010010u};
    assert(musashi_gpu_fill_derive(&fill));
    assert(!f->backend->fill_vram(f->backend->userdata,&fill));
    assert(!musashi_gpu_psycross_read_vram(f->gpu,0,0,1,1,&read,1));
    assert(read==0x1234);
    assert(f->backend->ready(f->backend->userdata) < 0);
    assert(!f->backend->reset(f->backend->userdata));
    assert(!f->backend->draw_mode(f->backend->userdata, 0xe1000000u));
    assert(!f->backend->display_enable(f->backend->userdata, 0));
    assert(!f->backend->clear_fifo(f->backend->userdata));
    assert(!f->backend->environment(f->backend->userdata, 0xe2000001u));
    assert(f->gpu->texture_window == before_window);
    assert(!musashi_gpu_psycross_present(f->gpu));
    assert(!musashi_gpu_psycross_shutdown(f->gpu));
    return 0;
}

static void fill_probe(MusashiGpuPsyCross *gpu, MusashiGpuBackend *backend) {
    static uint16_t pixels[1024 * 512];
    MusashiGpuFill fill = {.command=0x02ffffffu, .xy=0, .wh=0x01ff03ffu};
    MusashiGpuController controller = {0};
    uint64_t before;
    unsigned x, y;
    assert(musashi_gpu_controller_init(&controller, backend));
    assert(musashi_gpu_fill_derive(&fill));
    assert(backend->fill_vram(backend->userdata, &fill));
    fill.xy = 511u << 16; fill.wh = 0x000103ffu;
    assert(musashi_gpu_fill_derive(&fill));
    assert(backend->fill_vram(backend->userdata, &fill));
    /* First two actual GP0 words cannot paint. E6 has no effect onfill. */
    assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0, 0xe6000003u));
    assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0, 0x02000000u));
    assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0, 0));
    assert(musashi_gpu_psycross_read_vram(gpu,0,0,1024,512,pixels,1024*512));
    for (x=0;x<1024*512;++x) assert(pixels[x]==0x7fff);
    assert(musashi_gpu_controller_write32(&controller,MUSASHI_GPU_GP0,0x01e00280u));
    assert(musashi_gpu_psycross_read_vram(gpu,0,0,1024,512,pixels,1024*512));
    for (y=0;y<512;++y) for(x=0;x<1024;++x)
        assert(pixels[y*1024+x]==((x<640 && y<480)?0:0x7fff));
    /* Wrapped mixed-color fill ignores current clip/offset/mask. */
    fill.command=0x02f9f8ffu; fill.xy=0x01ff03ffu; fill.wh=0x00020011u;
    assert(musashi_gpu_fill_derive(&fill));
    assert(fill.color==0x7fff && fill.x==1008 && fill.width==32);
    fill.command=0x02ff0007u;
    assert(musashi_gpu_fill_derive(&fill));
    assert(fill.color==0x7c00);
    assert(backend->fill_vram(backend->userdata,&fill));
    assert(musashi_gpu_psycross_read_vram(gpu,0,0,1024,512,pixels,1024*512));
    for(y=0;y<512;++y) for(x=0;x<1024;++x) {
        uint16_t expected=(x<640&&y<480)?0:0x7fff;
        if((y==0||y==511)&&(x<16||x>=1008)) expected=0x7c00;
        assert(pixels[y*1024+x]==expected);
    }
    before=gpu->filled_pixels;
    {
        uint64_t completions=gpu->completed_fills;
        uint16_t out=0x1234;
        gpu->command_active=1;
        assert(!backend->fill_vram(backend->userdata,&fill));
        assert(!musashi_gpu_psycross_read_vram(gpu,0,0,1,1,&out,1));
        gpu->command_active=0;
        begin_scene_flag=1;
        assert(!backend->fill_vram(backend->userdata,&fill));
        assert(!musashi_gpu_psycross_read_vram(gpu,0,0,1,1,&out,1));
        begin_scene_flag=0;
        assert(out==0x1234 && gpu->completed_fills==completions);
        gpu->completed_fills=UINT64_MAX;
        assert(!backend->fill_vram(backend->userdata,&fill));
        gpu->completed_fills=completions;
        gpu->filled_pixels=UINT64_MAX;
        assert(!backend->fill_vram(backend->userdata,&fill));
        gpu->filled_pixels=before;
    }
    fill.wh=0x02000400u;
    assert(musashi_gpu_fill_derive(&fill));
    assert(backend->fill_vram(backend->userdata,&fill));
    assert(gpu->filled_pixels==before);
    fill.color=1; /* Inconsistent derived descriptor refuses without effects. */
    assert(!backend->fill_vram(backend->userdata,&fill));
    pixels[0]=0x1234;
    assert(!musashi_gpu_psycross_read_vram(gpu,1023,511,2,1,pixels,2));
    assert(pixels[0]==0x1234);
    assert(!musashi_gpu_psycross_read_vram(gpu,0,0,2,1,pixels,1));
    assert(pixels[0]==0x1234);
    assert(gpu->filled_pixels==before);
    assert(controller.completed_fills==1 && controller.filled_pixels==640*480);
}

/* Sample the default framebuffer after blitting, before SDL swaps buffers. */
static void read_scanout_pixel(void *user, int width, int height) {
    GLint previous;
    glGetIntegerv(GL_READ_BUFFER, &previous);
    glReadBuffer(GL_BACK);
    glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, user);
    glReadBuffer((GLenum)previous);
}

static void display_page_probe(MusashiGpuPsyCross *gpu, MusashiGpuBackend *backend) {
    MusashiGpuController controller = {0};
    static const struct {
        unsigned display_y, draw_y;
        unsigned char red, blue;
    } frames[] = {{0, 240, 255, 0}, {240, 240, 0, 255}, {240, 0, 0, 255}};
    unsigned char pixel[4];
    unsigned i;
    int matched = 1;

    assert(musashi_gpu_controller_init(&controller, backend));
    /* Fill two independent 256x240 pages with red and blue. */
    for (i = 0; i < 2; ++i) {
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0,
                                              i ? 0x02ff0000u : 0x020000ffu));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0, (i * 240u) << 16));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0, (240u << 16) | 256u));
    }
    assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP1, 0x03000000u));
    musashi_gpu_psycross_set_overlay(read_scanout_pixel, pixel);
    for (i = 0; i < sizeof(frames) / sizeof(frames[0]); ++i) {
        unsigned y = frames[i].draw_y;
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0,
                                              0xe3000000u | (y << 10)));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0,
                                              0xe4000000u | ((y + 239u) << 10) | 255u));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP1,
                                              0x05000000u | (frames[i].display_y << 10)));
        memset(pixel, 0, sizeof(pixel));
        assert(musashi_gpu_psycross_present(gpu));
        if (pixel[0] != frames[i].red || pixel[1] != 0 || pixel[2] != frames[i].blue) {
            fprintf(stderr, "display y=%u, draw y=%u: RGB %u,%u,%u (expected %u,0,%u)\n",
                    frames[i].display_y, y, pixel[0], pixel[1], pixel[2],
                    frames[i].red, frames[i].blue);
            matched = 0;
        }
    }
    musashi_gpu_psycross_set_overlay(NULL, NULL);
    assert(matched);
}

/* RGB identifies the absolute VRAM column and row, including alternating lines. */
static uint16_t chart_color(unsigned x, unsigned y) {
    unsigned red = (x ^ (x >> 5)) & 31u;
    unsigned green = (y ^ (y >> 5)) & 31u;
    unsigned blue = (y & 1u) ? 31u : 0u;
    return (uint16_t)(red | (green << 5) | (blue << 10));
}

typedef struct ChartView {
    unsigned origin_x, origin_y, rows;
    unsigned differences;
} ChartView;

static void read_chart(void *userdata, int width, int height) {
    ChartView *view = userdata;
    size_t size = (size_t)width * height * 4u;
    unsigned char *pixels = malloc(size);
    GLint buffer;
    int x, y;
    assert(pixels && width == 640 && height == 480);
    glGetIntegerv(GL_READ_BUFFER, &buffer);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glReadBuffer((GLenum)buffer);
    for (y = 0; y < height; ++y) {
        unsigned sy = view->origin_y + ((unsigned)y * view->rows) / (unsigned)height;
        for (x = 0; x < width; ++x) {
            unsigned sx = view->origin_x + (unsigned)x / 2u;
            unsigned color = chart_color(sx, sy);
            const unsigned char *actual = pixels + 4u * ((size_t)(height - 1 - y) * width + x);
            unsigned c;
            for (c = 0; c < 3; ++c) {
                unsigned five = (color >> (5u * c)) & 31u;
                if (actual[c] != ((five << 3) | (five >> 2))) {
                    ++view->differences;
                    break;
                }
            }
        }
    }
    free(pixels);
}

static void display_field_probe(MusashiGpuPsyCross *gpu, MusashiGpuBackend *backend) {
    static uint16_t chart[512][1024];
    static const struct { unsigned mode, dx, dy, cx, cy, rows; } frames[] = {
        {0x01, 38, 7, 600,   0, 240}, /* offscreen in X */
        {0x01, 38, 7,  64, 256, 240}, /* offscreen in Y */
        {0x25, 41, 9, 600,  16, 480}, /* all 480 lines, both fields visible */
        {0x25, 41, 9,  64,   0, 480}  /* overlapping draw area control */
    };
    MusashiGpuController controller = {0};
    unsigned i, x, y, field, mismatches = 0;
    assert(musashi_gpu_controller_init(&controller, backend));
    for (y = 0; y < 512; ++y)
        for (x = 0; x < 1024; ++x)
            chart[y][x] = chart_color(x, y);
    GR_CopyVRAM(&chart[0][0], 0, 0, 1024, 512, 0, 0);
    assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP1, 0x03000000u));
    for (i = 0; i < sizeof(frames) / sizeof(frames[0]); ++i) {
        ChartView view = {frames[i].dx, frames[i].dy, frames[i].rows, 0};
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP1,
                                              0x08000000u | frames[i].mode));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP1,
                                              0x05000000u | (view.origin_y << 10) | view.origin_x));
        assert(controller.display.width == 320 && controller.display.height == view.rows);
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0,
                                              0xe3000000u | (frames[i].cy << 10) | frames[i].cx));
        assert(musashi_gpu_controller_write32(&controller, MUSASHI_GPU_GP0,
                                              0xe4000000u | ((frames[i].cy + 239u) << 10) |
                                              (frames[i].cx + 319u)));
        musashi_gpu_psycross_set_overlay(read_chart, &view);
        for (field = 0; field < 2; ++field) {
            uint32_t status;
            assert(musashi_gpu_controller_read32(&controller, MUSASHI_GPU_GP1, &status));
            assert((status >> 31) == field);
            view.differences = 0;
            assert(musashi_gpu_psycross_present(gpu));
            fprintf(stderr, "chart mode=%02x display=%u,%u clip=%u,%u parity=%u wrong=%u/307200\n",
                    frames[i].mode, view.origin_x, view.origin_y, frames[i].cx, frames[i].cy,
                    field, view.differences);
            mismatches += view.differences;
            assert(musashi_gpu_controller_vblank(&controller));
        }
    }
    musashi_gpu_psycross_set_overlay(NULL, NULL);
    assert(!mismatches);
}

int main(void) {
    MusashiGpuPsyCross gpu = {0}, competing = {0};
    MusashiGpuBackend backend, unused;
    DRAWENV expected;
    RECT16 rect = {123, 177, 2, 1};
    uint32_t sentinel = 0x12346b5au, readback = 0;
    unsigned char pixel[4];
    GLboolean restored_mask[4];
    GLuint borrowed_framebuffer;
    GLint restored_framebuffer;
    GLfloat restored_color[4];
    Foreign args = {&gpu, &backend};
    SDL_Thread *thread;
    assert(!musashi_gpu_psycross_init(&gpu, &backend));
    PsyX_Initialise("Musashi GPU backend probe", 640, 480, 0);
    if (!SDL_GL_GetCurrentContext() || !SDL_GL_GetCurrentWindow()) {
        musashi_psyx_shutdown();
        return 77;
    }
    assert(musashi_gpu_psycross_init(&gpu, &backend));
    assert(!musashi_gpu_psycross_init(&competing, &unused));
    assert(backend.ready(backend.userdata) == 1);
    assert(!gpu.display_enabled && !g_GPUDisabledState);
    fill_probe(&gpu, &backend);
    LoadImage(&rect, (u_long *)&sentinel);
    activeDrawEnv.r0 = 91;
    activeDrawEnv.dr_env.code[0] = 0x12345678u;
    expected = activeDrawEnv;
    expected.clip = (RECT16){0, 0, 1, 1};
    expected.tw = (RECT16){0};
    expected.ofs[0] = expected.ofs[1] = 0;
    expected.tpage = 0;
    expected.dtd = expected.dfe = 0;
    assert(backend.draw_mode(backend.userdata, 0xe1000755u));
    assert(activeDrawEnv.tpage == 0x155 && activeDrawEnv.dtd == 1 && activeDrawEnv.dfe == 1);
    /* Real parser queues geometry. Reset cancels it without ending its scene. */
    {
        TILE tile = {0};
        setTile(&tile);
        setXY0(&tile, 1, 1);
        setWH(&tile, 2, 2);
        setRGB0(&tile, 255, 0, 0);
        DrawPrim(&tile);
    }
    assert(g_vertexIndex > 0 && begin_scene_flag);
    assert(backend.ready(backend.userdata) == 0);
    {
        uint32_t before_window = gpu.texture_window;
        RECT16 before_clip = activeDrawEnv.clip;
        assert(!backend.environment(backend.userdata, 0xe2000001u));
        assert(gpu.texture_window == before_window);
        assert(!memcmp(&activeDrawEnv.clip, &before_clip, sizeof(before_clip)));
    }
    assert(backend.reset(backend.userdata));
    assert(g_vertexIndex == 0 && g_splitIndex == 0 && begin_scene_flag);
    assert(!memcmp(&activeDrawEnv, &expected, sizeof(expected)));
    {
        MusashiGpuDisplayState descriptor = {
            0, 0x00c60260u, 0x00040010u, 1,
            0, 0, 320, 240, 608, 3168, 16, 256, 8
        };
        assert(backend.display(backend.userdata, &descriptor));
        assert(gpu.display.horizontal == 0x00c60260u);
        assert(gpu.display.mode == 1 && gpu.display.width == 320);
        assert(activeDispEnv.disp.x == 0 && activeDispEnv.disp.y == 0);
        assert(activeDispEnv.disp.w == 320 && activeDispEnv.disp.h == 240);
        assert(activeDispEnv.isinter == 0 && activeDispEnv.isrgb24 == 0);

        {
            MusashiGpuDisplayState bad = descriptor;
            DISPENV before_active = activeDispEnv;
            DISPENV before_current = currentDispEnv;
            MusashiGpuDisplayState before_gpu = gpu.display;
            bad.x = 1;
            assert(!backend.display(backend.userdata, &bad));
            assert(!memcmp(&gpu.display, &before_gpu, sizeof(before_gpu)));
            assert(!memcmp(&activeDispEnv, &before_active, sizeof(before_active)));
            assert(!memcmp(&currentDispEnv, &before_current, sizeof(before_current)));
            bad = descriptor;
            bad.origin |= 0x00080000u;
            assert(!backend.display(backend.userdata, &bad));
            bad = descriptor;
            bad.h_end = 3167;
            assert(!backend.display(backend.userdata, &bad));
            bad = descriptor;
            bad.horizontal |= 0x01000000u;
            assert(!backend.display(backend.userdata, &bad));
        }

        {
            TILE tile = {0};
            setTile(&tile);
            setXY0(&tile, 1, 1);
            setWH(&tile, 2, 2);
            setRGB0(&tile, 255, 0, 0);
            DrawPrim(&tile);
            assert(g_vertexIndex > 0 && g_splitIndex > 0);
            assert(!backend.display(backend.userdata, &descriptor));
            assert(backend.reset(backend.userdata));
            assert(backend.display(backend.userdata, &descriptor));
        }
    }
    /* The bounded environment owner accepts register state while geometry
     * and VRAM operations remain outside this adapter's contract. */
    assert(backend.environment(backend.userdata, 0xe2001285u));
    assert(gpu.texture_window == 0x1285u);
    assert(activeDrawEnv.tw.w == (0x1285 & 0x1f));
    assert(activeDrawEnv.tw.h == ((0x1285 >> 5) & 0x1f));
    assert(activeDrawEnv.tw.x == ((0x1285 >> 10) & 0x1f));
    assert(activeDrawEnv.tw.y == ((0x1285 >> 15) & 0x1f));
    assert(backend.environment(backend.userdata,
                               0xe3080c07u)); /* x=7, y=3, raw y bit19 */
    assert(backend.environment(backend.userdata, 0xe400180fu));
    assert(gpu.clip_x1 == 7 && gpu.clip_y1 == 3 &&
           gpu.clip_x2 == 15 && gpu.clip_y2 == 6);
    assert(activeDrawEnv.clip.x == 7 && activeDrawEnv.clip.y == 3 &&
           activeDrawEnv.clip.w == 9 && activeDrawEnv.clip.h == 4);
    assert(backend.environment(backend.userdata, 0xe3001810u));
    assert(activeDrawEnv.clip.w == 0 && activeDrawEnv.clip.h == 1);
    assert(backend.environment(backend.userdata, 0xe400180fu));
    assert(backend.environment(backend.userdata,
                               0xe50017ffu)); /* x=-1, y=2 */
    assert(gpu.drawing_offset == 0x17ffu);
    assert(activeDrawEnv.ofs[0] == -1 && activeDrawEnv.ofs[1] == 2);
    assert(backend.environment(backend.userdata, 0xe6000003u));
    assert(gpu.mask_flags == 3);
    /* Every encoded E5 component is accepted independently. The adapter
     * retains raw state while exposing the selected signed values to DRAWENV. */
    {
        unsigned int q;
        for (q = 0; q < 2048; ++q) {
            assert(backend.environment(backend.userdata,
                                       0xe5000000u | q));
            assert(gpu.drawing_offset == q);
            assert(activeDrawEnv.ofs[0] == (q < 1024 ? (short)q : (short)q - 2048));
        }
        for (q = 0; q < 2048; ++q) {
            assert(backend.environment(backend.userdata,
                                       0xe5000000u | (q << 11)));
            assert(gpu.drawing_offset == (q << 11));
            assert(activeDrawEnv.ofs[1] == (q < 1024 ? (short)q : (short)q - 2048));
        }
    }
    /* Reverse order, full 1024x512, zero and inverted inclusive bounds. */
    assert(backend.environment(backend.userdata, 0xe407ffffu));
    assert(backend.environment(backend.userdata, 0xe3000000u));
    assert(activeDrawEnv.clip.x == 0 && activeDrawEnv.clip.y == 0 &&
           activeDrawEnv.clip.w == 1024 && activeDrawEnv.clip.h == 512);
    assert(backend.environment(backend.userdata, 0xe4019064u));
    assert(backend.environment(backend.userdata, 0xe3019065u));
    assert(activeDrawEnv.clip.w == 0 && activeDrawEnv.clip.h == 1);
    assert(backend.environment(backend.userdata, 0xe3019064u));
    assert(backend.environment(backend.userdata, 0xe4018c64u));
    assert(activeDrawEnv.clip.w == 1 && activeDrawEnv.clip.h == 0);
    assert(backend.environment(backend.userdata, 0xe3019064u));
    assert(backend.environment(backend.userdata, 0xe4019064u));
    assert(activeDrawEnv.clip.w == 1 && activeDrawEnv.clip.h == 1);
    assert(backend.environment(backend.userdata,
                               0xe3081c02u)); /* raw Y bit19, decoded Y=7 */
    assert(gpu.drawing_area_start == 0x81c02u && activeDrawEnv.clip.y == 7);
    StoreImage(&rect, (u_long *)&readback);
    assert(readback == sentinel && gpu.presentations == 0);
    assert(!musashi_gpu_psycross_present(&gpu)); /* No implicit scene takeover. */
    begin_scene_flag = 0; /* Test-owned scene teardown without EndScene/VRAM copy. */
    assert(backend.display_enable(backend.userdata, 0));
    assert(backend.draw_mode(backend.userdata, 0xe1003000u));
    assert(!g_GPUDisabledState && !gpu.display_enabled);
    gpu.command_active = 1; /* Explicit reentrant command fixture. */
    assert(backend.ready(backend.userdata) == 0);
    assert(!backend.reset(backend.userdata));
    gpu.command_active = 0;
    thread = SDL_CreateThread(foreign, "gpu_foreign_probe", &args);
    assert(thread);
    SDL_WaitThread(thread, NULL);
    assert(!gpu.faulted);
    assert(SDL_GL_MakeCurrent((SDL_Window *)gpu.window, NULL) == 0);
    assert(backend.ready(backend.userdata) < 0);
    assert(!backend.reset(backend.userdata));
    {
        MusashiGpuFill fill={.command=0x020000ffu,.wh=0x00010010u};
        uint16_t out=0x1234;
        assert(musashi_gpu_fill_derive(&fill));
        assert(!backend.fill_vram(backend.userdata,&fill));
        assert(!musashi_gpu_psycross_read_vram(&gpu,0,0,1,1,&out,1));
        assert(out==0x1234);
    }
    assert(SDL_GL_MakeCurrent((SDL_Window *)gpu.window, gpu.context) == 0);
    assert(!backend.draw_mode(backend.userdata, 0xe2000000u));
    assert(backend.display_enable(backend.userdata, 1));
    assert(gpu.display_enabled);
    assert(backend.display_enable(backend.userdata, 0));
    assert(!gpu.display_enabled);
    assert(backend.clear_fifo(backend.userdata));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0);
    glGenFramebuffers(1, &borrowed_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, borrowed_framebuffer);
    glColorMaski(1, GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 0, 0);
    glColorMaski(0, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    glClearColor(0.25f, 0.5f, 0.75f, 0.5f);
    assert(musashi_gpu_psycross_present(&gpu));
    assert(glIsEnabled(GL_SCISSOR_TEST));
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restored_framebuffer);
    assert((GLuint)restored_framebuffer == borrowed_framebuffer);
    glGetBooleani_v(GL_COLOR_WRITEMASK, 1, restored_mask);
    assert(restored_mask[0] && !restored_mask[1] && restored_mask[2] && !restored_mask[3]);
    glGetBooleanv(GL_COLOR_WRITEMASK, restored_mask);
    assert(!restored_mask[0] && restored_mask[1] && !restored_mask[2] && restored_mask[3]);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, restored_color);
    assert(restored_color[0] == 0.25f && restored_color[1] == 0.5f &&
           restored_color[2] == 0.75f && restored_color[3] == 0.5f);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0);
    StoreImage(&rect, (u_long *)&readback);
    assert(readback == sentinel && gpu.presentations == 1);
    glDeleteFramebuffers(1, &borrowed_framebuffer);
    display_field_probe(&gpu, &backend);
    display_page_probe(&gpu, &backend);
    assert(musashi_gpu_psycross_shutdown(&gpu));
    assert(backend.ready(backend.userdata) < 0);
    musashi_psyx_shutdown();
    puts("GPU BACKEND OBSERVED: actual drawenv/queue reset, VRAM preserved, blank output, GP1 scanout across two drawing pages");
    return 0;
}
