/* GL 3.3 core executor. See bfm_gl_exec.h. */

#include "bfm_gl_exec.h"
#include "../../bfm_plat_widescreen.h"

#include <GL/glcorearb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BFM_GL_FUNCS(X)                                                    \
    X(PFNGLGETSTRINGPROC, GetString)                                       \
    X(PFNGLGETERRORPROC, GetError)                                         \
    X(PFNGLGETINTEGERVPROC, GetIntegerv)                                   \
    X(PFNGLVIEWPORTPROC, Viewport)                                         \
    X(PFNGLSCISSORPROC, Scissor)                                           \
    X(PFNGLENABLEPROC, Enable)                                             \
    X(PFNGLDISABLEPROC, Disable)                                           \
    X(PFNGLCLEARCOLORPROC, ClearColor)                                     \
    X(PFNGLCLEARPROC, Clear)                                               \
    X(PFNGLBLENDEQUATIONPROC, BlendEquation)                               \
    X(PFNGLBLENDFUNCPROC, BlendFunc)                                       \
    X(PFNGLBLENDCOLORPROC, BlendColor)                                     \
    X(PFNGLGENTEXTURESPROC, GenTextures)                                   \
    X(PFNGLDELETETEXTURESPROC, DeleteTextures)                             \
    X(PFNGLBINDTEXTUREPROC, BindTexture)                                   \
    X(PFNGLTEXIMAGE2DPROC, TexImage2D)                                     \
    X(PFNGLTEXSUBIMAGE2DPROC, TexSubImage2D)                               \
    X(PFNGLTEXPARAMETERIPROC, TexParameteri)                               \
    X(PFNGLACTIVETEXTUREPROC, ActiveTexture)                               \
    X(PFNGLPIXELSTOREIPROC, PixelStorei)                                   \
    X(PFNGLREADPIXELSPROC, ReadPixels)                                     \
    X(PFNGLGENFRAMEBUFFERSPROC, GenFramebuffers)                           \
    X(PFNGLDELETEFRAMEBUFFERSPROC, DeleteFramebuffers)                     \
    X(PFNGLBINDFRAMEBUFFERPROC, BindFramebuffer)                           \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC, FramebufferTexture2D)                 \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, CheckFramebufferStatus)             \
    X(PFNGLGENVERTEXARRAYSPROC, GenVertexArrays)                           \
    X(PFNGLDELETEVERTEXARRAYSPROC, DeleteVertexArrays)                     \
    X(PFNGLBINDVERTEXARRAYPROC, BindVertexArray)                           \
    X(PFNGLGENBUFFERSPROC, GenBuffers)                                     \
    X(PFNGLDELETEBUFFERSPROC, DeleteBuffers)                               \
    X(PFNGLBINDBUFFERPROC, BindBuffer)                                     \
    X(PFNGLBUFFERDATAPROC, BufferData)                                     \
    X(PFNGLVERTEXATTRIBPOINTERPROC, VertexAttribPointer)                   \
    X(PFNGLVERTEXATTRIBIPOINTERPROC, VertexAttribIPointer)                 \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, EnableVertexAttribArray)           \
    X(PFNGLCREATESHADERPROC, CreateShader)                                 \
    X(PFNGLSHADERSOURCEPROC, ShaderSource)                                 \
    X(PFNGLCOMPILESHADERPROC, CompileShader)                               \
    X(PFNGLGETSHADERIVPROC, GetShaderiv)                                   \
    X(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)                         \
    X(PFNGLDELETESHADERPROC, DeleteShader)                                 \
    X(PFNGLCREATEPROGRAMPROC, CreateProgram)                               \
    X(PFNGLATTACHSHADERPROC, AttachShader)                                 \
    X(PFNGLBINDATTRIBLOCATIONPROC, BindAttribLocation)                     \
    X(PFNGLLINKPROGRAMPROC, LinkProgram)                                   \
    X(PFNGLGETPROGRAMIVPROC, GetProgramiv)                                 \
    X(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog)                       \
    X(PFNGLDELETEPROGRAMPROC, DeleteProgram)                               \
    X(PFNGLUSEPROGRAMPROC, UseProgram)                                     \
    X(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation)                     \
    X(PFNGLUNIFORM1IPROC, Uniform1i)                                       \
    X(PFNGLUNIFORM1FPROC, Uniform1f)                                       \
    X(PFNGLUNIFORM2FPROC, Uniform2f)                                       \
    X(PFNGLUNIFORM4FPROC, Uniform4f)                                       \
    X(PFNGLUNIFORM4IPROC, Uniform4i)                                       \
    X(PFNGLDRAWARRAYSPROC, DrawArrays)                                     \
    X(PFNGLBLENDFUNCSEPARATEPROC, BlendFuncSeparate)                       \
    X(PFNGLBLENDEQUATIONSEPARATEPROC, BlendEquationSeparate)               \
    X(PFNGLSTENCILFUNCPROC, StencilFunc)                                   \
    X(PFNGLSTENCILOPPROC, StencilOp)                                       \
    X(PFNGLCLEARSTENCILPROC, ClearStencil)                                 \
    X(PFNGLCOLORMASKPROC, ColorMask)                                       \
    X(PFNGLGENRENDERBUFFERSPROC, GenRenderbuffers)                         \
    X(PFNGLDELETERENDERBUFFERSPROC, DeleteRenderbuffers)                   \
    X(PFNGLBINDRENDERBUFFERPROC, BindRenderbuffer)                         \
    X(PFNGLRENDERBUFFERSTORAGEPROC, RenderbufferStorage)                   \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, FramebufferRenderbuffer)           \
    X(PFNGLBLITFRAMEBUFFERPROC, BlitFramebuffer)

/* Entry points only MSAA needs. bfm_gl_exec_set_msaa loads them: a context
 * (or a driver) without one leaves MSAA off; they never fail
 * bfm_gl_exec_create. */
#define BFM_GL_MS_FUNCS(X)                                                 \
    X(PFNGLTEXIMAGE2DMULTISAMPLEPROC, TexImage2DMultisample)               \
    X(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, RenderbufferStorageMultisample) \
    X(PFNGLSAMPLEMASKIPROC, SampleMaski)                                   \
    X(PFNGLDRAWBUFFERPROC, DrawBuffer)                                     \
    X(PFNGLREADBUFFERPROC, ReadBuffer)

typedef struct Target {
    GLuint tex, fbo;             /* the single-sample image every reader sees: the resolved image when multisampled */
    int w, h;                    /* 1x size */
    GLuint stencil;              /* depth24/stencil8, once mask-check is used (multisampled with ms_tex) */
    GLuint ms_tex, ms_fbo;       /* MSAA: RGBA8 multisample texture, and the FBO every draw renders into (0 = off) */
    GLuint st_fbo;               /* MSAA: stencil-only FBO, for the per-sample stencil build */
    int ms_samples;              /* the samples ms_tex really holds */
} Target;

/* Uniform locations of one draw program (the single-sample one, or the MSAA variant). */
typedef struct DrawLoc {
    GLint size, flip, half, pass, pass_bit, dither, scale, win, vram, repl;
} DrawLoc;

typedef struct Snapshot {
    GLuint tex;
    int w, h;                    /* device pixels */
} Snapshot;

struct BfmGlExec {
#define X(type, name) type name;
    BFM_GL_FUNCS(X)
#undef X
#define X(type, name) type ms_##name;
    BFM_GL_MS_FUNCS(X)
#undef X
    BfmGlGetProc getproc;
    GLuint prog, blit, vao, vbo, empty_vao, vram_tex, white_tex, image_tex;
    Target tgt[3];               /* VRAM, scene 0, scene 1 */
    Snapshot snap[BFM_GL_MAX_SNAPSHOTS];
    GLuint snap_fbo;
    int scale;
    int integer;                 /* present at whole multiples of the display height */
    /* the locations of the draw program in use (prog): loc_use copies a DrawLoc here */
    GLint u_size, u_flip, u_half, u_pass, u_pass_bit, u_dither, u_scale, u_win, u_vram, u_repl;
    GLint b_src, b_tex, b_alpha, b_five;
    /* redundant-state filter for run_cmds (-2 = unknown) */
    int cur_blend, cur_pass, cur_dither;
    BfmPlatRect cur_win;
    unsigned err;
    /* MSAA (bfm_gl_exec_set_msaa); everything below is zero while it is off */
    GLuint base_prog, ms_prog, ms_stencil_prog;   /* prog is one of the first two */
    DrawLoc base_loc, ms_loc;
    GLint s_ms, s_sample;        /* ms_stencil_prog's uniforms */
    int msaa;                    /* the samples the targets really hold (0 = off) */
    int msaa_n0;                 /* the request after the GPU limits, before the memory fit */
    int msaa_alloc;              /* the samples asked of GL for each target */
    char msaa_note[160];
    BfmGlMsaaStats stats;
};

static const char *vs_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec4 a_color;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "layout(location = 3) in uvec4 a_info;\n"
    "layout(location = 4) in vec2 a_st;\n"
    "layout(location = 5) in ivec4 a_t01;\n"   /* x0 y0 x1 y1 */
    "layout(location = 6) in ivec2 a_t2;\n"    /* x2 y2 */
    "layout(location = 7) in uvec3 a_tuv;\n"   /* u | v << 16 per vertex */
    "layout(location = 8) in uvec3 a_trgb;\n"  /* r | g << 8 | b << 16 per vertex */
    "uniform vec2 u_size;\n"
    "uniform int u_flip;\n"
    "uniform float u_half;\n"
    "out vec4 v_color;\n"
    "noperspective out vec2 v_uv;\n"
    "flat out uvec4 v_info;\n"
    "noperspective out vec2 v_st;\n"
    "flat out ivec4 v_t01;\n"
    "flat out ivec2 v_t2;\n"
    "flat out uvec3 v_tuv;\n"
    "flat out uvec3 v_trgb;\n"
    "void main() {\n"
    /* The PS1 (and the cpu rasteriser) samples a pixel at its integer
     * position; GL samples a fragment at its centre. Moving the geometry by
     * half a target pixel (u_half = 0.5 / scale target-local pixels; 0 for
     * the window overlay) puts the first sub-pixel's centre on the integer
     * position, so coverage follows the PS1 top-left rule and texture
     * coordinates and colours are those of the integer sample point. */
    "    vec2 n = (a_pos + u_half) / u_size * 2.0 - 1.0;\n"
    "    if (u_flip != 0) n.y = -n.y;\n"
    "    gl_Position = vec4(n, 0.0, 1.0);\n"
    "    v_color = a_color; v_uv = a_uv; v_info = a_info; v_st = a_st;\n"
    "    v_t01 = a_t01; v_t2 = a_t2; v_tuv = a_tuv; v_trgb = a_trgb;\n"
    "}\n";

/* Mirrors bfm_gl_sample / bfm_gl_modulate / bfm_gl_dither5. VRAM texels and
 * vertex colours take the PS1's integer route: the interpolated 8-bit colour
 * is truncated, modulation is ((texel5 << 3) * colour8) >> 7, dithering adds
 * to that 8-bit value, and the stored channel is its top five bits (written
 * as c5 / 31 so an RGBA8 target reads back as c5). HD replacement and
 * snapshot texels keep their 8-bit precision. */
static const char *fs_src =
    "#version 330 core\n"
    "in vec4 v_color;\n"
    "noperspective in vec2 v_uv;\n"
    "flat in uvec4 v_info;\n"
    "noperspective in vec2 v_st;\n"
    "flat in ivec4 v_t01;\n"
    "flat in ivec2 v_t2;\n"
    "flat in uvec3 v_tuv;\n"
    "flat in uvec3 v_trgb;\n"
    "uniform usampler2D u_vram;\n"
    "uniform sampler2D u_repl;\n"
    "uniform int u_pass;\n"
    "uniform int u_pass_bit;\n"
    "uniform int u_dither;\n"
    "uniform int u_scale;\n"
    "uniform float u_half;\n"
    "uniform ivec4 u_win;\n"
    "layout(location = 0, index = 0) out vec4 o_color;\n"
    "layout(location = 0, index = 1) out vec4 o_factor;\n"
    "const int dm[16] = int[16](-4, 0, -3, 1, 2, -2, 3, -1, -3, 1, -4, 0, 3, -1, 2, -2);\n"
    /* interpolants that are exactly integral at the sample point (texel per
     * pixel, vertex colours) must not floor to the previous integer because
     * of interpolation rounding */
    "const float EPS = 1.0 / 512.0;\n"
    "uint vr(int x, int y) { return texelFetch(u_vram, ivec2(x & 1023, y & 511), 0).r; }\n"
    "int wnd(int u, int off, int size) {\n"
    "    if (size <= 0) return u;\n"
    "    return (u & (size - 1)) | (off & ~(size - 1) & 255);\n"
    "}\n";

/* main(): a second literal keeps each within C99's 4095-character minimum */
static const char *fs_main =
    "void main() {\n"
    "    uint flags = v_info.z;\n"
    "    vec3 rgb = v_color.rgb;\n"
    "    ivec3 c8 = ivec3(floor(v_color.rgb * 255.0 + EPS));\n"
    "    int iu = int(floor(v_uv.x + EPS)), iv = int(floor(v_uv.y + EPS));\n"
    "    bool stp = false, exact = (flags & 1u) == 0u;\n"
    /* On a 1x target (a target pixel is a PS1 pixel; not the window
     * overlay) the texture coordinates and the colour are the PS1
     * rasteriser's integers: the
     * barycentric weights of the fragment's integer position, times the
     * vertex values, divided by the signed area, truncated. Weights are
     * below 2^20 and values below 2^11, so the sums fit 32 bits. The
     * interpolated values above remain for the window overlay and for
     * internal scales > 1, where the sample points are fractional. */
    "    if (u_scale == 1 && u_half > 0.0) {\n"
    "        ivec2 q = ivec2(floor(gl_FragCoord.xy));\n"
    "        int x0 = v_t01.x, y0 = v_t01.y, x1 = v_t01.z, y1 = v_t01.w, x2 = v_t2.x, y2 = v_t2.y;\n"
    "        int area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);\n"
    "        int w0 = (x1 - q.x) * (y2 - q.y) - (y1 - q.y) * (x2 - q.x);\n"
    "        int w1 = (x2 - q.x) * (y0 - q.y) - (y2 - q.y) * (x0 - q.x);\n"
    "        int w2 = (x0 - q.x) * (y1 - q.y) - (y0 - q.y) * (x1 - q.x);\n"
    "        if (area != 0) {\n"
    "            ivec3 u3 = ivec3(v_tuv & 0xFFFFu), v3 = ivec3(v_tuv >> 16);\n"
    "            ivec3 r3 = ivec3(v_trgb & 0xFFu), g3 = ivec3((v_trgb >> 8) & 0xFFu);\n"
    "            ivec3 b3 = ivec3((v_trgb >> 16) & 0xFFu);\n"
    "            iu = (w0 * u3.x + w1 * u3.y + w2 * u3.z) / area;\n"
    "            iv = (w0 * v3.x + w1 * v3.y + w2 * v3.z) / area;\n"
    "            c8 = clamp(ivec3((w0 * r3.x + w1 * r3.y + w2 * r3.z) / area,\n"
    "                             (w0 * g3.x + w1 * g3.y + w2 * g3.z) / area,\n"
    "                             (w0 * b3.x + w1 * b3.y + w2 * b3.z) / area), 0, 255);\n"
    "        }\n"
    "    }\n";

static const char *fs_tex =
    "    if ((flags & 1u) != 0u) {\n"
    "        vec3 t = vec3(0.0);\n"
    /* replacement/snapshot texels sample where the unshifted geometry did */
    "        vec2 st = u_half > 0.0 ? v_st + 0.5 * (dFdx(v_st) + dFdy(v_st)) : v_st;\n"
    "        if ((flags & 16u) != 0u) {\n"
    "            vec4 r = texture(u_repl, st);\n"
    "            stp = r.a > 0.5;\n"
    "            if (!stp && all(lessThan(r.rgb, vec3(0.5 / 255.0)))) discard;\n"
    "            t = r.rgb;\n"
    "        } else if (v_info.w != 0u) {\n"
    "            vec4 r = texture(u_repl, st);\n"
    "            if (r.a < 0.5) discard;\n"
    "            t = r.rgb;\n"
    "        } else {\n"
    "            uint p;\n"
    "            int u = iu, v = iv;\n"
    "            if ((flags & 8u) != 0u) {\n"
    "                p = vr(u, v);\n"
    "            } else {\n"
    "                uint tp = v_info.y, cl = v_info.x, d = (tp >> 7) & 3u;\n"
    "                int bx = int(tp & 15u) * 64, by = int((tp >> 4) & 1u) * 256;\n"
    "                int cx = int(cl & 63u) * 16, cy = int((cl >> 6) & 511u);\n"
    "                u = wnd(u & 255, u_win.x, u_win.z);\n"
    "                v = wnd(v & 255, u_win.y, u_win.w);\n"
    "                if (d == 0u) {\n"
    "                    uint w = vr(bx + u / 4, by + v);\n"
    "                    p = vr(cx + int((w >> uint((u & 3) * 4)) & 15u), cy);\n"
    "                } else if (d == 1u) {\n"
    "                    uint w = vr(bx + u / 2, by + v);\n"
    "                    p = vr(cx + int((w >> uint((u & 1) * 8)) & 255u), cy);\n"
    "                } else {\n"
    "                    p = vr(bx + u, by + v);\n"
    "                }\n"
    "                if (p == 0u) discard;\n"
    "            }\n"
    "            stp = (p & 0x8000u) != 0u;\n"
    "            {\n"
    "                ivec3 t5 = ivec3(int(p & 31u), int((p >> 5) & 31u), int((p >> 10) & 31u));\n"
    "                c8 = (flags & 2u) != 0u ? t5 << 3 : min((t5 * c8) >> 4, 255);\n"
    "            }\n"
    "            exact = true;\n"
    "        }\n"
    "        if (!exact) rgb = (flags & 2u) != 0u ? t : min(t * v_color.rgb * (255.0 / 128.0), vec3(1.0));\n"
    "    }\n"
    "    ivec3 c5 = ivec3(0);\n"
    "    bool five = exact;\n"
    "    if (exact) {\n"
    "        if (u_dither != 0 && (flags & 4u) != 0u) {\n"
    "            ivec2 q = ivec2(gl_FragCoord.xy) / u_scale;\n"
    "            c8 = clamp(c8 + dm[(q.y & 3) * 4 + (q.x & 3)], 0, 255);\n"
    "        }\n"
    "        c5 = c8 >> 3;\n"
    "        rgb = vec3(c5) / 31.0;\n"
    "    } else if (u_dither != 0 && (flags & 4u) != 0u) {\n"
    "        ivec2 q = ivec2(gl_FragCoord.xy) / u_scale;\n"
    "        float d = float(dm[(q.y & 3) * 4 + (q.x & 3)]);\n"
    "        vec3 d8 = clamp(floor(rgb * 255.0 + 0.5) + d, 0.0, 255.0);\n"
    "        c5 = ivec3(d8) >> 3;\n"
    "        rgb = vec3(c5) / 31.0;\n"
    "        five = true;\n"
    "    }\n";

/* the pass split, the blend and the written colour: a fourth literal (each
 * stays within C99's 4095-character minimum) */
static const char *fs_out =
    "    /* two-pass draws: pass 1 keeps fragments whose bit is 0, pass 2 those\n"
    "     * whose bit is 1; the bit is STP (mode-2 split) or the written bit 15\n"
    "     * (stencil upkeep). Untextured fragments have STP 0. */\n"
    "    {\n"
    "        bool bit = u_pass_bit != 0 ? (stp || (flags & 32u) != 0u) : stp;\n"
    "        if (u_pass == 1 && bit) discard;\n"
    "        if (u_pass == 2 && !bit) discard;\n"
    "    }\n"
    "    /* semi-transparency per fragment (dual-source): untextured pixels,\n"
    "     * STP texels and replacement texels blend; the rest are opaque.\n"
    "     * Mode 2 draws use a reverse-subtract equation and ignore o_factor.\n"
    "     * A 5-bit front f blends exactly as the PS1 does (bfm_gl_blend_state\n"
    "     * / bfm_gl_blend_byte): a target byte 8b + r (r < 8) holds 5-bit b;\n"
    "     * the source is a whole number of steps, src_mul * (f >> src_shift)\n"
    "     * of 255, mode 3 floors F / 4 itself, and mode 0 scales the\n"
    "     * destination by 127/255 so 4(b + f) + r/2 - (8b + r)/510 rounds\n"
    "     * into step (b + f) >> 1 for every r. 8-bit fronts (replacement\n"
    "     * texels) keep the plain factors. */\n"
    "    vec3 src = rgb;\n"
    "    float df = 0.0;\n"
    "    if ((flags & 64u) != 0u && ((flags & 1u) == 0u || stp || v_info.w != 0u)) {\n"
    "        uint mode = (flags >> 8) & 3u;\n"
    "        df = 1.0;\n"
    "        if (five) {\n"
    "            if (mode == 0u) { src = vec3(c5 * 4) / 255.0; df = 127.0 / 255.0; }\n"
    "            else if (mode == 3u) src = vec3((c5 >> 2) * 8) / 255.0;\n"
    "            else src = vec3(c5 * 8) / 255.0;\n"
    "        } else if (mode == 0u) { src = rgb * 0.5; df = 0.5; }\n"
    "        else if (mode == 3u) src = rgb * 0.25;\n"
    "    }\n"
    "    /* alpha carries the PS1 mask bit (bit 15) of the written pixel */\n"
    "    o_color = vec4(src, (stp || (flags & 32u) != 0u) ? 1.0 : 0.0);\n"
    "    o_factor = vec4(df, df, df, 0.0);\n"
    "}\n";

static const char *blit_vs =
    "#version 330 core\n"
    "uniform vec4 u_src;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "    vec2 c = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));\n"
    "    gl_Position = vec4(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);\n"
    "    v_uv = vec2(mix(u_src.x, u_src.z, c.x), mix(u_src.y, u_src.w, c.y));\n"
    "}\n";

/* u_five: show each byte as the 8-bit value of its 5-bit pixel (byte >> 3,
 * as c5 / 31: what the Original path displays), so a blend's low bits never
 * reach the window; off for movie images and live HD replacements. */
static const char *blit_fs =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform int u_alpha_test;\n"
    "uniform int u_five;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "    vec4 c = texture(u_tex, v_uv);\n"
    "    if (u_alpha_test != 0 && c.a < 0.5) discard;\n"
    "    if (u_five != 0) c.rgb = floor(floor(c.rgb * 255.0 + 0.5) / 8.0) / 31.0;\n"
    "    o_color = vec4(c.rgb, 1.0);\n"
    "}\n";

/* ---- MSAA shaders ---------------------------------------------------------
 *
 * The multisampled draw program is not a second copy of the shaders above:
 * bfm_gl_exec_set_msaa derives it from them at run time by the replacements
 * in ms_patches, each of which must match its string exactly once or the
 * variant is refused and MSAA stays off. The single-sample strings (and so
 * the program every msaa = 0 run compiles) are never edited. The changes:
 *  - the vertex shader drops the u_half offset. The single-sample path moves
 *    geometry by half a device pixel so that a pixel centre falls on the PS1's
 *    integer sample point; with samples spread over the pixel that offset
 *    would put every pixel-aligned rect edge through the middle of a pixel
 *    and blur each sprite, glyph and tile border. Without it a rect covers
 *    whole pixels, all of whose samples agree, and only polygon edges blend.
 *  - texture coordinates, colours and replacement coordinates are `centroid`
 *    varyings: a fringe pixel (centre outside the primitive, some samples
 *    inside) reads values from inside it, not an extrapolation that would
 *    fetch the neighbouring atlas texel.
 *  - the integer barycentric formulas (1x) run only for a pixel whose integer
 *    position lies inside its triangle: those are exactly the pixels the PS1
 *    draws, so they keep its values; a fringe pixel keeps the centroid
 *    values.
 *  - the half-pixel derivative correction of the replacement coordinates goes
 *    (it undid the offset removed above).
 * u_half stays set: the fragment shader reads it as "a target, not the window
 * overlay". */
typedef struct MsPatch {
    int part;                    /* 0 = vs_src, 1..4 = fs_src, fs_main, fs_tex, fs_out */
    const char *from, *to;
} MsPatch;

static const MsPatch ms_patches[] = {
    {0, "    vec2 n = (a_pos + u_half) / u_size * 2.0 - 1.0;\n",
        "    vec2 n = a_pos / u_size * 2.0 - 1.0;\n"},
    {0, "\nout vec4 v_color;\n", "\ncentroid out vec4 v_color;\n"},
    {0, "\nnoperspective out vec2 v_uv;\n", "\nnoperspective centroid out vec2 v_uv;\n"},
    {0, "\nnoperspective out vec2 v_st;\n", "\nnoperspective centroid out vec2 v_st;\n"},
    {1, "\nin vec4 v_color;\n", "\ncentroid in vec4 v_color;\n"},
    {1, "\nnoperspective in vec2 v_uv;\n", "\nnoperspective centroid in vec2 v_uv;\n"},
    {1, "\nnoperspective in vec2 v_st;\n", "\nnoperspective centroid in vec2 v_st;\n"},
    {2, "        if (area != 0) {\n",
        "        if (area != 0 && (area > 0 ? min(w0, min(w1, w2)) >= 0 : max(w0, max(w1, w2)) <= 0)) {\n"},
    {3, "        vec2 st = u_half > 0.0 ? v_st + 0.5 * (dFdx(v_st) + dFdy(v_st)) : v_st;\n",
        "        vec2 st = v_st;\n"},
};

/* One mask-check stencil sample: the sample's alpha (the PS1 mask bit) decides. */
static const char *ms_stencil_fs =
    "#version 330 core\n"
    "uniform sampler2DMS u_ms;\n"
    "uniform int u_sample;\n"
    "void main() {\n"
    "    if (texelFetch(u_ms, ivec2(gl_FragCoord.xy), u_sample).a < 0.5) discard;\n"
    "}\n";

/* src with the one occurrence of `from` replaced by `to`; NULL (and nothing
 * allocated) when `from` is absent or occurs more than once. */
static char *patch_once(const char *src, const char *from, const char *to) {
    const char *at = strstr(src, from);
    size_t sl = strlen(src), fl = strlen(from), tl = strlen(to), pre;
    char *out;
    if (!at || strstr(at + fl, from)) return NULL;
    pre = (size_t)(at - src);
    if (!(out = (char *)malloc(sl - fl + tl + 1))) return NULL;
    memcpy(out, src, pre);
    memcpy(out + pre, to, tl);
    memcpy(out + pre + tl, at + fl, sl - pre - fl + 1);
    return out;
}

/* The five sources of the multisampled draw program (vs, then the four
 * fragment parts): malloc'd, to be freed by the caller. Returns the number of
 * patches that did not match exactly once (0 = all applied). */
static int ms_sources(char *out[5]) {
    const char *base[5];
    size_t i;
    int bad = 0;
    base[0] = vs_src; base[1] = fs_src; base[2] = fs_main; base[3] = fs_tex; base[4] = fs_out;
    for (i = 0; i < 5; i++) {
        out[i] = (char *)malloc(strlen(base[i]) + 1);
        if (out[i]) memcpy(out[i], base[i], strlen(base[i]) + 1);
        else bad++;
    }
    for (i = 0; i < sizeof ms_patches / sizeof ms_patches[0]; i++) {
        const MsPatch *p = &ms_patches[i];
        char *r = out[p->part] ? patch_once(out[p->part], p->from, p->to) : NULL;
        if (!r) {
            bad++;
            continue;
        }
        free(out[p->part]);
        out[p->part] = r;
    }
    return bad;
}

static void note_err(BfmGlExec *x) {
    GLenum e;
    while ((e = x->GetError()) != GL_NO_ERROR)
        if (!x->err) x->err = e;
}

unsigned bfm_gl_exec_error(BfmGlExec *x) {
    unsigned e;
    note_err(x);
    e = x->err;
    x->err = 0;
    return e;
}

static GLuint compile(BfmGlExec *x, GLenum kind, const char *const *src, GLsizei parts,
                      char *err, size_t n) {
    GLuint s = x->CreateShader(kind);
    GLint ok = 0;
    x->ShaderSource(s, parts, src, NULL);
    x->CompileShader(s);
    x->GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        if (err && n) x->GetShaderInfoLog(s, (GLsizei)n, NULL, err);
        x->DeleteShader(s);
        return 0;
    }
    return s;
}

/* fs: the fragment shader as consecutive literals (each within C99's
 * 4095-character minimum). */
static GLuint link(BfmGlExec *x, const char *vs, const char *const *fs, GLsizei nfs, char *err,
                   size_t n) {
    GLuint v = compile(x, GL_VERTEX_SHADER, &vs, 1, err, n), f, p;
    GLint ok = 0;
    if (!v) return 0;
    if (!(f = compile(x, GL_FRAGMENT_SHADER, fs, nfs, err, n))) {
        x->DeleteShader(v);
        return 0;
    }
    p = x->CreateProgram();
    x->AttachShader(p, v);
    x->AttachShader(p, f);
    x->LinkProgram(p);
    x->DeleteShader(v);
    x->DeleteShader(f);
    x->GetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        if (err && n) x->GetProgramInfoLog(p, (GLsizei)n, NULL, err);
        x->DeleteProgram(p);
        return 0;
    }
    return p;
}

static void target_free(BfmGlExec *x, Target *t) {
    if (t->stencil) x->DeleteRenderbuffers(1, &t->stencil);
    if (t->st_fbo) x->DeleteFramebuffers(1, &t->st_fbo);
    if (t->ms_fbo) x->DeleteFramebuffers(1, &t->ms_fbo);
    if (t->ms_tex) x->DeleteTextures(1, &t->ms_tex);
    if (t->fbo) x->DeleteFramebuffers(1, &t->fbo);
    if (t->tex) x->DeleteTextures(1, &t->tex);
    memset(t, 0, sizeof *t);
}

/* The draw framebuffer of a target: the multisampled one when it has one. */
static GLuint draw_fbo(const Target *t) { return t->ms_fbo ? t->ms_fbo : t->fbo; }

/* MSAA: the multisample texture and FBO behind a target (x->msaa_alloc
 * samples asked for; t->ms_samples = what the texture really holds). Leaves
 * the texture and FBO bindings as target_alloc expects. */
static int ms_target_alloc(BfmGlExec *x, Target *t, int w, int h) {
    GLint have = 0;
    GLenum ge;
    note_err(x);                 /* keep what is pending; the check below sees only this allocation's error */
    x->GenTextures(1, &t->ms_tex);
    x->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->ms_tex);
    /* fixed sample locations: required to mix this texture and the stencil
     * renderbuffer in one framebuffer */
    x->ms_TexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, x->msaa_alloc, GL_RGBA8,
                                w * x->scale, h * x->scale, GL_TRUE);
    x->GenFramebuffers(1, &t->ms_fbo);
    x->BindFramebuffer(GL_FRAMEBUFFER, t->ms_fbo);
    x->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                            t->ms_tex, 0);
    x->GetIntegerv(GL_SAMPLES, &have);
    ge = x->GetError();
    if (ge != GL_NO_ERROR || x->CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ||
        have < 2) {
        x->BindFramebuffer(GL_FRAMEBUFFER, 0);
        x->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        return BFM_PLAT_ERROR;
    }
    t->ms_samples = have;
    x->Disable(GL_SCISSOR_TEST);
    x->ClearColor(0, 0, 0, 0);   /* alpha = mask bit 0 */
    x->Clear(GL_COLOR_BUFFER_BIT);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    x->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    return BFM_PLAT_OK;
}

static int target_alloc(BfmGlExec *x, Target *t, int w, int h) {
    target_free(x, t);
    x->GenTextures(1, &t->tex);
    x->BindTexture(GL_TEXTURE_2D, t->tex);
    x->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w * x->scale, h * x->scale, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    /* Native scanout must preserve texels and must not sample neighboring
     * VRAM pages when the display rectangle is enlarged. HD replacement
     * textures have their own filtering policy. */
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    x->GenFramebuffers(1, &t->fbo);
    x->BindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    x->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            t->tex, 0);
    if (x->CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        x->BindFramebuffer(GL_FRAMEBUFFER, 0);
        target_free(x, t);
        return BFM_PLAT_ERROR;
    }
    x->Disable(GL_SCISSOR_TEST);
    x->ClearColor(0, 0, 0, 0);   /* alpha = mask bit 0 */
    x->Clear(GL_COLOR_BUFFER_BIT);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    if (x->msaa_alloc && ms_target_alloc(x, t, w, h) != BFM_PLAT_OK) {
        target_free(x, t);
        x->ActiveTexture(GL_TEXTURE0);
        x->BindTexture(GL_TEXTURE_2D, x->vram_tex);
        return BFM_PLAT_ERROR;
    }
    /* targets can be (re)allocated in the middle of a command list: give
     * unit 0 back to the VRAM texture the draw shader samples */
    x->ActiveTexture(GL_TEXTURE0);
    x->BindTexture(GL_TEXTURE_2D, x->vram_tex);
    t->w = w;
    t->h = h;
    return BFM_PLAT_OK;
}

/* MSAA: resolve the 1x rect r of a multisampled target into its
 * single-sample texture, which every reader then sees. Source and
 * destination rects are identical (the only blit allowed out of a
 * multisampled framebuffer). Does nothing for a single-sample target. */
static void target_resolve(BfmGlExec *x, Target *t, const BfmPlatRect *r) {
    int s = x->scale, x0, y0, x1, y1;
    if (!t->ms_fbo) return;
    x0 = r->x < 0 ? 0 : r->x;
    y0 = r->y < 0 ? 0 : r->y;
    x1 = r->x + r->w > t->w ? t->w : r->x + r->w;
    y1 = r->y + r->h > t->h ? t->h : r->y + r->h;
    if (x0 >= x1 || y0 >= y1) return;
    x->Disable(GL_SCISSOR_TEST);       /* a blit is clipped by it */
    x->BindFramebuffer(GL_READ_FRAMEBUFFER, t->ms_fbo);
    x->BindFramebuffer(GL_DRAW_FRAMEBUFFER, t->fbo);
    x->BlitFramebuffer(x0 * s, y0 * s, x1 * s, y1 * s, x0 * s, y0 * s, x1 * s, y1 * s,
                       GL_COLOR_BUFFER_BIT, GL_NEAREST);
    x->stats.resolves++;
}

/* The uniform locations of a draw program, and making them the active set (x->u_*). */
static void loc_load(BfmGlExec *x, GLuint prog, DrawLoc *l) {
    l->size = x->GetUniformLocation(prog, "u_size");
    l->flip = x->GetUniformLocation(prog, "u_flip");
    l->half = x->GetUniformLocation(prog, "u_half");
    l->pass = x->GetUniformLocation(prog, "u_pass");
    l->pass_bit = x->GetUniformLocation(prog, "u_pass_bit");
    l->dither = x->GetUniformLocation(prog, "u_dither");
    l->scale = x->GetUniformLocation(prog, "u_scale");
    l->win = x->GetUniformLocation(prog, "u_win");
    l->vram = x->GetUniformLocation(prog, "u_vram");
    l->repl = x->GetUniformLocation(prog, "u_repl");
}

static void loc_use(BfmGlExec *x, const DrawLoc *l) {
    x->u_size = l->size;
    x->u_flip = l->flip;
    x->u_half = l->half;
    x->u_pass = l->pass;
    x->u_pass_bit = l->pass_bit;
    x->u_dither = l->dither;
    x->u_scale = l->scale;
    x->u_win = l->win;
    x->u_vram = l->vram;
    x->u_repl = l->repl;
}

/* The draw program for the next draws: the multisampled variant for the
 * multisampled targets, else the single-sample one (the window overlay too). */
static void select_prog(BfmGlExec *x, int ms) {
    x->prog = ms ? x->ms_prog : x->base_prog;
    loc_use(x, ms ? &x->ms_loc : &x->base_loc);
}

BfmGlExec *bfm_gl_exec_create(BfmGlGetProc gp, int scale, char *err, size_t n) {
    BfmGlExec *x;
    static const uint8_t white[4] = {255, 255, 255, 255};
    const char *missing = NULL;
    if (err && n) err[0] = '\0';
    if (!gp) return NULL;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    if (!(x = (BfmGlExec *)calloc(1, sizeof *x))) return NULL;
    /* object -> function pointer via memcpy (POSIX representation;
     * ISO C has no cast for it) */
#define X(type, name)                                                      \
    {                                                                      \
        void *p_ = gp("gl" #name);                                         \
        memcpy(&x->name, &p_, sizeof p_);                                  \
        if (!p_ && !missing) missing = "gl" #name;                         \
    }
    BFM_GL_FUNCS(X)
#undef X
    if (missing) {
        if (err && n) snprintf(err, n, "GL function %s not available", missing);
        free(x);
        return NULL;
    }
    x->scale = scale;
    x->getproc = gp;
    {
        const char *fs[4];
        fs[0] = fs_src; fs[1] = fs_main; fs[2] = fs_tex; fs[3] = fs_out;
        x->prog = x->base_prog = link(x, vs_src, fs, 4, err, n);
    }
    if (!x->prog || !(x->blit = link(x, blit_vs, &blit_fs, 1, err, n))) {
        bfm_gl_exec_destroy(x);
        return NULL;
    }
    loc_load(x, x->base_prog, &x->base_loc);
    loc_use(x, &x->base_loc);
    x->b_src = x->GetUniformLocation(x->blit, "u_src");
    x->b_tex = x->GetUniformLocation(x->blit, "u_tex");
    x->b_alpha = x->GetUniformLocation(x->blit, "u_alpha_test");
    x->b_five = x->GetUniformLocation(x->blit, "u_five");
    x->GenFramebuffers(1, &x->snap_fbo);

    x->GenVertexArrays(1, &x->vao);
    x->GenVertexArrays(1, &x->empty_vao);
    x->GenBuffers(1, &x->vbo);
    x->BindVertexArray(x->vao);
    x->BindBuffer(GL_ARRAY_BUFFER, x->vbo);
    x->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BfmGlVertex),
                           (const void *)offsetof(BfmGlVertex, x));
    x->VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BfmGlVertex),
                           (const void *)offsetof(BfmGlVertex, r));
    x->VertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(BfmGlVertex),
                           (const void *)offsetof(BfmGlVertex, u));
    x->VertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, sizeof(BfmGlVertex),
                            (const void *)offsetof(BfmGlVertex, clut));
    x->VertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(BfmGlVertex),
                           (const void *)offsetof(BfmGlVertex, s));
    x->VertexAttribIPointer(5, 4, GL_SHORT, sizeof(BfmGlVertex),
                            (const void *)offsetof(BfmGlVertex, txy));
    x->VertexAttribIPointer(6, 2, GL_SHORT, sizeof(BfmGlVertex),
                            (const void *)(offsetof(BfmGlVertex, txy) + 4 * sizeof(int16_t)));
    x->VertexAttribIPointer(7, 3, GL_UNSIGNED_INT, sizeof(BfmGlVertex),
                            (const void *)offsetof(BfmGlVertex, tuv));
    x->VertexAttribIPointer(8, 3, GL_UNSIGNED_INT, sizeof(BfmGlVertex),
                            (const void *)offsetof(BfmGlVertex, trgb));
    x->EnableVertexAttribArray(0);
    x->EnableVertexAttribArray(1);
    x->EnableVertexAttribArray(2);
    x->EnableVertexAttribArray(3);
    x->EnableVertexAttribArray(4);
    x->EnableVertexAttribArray(5);
    x->EnableVertexAttribArray(6);
    x->EnableVertexAttribArray(7);
    x->EnableVertexAttribArray(8);
    x->BindVertexArray(0);

    x->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    x->PixelStorei(GL_PACK_ALIGNMENT, 1);
    x->GenTextures(1, &x->vram_tex);
    x->BindTexture(GL_TEXTURE_2D, x->vram_tex);
    x->TexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, BFM_GL_VRAM_W, BFM_GL_VRAM_H, 0,
                  GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    x->white_tex = bfm_gl_exec_texture(x, 1, 1, white);

    if (target_alloc(x, &x->tgt[0], BFM_GL_VRAM_W, BFM_GL_VRAM_H) != BFM_PLAT_OK) {
        if (err && n) snprintf(err, n, "VRAM render target incomplete at %dx", scale);
        bfm_gl_exec_destroy(x);
        return NULL;
    }
    note_err(x);
    if (x->err) {
        if (err && n) snprintf(err, n, "GL error 0x%x during setup", x->err);
        bfm_gl_exec_destroy(x);
        return NULL;
    }
    return x;
}

void bfm_gl_exec_destroy(BfmGlExec *x) {
    int i;
    if (!x) return;
    for (i = 0; i < 3; i++) target_free(x, &x->tgt[i]);
    for (i = 0; i < BFM_GL_MAX_SNAPSHOTS; i++)
        if (x->snap[i].tex) x->DeleteTextures(1, &x->snap[i].tex);
    if (x->snap_fbo) x->DeleteFramebuffers(1, &x->snap_fbo);
    if (x->vram_tex) x->DeleteTextures(1, &x->vram_tex);
    if (x->white_tex) x->DeleteTextures(1, &x->white_tex);
    if (x->image_tex) x->DeleteTextures(1, &x->image_tex);
    if (x->vbo) x->DeleteBuffers(1, &x->vbo);
    if (x->vao) x->DeleteVertexArrays(1, &x->vao);
    if (x->empty_vao) x->DeleteVertexArrays(1, &x->empty_vao);
    if (x->base_prog) x->DeleteProgram(x->base_prog);
    if (x->ms_prog) x->DeleteProgram(x->ms_prog);
    if (x->ms_stencil_prog) x->DeleteProgram(x->ms_stencil_prog);
    if (x->blit) x->DeleteProgram(x->blit);
    free(x);
}

int bfm_gl_exec_scale(const BfmGlExec *x) { return x ? x->scale : 1; }

/* MSAA: halve the request until the VRAM target's multisample colour (the
 * largest target; mask-check adds as much again for the stencil) fits 256 MiB
 * at this internal scale; 0 when even 2 samples do not. */
static int msaa_fit(int scale, int n) {
    while (n >= 2 && (size_t)BFM_GL_VRAM_W * BFM_GL_VRAM_H * (size_t)scale * (size_t)scale * 4u *
                             (size_t)n > ((size_t)256u << 20))
        n /= 2;
    return n >= 2 ? n : 0;
}

/* (Re)allocates the VRAM target at the current scale: multisampled when
 * msaa_n0 fits, else single-sample. A multisample target that will not
 * allocate turns MSAA off for good. msaa becomes what the target really
 * holds. */
static int vram_target_realloc(BfmGlExec *x) {
    x->msaa_alloc = x->msaa_n0 ? msaa_fit(x->scale, x->msaa_n0) : 0;
    if (x->msaa_alloc) {
        if (target_alloc(x, &x->tgt[0], BFM_GL_VRAM_W, BFM_GL_VRAM_H) == BFM_PLAT_OK) {
            x->msaa = x->tgt[0].ms_samples;
            return BFM_PLAT_OK;
        }
        snprintf(x->msaa_note, sizeof x->msaa_note,
                 "MSAA off: a %d-sample VRAM target at %dx did not allocate", x->msaa_alloc,
                 x->scale);
        x->msaa_n0 = x->msaa_alloc = 0;
    } else if (x->msaa_n0) {
        snprintf(x->msaa_note, sizeof x->msaa_note,
                 "MSAA off at %dx: even 2 samples exceed the 256 MiB budget of the VRAM target", x->scale);
    }
    x->msaa = 0;
    return target_alloc(x, &x->tgt[0], BFM_GL_VRAM_W, BFM_GL_VRAM_H);
}

int bfm_gl_exec_set_scale(BfmGlExec *x, int scale) {
    int i;
    if (!x || scale < 1 || scale > 8) return BFM_PLAT_INVALID;
    if (scale == x->scale) return BFM_PLAT_OK;
    x->scale = scale;
    for (i = 1; i < 3; i++) target_free(x, &x->tgt[i]);
    if (x->msaa_n0) return vram_target_realloc(x);   /* the memory fit depends on the scale */
    return target_alloc(x, &x->tgt[0], BFM_GL_VRAM_W, BFM_GL_VRAM_H);
}

/* MSAA off: the single-sample program and targets again. */
static void msaa_drop_programs(BfmGlExec *x) {
    if (x->ms_prog) x->DeleteProgram(x->ms_prog);
    if (x->ms_stencil_prog) x->DeleteProgram(x->ms_stencil_prog);
    x->ms_prog = x->ms_stencil_prog = 0;
    memset(&x->ms_loc, 0, sizeof x->ms_loc);
    x->msaa = x->msaa_n0 = x->msaa_alloc = 0;
    select_prog(x, 0);
}

/* A one-line, line-break-free copy of a GL info log for msaa_note. */
static void note_log(char *dst, size_t n, const char *prefix, const char *log) {
    char tmp[96];
    size_t i;
    snprintf(tmp, sizeof tmp, "%s", log);
    for (i = 0; tmp[i]; i++)
        if (tmp[i] == '\n' || tmp[i] == '\r') tmp[i] = ' ';
    snprintf(dst, n, "%s%s", prefix, tmp);
}

int bfm_gl_exec_set_msaa(BfmGlExec *x, int samples) {
    char *src[5], log[512];
    const char *fs[4], *missing = NULL;
    GLint max_s = 0, max_c = 0;
    GLuint prog, sprog;
    int i, n0, bad;
    if (!x) return 0;
    x->msaa_note[0] = '\0';
    if (samples < 2) {                       /* off (or "1x"): nothing was changed unless it was on */
        if (x->msaa_n0 || x->ms_prog) {
            msaa_drop_programs(x);
            for (i = 1; i < 3; i++) target_free(x, &x->tgt[i]);
            target_alloc(x, &x->tgt[0], BFM_GL_VRAM_W, BFM_GL_VRAM_H);
        }
        return 0;
    }
    if (x->msaa_n0 || x->ms_prog) msaa_drop_programs(x);   /* a new request starts from single-sample */
#define X(type, name)                                                      \
    {                                                                      \
        void *p_ = x->getproc ? x->getproc("gl" #name) : NULL;             \
        memcpy(&x->ms_##name, &p_, sizeof p_);                             \
        if (!p_ && !missing) missing = "gl" #name;                         \
    }
    BFM_GL_MS_FUNCS(X)
#undef X
    if (missing) {
        snprintf(x->msaa_note, sizeof x->msaa_note, "MSAA off: GL function %s not available", missing);
        return 0;
    }
    note_err(x);                             /* bank what is pending, then drop the limit queries' own errors */
    x->GetIntegerv(GL_MAX_SAMPLES, &max_s);
    x->GetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_c);
    while (x->GetError() != GL_NO_ERROR) {}
    if (max_c < max_s) max_s = max_c;
    if (!(n0 = bfm_gl_msaa_effective(samples, max_s))) {
        snprintf(x->msaa_note, sizeof x->msaa_note,
                 "MSAA off: the GPU supports fewer than 2 samples (max %d)", max_s);
        return 0;
    }
    if ((bad = ms_sources(src)) != 0) {
        snprintf(x->msaa_note, sizeof x->msaa_note,
                 "MSAA off: %d shader patch(es) did not match exactly once", bad);
        for (i = 0; i < 5; i++) free(src[i]);
        return 0;
    }
    fs[0] = src[1]; fs[1] = src[2]; fs[2] = src[3]; fs[3] = src[4];
    log[0] = '\0';
    prog = link(x, src[0], fs, 4, log, sizeof log);
    for (i = 0; i < 5; i++) free(src[i]);
    if (!prog) {
        note_log(x->msaa_note, sizeof x->msaa_note, "MSAA off: draw program: ", log);
        return 0;
    }
    log[0] = '\0';
    sprog = link(x, blit_vs, &ms_stencil_fs, 1, log, sizeof log);
    if (!sprog) {
        x->DeleteProgram(prog);
        note_log(x->msaa_note, sizeof x->msaa_note, "MSAA off: stencil program: ", log);
        return 0;
    }
    x->ms_prog = prog;
    x->ms_stencil_prog = sprog;
    loc_load(x, prog, &x->ms_loc);
    x->s_ms = x->GetUniformLocation(sprog, "u_ms");
    x->s_sample = x->GetUniformLocation(sprog, "u_sample");
    x->msaa_n0 = n0;
    for (i = 1; i < 3; i++) target_free(x, &x->tgt[i]);
    vram_target_realloc(x);                  /* sets msaa, or turns MSAA off for good (and says why) */
    if (!x->msaa) {
        msaa_drop_programs(x);
        return 0;
    }
    snprintf(x->msaa_note, sizeof x->msaa_note,
             "asked GL for %d samples (request %d, GPU max %d); the framebuffers hold %d",
             x->msaa_alloc, samples, max_s, x->msaa);
    return x->msaa;
}

int bfm_gl_exec_msaa(const BfmGlExec *x) { return x ? x->msaa : 0; }

const char *bfm_gl_exec_msaa_note(const BfmGlExec *x) { return x ? x->msaa_note : ""; }

void bfm_gl_exec_msaa_stats(const BfmGlExec *x, BfmGlMsaaStats *out) {
    if (!out) return;
    if (x) *out = x->stats;
    else memset(out, 0, sizeof *out);
}

void bfm_gl_exec_set_integer(BfmGlExec *x, int on) {
    if (x) x->integer = on != 0;
}

void bfm_gl_exec_sync_vram(BfmGlExec *x, BfmGlVram *v) {
    int y;
    if (!x || !v || !v->dirty) return;
    x->BindTexture(GL_TEXTURE_2D, x->vram_tex);
    x->PixelStorei(GL_UNPACK_ALIGNMENT, 2);
    for (y = v->dy0; y < v->dy1; y++)
        x->TexSubImage2D(GL_TEXTURE_2D, 0, v->dx0, y, v->dx1 - v->dx0, 1,
                         GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                         &v->px[(size_t)y * BFM_GL_VRAM_W + v->dx0]);
    x->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    v->dirty = 0;
}

/* The render target for a command target, sized for the batch. */
static Target *target_for(BfmGlExec *x, const BfmGlBatch *b, int target) {
    Target *t;
    int w, h;
    if (target == BFM_GL_TARGET_VRAM) return &x->tgt[0];
    if (target != BFM_GL_TARGET_SCENE0 && target != BFM_GL_TARGET_SCENE1) return NULL;
    t = &x->tgt[target];
    bfm_gl_batch_target_size(b, target, &w, &h);
    if ((t->w != w || t->h != h) && target_alloc(x, t, w, h) != BFM_PLAT_OK)
        return NULL;
    return t;
}

static void set_blend(BfmGlExec *x, int mode) {
    if (mode == x->cur_blend) return;
    x->cur_blend = mode;
    if (mode < 0) {
        x->Disable(GL_BLEND);
        return;
    }
    x->Enable(GL_BLEND);
    if (mode == BFM_GL_BLEND_DUAL) {
        /* dst * src1 + src (the shader premultiplies by the src factor);
         * alpha = the fragment's mask bit (src1.a = 0) */
        x->BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        x->BlendFuncSeparate(GL_ONE, GL_SRC1_COLOR, GL_ONE, GL_SRC1_ALPHA);
        return;
    }
    {
        BfmGlBlendState st = bfm_gl_blend_state(mode);
        /* colour blends; alpha (the mask bit) is written as the fragment's */
        x->BlendEquationSeparate(st.equation == BFM_GL_FUNC_REVERSE_SUBTRACT
                                     ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_ADD,
                                 GL_FUNC_ADD);
        x->BlendFuncSeparate(st.src == BFM_GL_CONSTANT_COLOR ? GL_CONSTANT_COLOR : GL_ONE,
                             st.dst == BFM_GL_CONSTANT_COLOR ? GL_CONSTANT_COLOR : GL_ONE,
                             GL_ONE, GL_ZERO);
        x->BlendColor(st.constant, st.constant, st.constant, st.constant);
    }
}

/* The context is borrowed (PsyCross made it): make sure no sample state left
 * by anyone else can turn the multisampled draws into something else.
 * GL_MULTISAMPLE is on by default in a core context, but is not assumed. */
static void msaa_state(BfmGlExec *x) {
    x->Enable(GL_MULTISAMPLE);
    x->Disable(GL_SAMPLE_MASK);
    x->Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);   /* would turn the mask-bit alpha into coverage */
    x->Disable(GL_SAMPLE_ALPHA_TO_ONE);
    x->Disable(GL_SAMPLE_COVERAGE);
}

/* Binds the draw program and its fixed state (after anything that changed
 * the program, VAO or texture units). */
static void draw_state(BfmGlExec *x) {
    x->cur_blend = -2;
    x->cur_pass = -2;
    x->cur_dither = -2;
    x->cur_win.w = -1;
    x->UseProgram(x->prog);
    x->Uniform1i(x->u_vram, 0);
    x->Uniform1i(x->u_repl, 1);
    x->ActiveTexture(GL_TEXTURE0);
    x->BindTexture(GL_TEXTURE_2D, x->vram_tex);
    x->BindVertexArray(x->vao);
}

/* MSAA mask-check: the same stencil, one bit per sample. The multisample
 * stencil renderbuffer joins the draw FBO (the colour texture's sample
 * count); a stencil-only FBO over the same renderbuffer is what the build
 * renders into, so the colour texture is sampled without ever being attached
 * to the framebuffer being drawn (no feedback loop, no copy). Each sample gets
 * its own full-target pass under GL_SAMPLE_MASK: stencil 1 where that
 * sample's alpha (the mask bit) is set. */
static int stencil_enable_ms(BfmGlExec *x, Target *t) {
    int w = t->w * x->scale, h = t->h * x->scale, i, ok;
    note_err(x);                 /* keep what is pending; the check below sees only this call's error */
    x->GenRenderbuffers(1, &t->stencil);
    x->BindRenderbuffer(GL_RENDERBUFFER, t->stencil);
    x->ms_RenderbufferStorageMultisample(GL_RENDERBUFFER, t->ms_samples, GL_DEPTH24_STENCIL8, w, h);
    x->BindFramebuffer(GL_FRAMEBUFFER, t->ms_fbo);
    x->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                               t->stencil);
    ok = x->GetError() == GL_NO_ERROR &&
         x->CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (ok) {
        x->GenFramebuffers(1, &t->st_fbo);
        x->BindFramebuffer(GL_FRAMEBUFFER, t->st_fbo);
        x->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                   t->stencil);
        x->ms_DrawBuffer(GL_NONE);
        x->ms_ReadBuffer(GL_NONE);
        ok = x->CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    if (!ok) {   /* the draws go on without a stencil, into the multisample FBO left bound */
        x->BindFramebuffer(GL_FRAMEBUFFER, t->ms_fbo);
        x->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        if (t->st_fbo) x->DeleteFramebuffers(1, &t->st_fbo);
        x->DeleteRenderbuffers(1, &t->stencil);
        t->st_fbo = t->stencil = 0;
        return BFM_PLAT_ERROR;
    }
    x->BindFramebuffer(GL_FRAMEBUFFER, t->st_fbo);
    x->Viewport(0, 0, w, h);
    x->Disable(GL_SCISSOR_TEST);
    x->Disable(GL_BLEND);
    x->ClearStencil(0);
    x->Clear(GL_STENCIL_BUFFER_BIT);
    x->Enable(GL_STENCIL_TEST);
    x->StencilFunc(GL_ALWAYS, 1, 0xFF);
    x->StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    x->Enable(GL_SAMPLE_MASK);
    x->UseProgram(x->ms_stencil_prog);
    x->Uniform1i(x->s_ms, 0);
    x->ActiveTexture(GL_TEXTURE0);
    x->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->ms_tex);
    x->BindVertexArray(x->empty_vao);
    for (i = 0; i < t->ms_samples; i++) {
        x->ms_SampleMaski(0, 1u << i);
        x->Uniform1i(x->s_sample, i);
        x->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    x->Disable(GL_SAMPLE_MASK);          /* later draws must write every sample again */
    x->ms_SampleMaski(0, 0xFFFFFFFFu);
    x->Disable(GL_STENCIL_TEST);
    x->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    x->stats.stencil_builds++;
    return BFM_PLAT_OK;
}

/* Mask-check needs a stencil copy of the mask bits. The first time a
 * target sees a check, attach one and build it from the alpha channel
 * (which always holds the mask bit), through a temporary copy so the
 * target is never sampled while bound. From then on every draw keeps the
 * stencil in step. */
static int stencil_enable(BfmGlExec *x, Target *t) {
    int w = t->w * x->scale, h = t->h * x->scale;
    GLuint tmp = 0, tmp_fbo = 0;
    if (t->stencil) return BFM_PLAT_OK;
    if (t->ms_fbo) return stencil_enable_ms(x, t);
    x->GenRenderbuffers(1, &t->stencil);
    x->BindRenderbuffer(GL_RENDERBUFFER, t->stencil);
    x->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    x->BindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    x->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_RENDERBUFFER, t->stencil);
    if (x->CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        x->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_RENDERBUFFER, 0);
        x->DeleteRenderbuffers(1, &t->stencil);
        t->stencil = 0;
        return BFM_PLAT_ERROR;
    }
    /* copy colour+alpha out */
    x->GenTextures(1, &tmp);
    x->BindTexture(GL_TEXTURE_2D, tmp);
    x->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    x->GenFramebuffers(1, &tmp_fbo);
    x->BindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp_fbo);
    x->FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmp, 0);
    x->BindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
    x->Disable(GL_SCISSOR_TEST);
    x->BlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    /* stencil = 1 where alpha is set */
    x->BindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    x->Viewport(0, 0, w, h);
    x->ClearStencil(0);
    x->Clear(GL_STENCIL_BUFFER_BIT);
    x->Enable(GL_STENCIL_TEST);
    x->StencilFunc(GL_ALWAYS, 1, 0xFF);
    x->StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    x->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    x->Disable(GL_BLEND);
    x->UseProgram(x->blit);
    x->Uniform1i(x->b_tex, 0);
    x->Uniform1i(x->b_alpha, 1);
    x->Uniform4f(x->b_src, 0.0f, 1.0f, 1.0f, 0.0f);   /* row r -> row r */
    x->ActiveTexture(GL_TEXTURE0);
    x->BindTexture(GL_TEXTURE_2D, tmp);
    x->BindVertexArray(x->empty_vao);
    x->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    x->Uniform1i(x->b_alpha, 0);
    x->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    x->Disable(GL_STENCIL_TEST);
    x->DeleteFramebuffers(1, &tmp_fbo);
    x->DeleteTextures(1, &tmp);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    return BFM_PLAT_OK;
}

/* SNAPSHOT: copy a target-local rect at device resolution. */
static void take_snapshot(BfmGlExec *x, Target *t, const BfmGlCmd *c) {
    Snapshot *sn;
    int s = x->scale, w = c->rect.w * s, h = c->rect.h * s;
    if (!c->snap || c->snap > BFM_GL_MAX_SNAPSHOTS) return;
    sn = &x->snap[c->snap - 1];
    if (!sn->tex) {
        x->GenTextures(1, &sn->tex);
        x->BindTexture(GL_TEXTURE_2D, sn->tex);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (sn->w != w || sn->h != h) {
        x->BindTexture(GL_TEXTURE_2D, sn->tex);
        x->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        sn->w = w;
        sn->h = h;
    }
    if (t->ms_fbo) {             /* MSAA: copy the resolved image */
        target_resolve(x, t, &c->rect);
        x->stats.snapshots++;
    }
    x->BindFramebuffer(GL_DRAW_FRAMEBUFFER, x->snap_fbo);
    x->FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            sn->tex, 0);
    x->BindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
    x->Disable(GL_SCISSOR_TEST);
    x->BlitFramebuffer(c->rect.x * s, c->rect.y * s, (c->rect.x + c->rect.w) * s,
                       (c->rect.y + c->rect.h) * s, 0, 0, w, h, GL_COLOR_BUFFER_BIT,
                       GL_NEAREST);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* pass: 0 all fragments, 1 bit clear, 2 bit set; by_written: the bit is
 * the written bit 15 instead of STP */
static void set_pass(BfmGlExec *x, int pass, int by_written) {
    int v = pass | (by_written << 4);
    if (v == x->cur_pass) return;
    x->cur_pass = v;
    x->Uniform1i(x->u_pass, pass);
    x->Uniform1i(x->u_pass_bit, by_written);
}

static void stencil_pass(BfmGlExec *x, int check, int ref) {
    /* check: pass only where the stored bit is 0; the REPLACE value is the
     * bit this pass writes (GREATER with ref 1 is "stencil < 1") */
    if (check) x->StencilFunc(ref ? GL_GREATER : GL_EQUAL, ref, 1);
    else x->StencilFunc(GL_ALWAYS, ref, 1);
    x->StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

static int readback_target(BfmGlExec *x, Target *t, const BfmPlatRect *r, uint16_t *out);

/* Draws one command list's commands for a bound target. win_vp: for the
 * window target, the viewport (x, y, w, h) in window pixels. */
static void run_cmds(BfmGlExec *x, const BfmGlBatch *b, int dither,
                     const int *win_vp) {
    size_t i;
    GLuint bound_repl = (GLuint)-1;
    int cur_target = -1, scale = x->scale, stencil = 0;
    int ms = x->msaa != 0 && !win_vp;   /* the overlay (win_vp) draws to the window, never multisampled */
    Target *t = NULL;
    select_prog(x, ms);
    if (ms) msaa_state(x);
    draw_state(x);
    for (i = 0; i < b->nc; i++) {
        const BfmGlCmd *c = &b->c[i];
        int w, h;
        if (c->kind == BFM_GL_CMD_SNAPSHOT || c->kind == BFM_GL_CMD_RESOLVE) {
            Target *src = target_for(x, b, c->target);
            if (!src) continue;
            if (c->kind == BFM_GL_CMD_SNAPSHOT) {
                take_snapshot(x, src, c);
            } else if (b->vram) {
                uint16_t *tmp = (uint16_t *)malloc((size_t)c->rect.w * c->rect.h * 2u);
                if (tmp && readback_target(x, src, &c->rect, tmp) == BFM_PLAT_OK) {
                    bfm_gl_vram_upload(b->vram, &c->vrect, tmp);
                    x->ActiveTexture(GL_TEXTURE0);
                    bfm_gl_exec_sync_vram(x, b->vram);
                }
                free(tmp);
            }
            draw_state(x);
            cur_target = -1;
            bound_repl = (GLuint)-1;
            continue;
        }
        if (c->target != cur_target) {
            if (c->target == BFM_GL_TARGET_WINDOW) {
                if (!win_vp) continue;
                x->BindFramebuffer(GL_FRAMEBUFFER, 0);
                x->Viewport(win_vp[0], win_vp[1], win_vp[2], win_vp[3]);
                x->Disable(GL_SCISSOR_TEST);
                x->Disable(GL_STENCIL_TEST);
                bfm_gl_batch_target_size(b, BFM_GL_TARGET_WINDOW, &w, &h);
                x->Uniform1i(x->u_flip, 1);
                x->Uniform1i(x->u_scale, 1);
                x->Uniform1f(x->u_half, 0.0f);   /* overlay: display pixels as given */
                t = NULL;
                stencil = 0;
            } else {
                if (!(t = target_for(x, b, c->target))) continue;
                x->BindFramebuffer(GL_FRAMEBUFFER, draw_fbo(t));
                x->Viewport(0, 0, t->w * scale, t->h * scale);
                x->Enable(GL_SCISSOR_TEST);
                w = t->w;
                h = t->h;
                x->Uniform1i(x->u_flip, 0);
                x->Uniform1i(x->u_scale, scale);
                x->Uniform1f(x->u_half, 0.5f / (float)scale);
                stencil = t->stencil != 0;
                if (stencil) x->Enable(GL_STENCIL_TEST);
                else x->Disable(GL_STENCIL_TEST);
            }
            x->Uniform2f(x->u_size, (float)w, (float)h);
            cur_target = c->target;
        }
        if (c->kind == BFM_GL_CMD_DRAW && c->mask_check && t && !t->stencil) {
            if (stencil_enable(x, t) == BFM_PLAT_OK) {
                draw_state(x);
                bound_repl = (GLuint)-1;
                x->BindFramebuffer(GL_FRAMEBUFFER, draw_fbo(t));
                x->Viewport(0, 0, t->w * scale, t->h * scale);
                x->Enable(GL_SCISSOR_TEST);
                x->Enable(GL_STENCIL_TEST);
                stencil = 1;
            }
        }
        if (c->target != BFM_GL_TARGET_WINDOW)
            x->Scissor(c->rect.x * scale, c->rect.y * scale, c->rect.w * scale,
                       c->rect.h * scale);
        if (c->kind == BFM_GL_CMD_FILL) {
            if (c->target == BFM_GL_TARGET_WINDOW) continue;
            x->ClearColor((float)(c->rgb & 0xFF) / 255.0f,
                          (float)((c->rgb >> 8) & 0xFF) / 255.0f,
                          (float)((c->rgb >> 16) & 0xFF) / 255.0f, 0.0f);
            if (stencil) x->ClearStencil(0);
            x->Clear(GL_COLOR_BUFFER_BIT | (stencil ? GL_STENCIL_BUFFER_BIT : 0));
            continue;
        }
        if (!c->count) continue;
        {
            GLuint want = x->white_tex;
            if (c->snap && c->snap <= BFM_GL_MAX_SNAPSHOTS && x->snap[c->snap - 1].tex)
                want = x->snap[c->snap - 1].tex;
            else if (c->repl && b->repl && b->repl->r[c->repl - 1].live)
                want = b->repl->r[c->repl - 1].id;
            if (want != bound_repl) {
                x->ActiveTexture(GL_TEXTURE1);
                x->BindTexture(GL_TEXTURE_2D, want);
                x->ActiveTexture(GL_TEXTURE0);
                bound_repl = want;
            }
        }
        if (memcmp(&x->cur_win, &c->win, sizeof c->win) != 0) {
            x->Uniform4i(x->u_win, c->win.x, c->win.y, c->win.w, c->win.h);
            x->cur_win = c->win;
        }
        if (x->cur_dither != (dither && c->dither)) {
            x->cur_dither = dither && c->dither;
            x->Uniform1i(x->u_dither, x->cur_dither);
        }
        if (c->split_stp) {
            /* mode 2 textured: texels without STP opaque, then STP texels
             * subtracted; the command's prims all share mask_set */
            set_blend(x, -1);
            if (stencil) stencil_pass(x, c->mask_check, c->mask_set);
            set_pass(x, 1, 0);
            x->DrawArrays(GL_TRIANGLES, (GLint)c->first, (GLsizei)c->count);
            set_blend(x, c->blend);
            if (stencil) stencil_pass(x, c->mask_check, 1);
            set_pass(x, 2, 0);
            x->DrawArrays(GL_TRIANGLES, (GLint)c->first, (GLsizei)c->count);
        } else if (stencil && (c->textured || (c->mask_set && c->mask_clear))) {
            /* stencil upkeep: each fragment writes its bit 15 */
            set_blend(x, c->blend);
            stencil_pass(x, c->mask_check, 0);
            set_pass(x, 1, 1);
            x->DrawArrays(GL_TRIANGLES, (GLint)c->first, (GLsizei)c->count);
            stencil_pass(x, c->mask_check, 1);
            set_pass(x, 2, 1);
            x->DrawArrays(GL_TRIANGLES, (GLint)c->first, (GLsizei)c->count);
        } else {
            set_blend(x, c->blend);
            if (stencil) stencil_pass(x, c->mask_check, c->mask_set);
            set_pass(x, 0, 0);
            x->DrawArrays(GL_TRIANGLES, (GLint)c->first, (GLsizei)c->count);
        }
    }
    set_blend(x, -1);
    x->Disable(GL_SCISSOR_TEST);
    x->Disable(GL_STENCIL_TEST);
    x->BindVertexArray(0);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void upload_verts(BfmGlExec *x, const BfmGlBatch *b) {
    x->BindBuffer(GL_ARRAY_BUFFER, x->vbo);
    x->BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(b->nv * sizeof(BfmGlVertex)),
                  b->nv ? b->v : NULL, GL_STREAM_DRAW);
}

int bfm_gl_exec_run(BfmGlExec *x, const BfmGlBatch *b, int dither) {
    if (!x || !b) return BFM_PLAT_INVALID;
    if (!b->nc) return BFM_PLAT_OK;
    upload_verts(x, b);
    run_cmds(x, b, dither, NULL);
    note_err(x);
    return x->err ? BFM_PLAT_ERROR : BFM_PLAT_OK;
}

static int readback_target(BfmGlExec *x, Target *t, const BfmPlatRect *r, uint16_t *out) {
    uint8_t *buf;
    int s = x->scale, i, j;
    if (r->x < 0 || r->y < 0 || r->x + r->w > t->w || r->y + r->h > t->h)
        return BFM_PLAT_INVALID;
    buf = (uint8_t *)malloc((size_t)r->w * s * (size_t)r->h * s * 4u);
    if (!buf) return BFM_PLAT_ERROR;
    if (t->ms_fbo) {             /* MSAA: read the resolved image (ReadPixels cannot read samples) */
        target_resolve(x, t, r);
        x->stats.readbacks++;
    }
    x->BindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    x->ReadPixels(r->x * s, r->y * s, r->w * s, r->h * s, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    /* the first sub-pixel of each native pixel is the one whose centre sits
     * on the integer sample point (u_half), so it holds the native result */
    for (j = 0; j < r->h; j++)
        for (i = 0; i < r->w; i++) {
            const uint8_t *p = buf + (((size_t)(j * s) * (size_t)(r->w * s)) +
                                      (size_t)(i * s)) * 4u;
            out[(size_t)j * r->w + i] = (uint16_t)((p[0] >> 3) | ((p[1] >> 3) << 5) |
                                                   ((p[2] >> 3) << 10) |
                                                   (p[3] >= 128 ? 0x8000 : 0));
        }
    free(buf);
    note_err(x);
    return x->err ? BFM_PLAT_ERROR : BFM_PLAT_OK;
}

int bfm_gl_exec_readback(BfmGlExec *x, const BfmGlBatch *b, int target,
                         const BfmPlatRect *r, uint16_t *out) {
    Target *t;
    if (!x || !b || !r || !out || r->w <= 0 || r->h <= 0) return BFM_PLAT_INVALID;
    if (!(t = target_for(x, b, target))) return BFM_PLAT_INVALID;
    return readback_target(x, t, r, out);
}

int bfm_gl_exec_readback_rgba(BfmGlExec *x, const BfmGlBatch *b, int target,
                              const BfmPlatRect *r, uint8_t *out) {
    Target *t;
    int s;
    if (!x || !b || !r || !out || r->w <= 0 || r->h <= 0) return BFM_PLAT_INVALID;
    if (!(t = target_for(x, b, target))) return BFM_PLAT_INVALID;
    if (r->x < 0 || r->y < 0 || r->x + r->w > t->w || r->y + r->h > t->h)
        return BFM_PLAT_INVALID;
    s = x->scale;
    if (t->ms_fbo) {             /* MSAA: read the resolved image */
        target_resolve(x, t, r);
        x->stats.readbacks++;
    }
    x->BindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    x->ReadPixels(r->x * s, r->y * s, r->w * s, r->h * s, GL_RGBA, GL_UNSIGNED_BYTE, out);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    note_err(x);
    return x->err ? BFM_PLAT_ERROR : BFM_PLAT_OK;
}

void bfm_gl_exec_finish(BfmGlExec *x) {
    uint8_t px[4];
    /* a 1-pixel read is the portable "wait for the GPU" */
    if (!x) return;
    if (x->tgt[0].ms_fbo) {      /* MSAA: the draws land in the multisample texture; resolve a pixel so the read waits for them */
        BfmPlatRect one;
        one.x = one.y = 0;
        one.w = one.h = 1;
        target_resolve(x, &x->tgt[0], &one);
    }
    x->BindFramebuffer(GL_FRAMEBUFFER, x->tgt[0].fbo);
    x->ReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t bfm_gl_exec_texture(BfmGlExec *x, uint32_t w, uint32_t h,
                             const uint8_t *rgba) {
    GLuint t = 0;
    if (!x || !w || !h || !rgba) return 0;
    x->GenTextures(1, &t);
    x->BindTexture(GL_TEXTURE_2D, t);
    x->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, rgba);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    note_err(x);
    return t;
}

void bfm_gl_exec_texture_free(BfmGlExec *x, uint32_t id) {
    GLuint t = id;
    if (x && id) x->DeleteTextures(1, &t);
}

/* src_h: the displayed image's height in display lines (integer mode). */
static void fit(const BfmGlExec *x, int win_w, int win_h, double aspect, int src_h,
                int vp[4]) {
    bfm_plat_present_viewport(win_w, win_h, aspect, x->integer ? src_h : 0, vp);
}

/* Clears the window and draws texture region (s0,t0)-(s1,t1), t0 = top,
 * into viewport vp; five: see blit_fs. */
static void blit(BfmGlExec *x, GLuint tex, int win_w, int win_h, const int vp[4],
                 float s0, float t0, float s1, float t1, int five) {
    x->BindFramebuffer(GL_FRAMEBUFFER, 0);
    x->Disable(GL_SCISSOR_TEST);
    x->Disable(GL_BLEND);
    x->Viewport(0, 0, win_w, win_h);
    x->ClearColor(0, 0, 0, 1);
    x->Clear(GL_COLOR_BUFFER_BIT);
    x->Viewport(vp[0], vp[1], vp[2], vp[3]);
    x->UseProgram(x->blit);
    x->Uniform1i(x->b_tex, 0);
    x->Uniform4f(x->b_src, s0, t0, s1, t1);
    x->Uniform1i(x->b_five, five);
    x->ActiveTexture(GL_TEXTURE0);
    x->BindTexture(GL_TEXTURE_2D, tex);
    x->BindVertexArray(x->empty_vao);
    x->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    x->Uniform1i(x->b_five, 0);
    x->BindVertexArray(0);
}

/* Any HD replacement texture in use: their texels keep 8 bits on screen. */
static int repl_live(const BfmGlBatch *b) {
    int i;
    if (!b->repl) return 0;
    for (i = 0; i < BFM_GL_MAX_REPLACEMENTS; i++)
        if (b->repl->r[i].live) return 1;
    return 0;
}

int bfm_gl_exec_present_image(BfmGlExec *x, int w, int h, const uint8_t *rgba,
                              const BfmGlBatch *overlay, int win_w, int win_h,
                              double aspect) {
    int vp[4];
    if (!x || !rgba || w <= 0 || h <= 0 || win_w <= 0 || win_h <= 0)
        return BFM_PLAT_INVALID;
    if (!x->image_tex) {
        x->GenTextures(1, &x->image_tex);
        x->BindTexture(GL_TEXTURE_2D, x->image_tex);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        x->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    x->BindTexture(GL_TEXTURE_2D, x->image_tex);
    x->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    fit(x, win_w, win_h, aspect, h, vp);
    blit(x, x->image_tex, win_w, win_h, vp, 0.0f, 0.0f, 1.0f, 1.0f, 0);
    if (overlay && overlay->nc) {
        upload_verts(x, overlay);
        run_cmds(x, overlay, 0, vp);
    }
    note_err(x);
    return x->err ? BFM_PLAT_ERROR : BFM_PLAT_OK;
}

int bfm_gl_exec_present(BfmGlExec *x, const BfmGlBatch *b,
                        const BfmGlBatch *overlay, int win_w, int win_h,
                        double aspect, int dither) {
    int slot, vp[4], ivp[4], tw;
    float s0, t0, s1, t1;
    Target *src;
    if (!x || !b || win_w <= 0 || win_h <= 0) return BFM_PLAT_INVALID;
    fit(x, win_w, win_h, aspect, b->disp.h, vp);
    slot = bfm_gl_batch_scene_at(b, b->disp.x, b->disp.y);
    if (slot >= 0 && (src = target_for(x, b, BFM_GL_TARGET_SCENE0 + slot)) != NULL) {
        s0 = 0.0f; t0 = 0.0f; s1 = 1.0f; t1 = 1.0f;
        tw = src->w;
        /* the 4:3 display area inside the wide image, for the overlay */
        ivp[0] = vp[0] + (int)((double)vp[2] * b->margin / tw + 0.5);
        ivp[2] = (int)((double)vp[2] * b->disp.w / tw + 0.5);
    } else {
        src = &x->tgt[0];
        s0 = (float)b->disp.x / BFM_GL_VRAM_W;
        s1 = (float)(b->disp.x + b->disp.w) / BFM_GL_VRAM_W;
        t0 = (float)b->disp.y / BFM_GL_VRAM_H;
        t1 = (float)(b->disp.y + b->disp.h) / BFM_GL_VRAM_H;
        ivp[0] = vp[0];
        ivp[2] = vp[2];
    }
    ivp[1] = vp[1];
    ivp[3] = vp[3];
    if (src->ms_fbo) {           /* MSAA: show the resolved image, of the part that is shown */
        BfmPlatRect shown;
        if (src == &x->tgt[0]) {
            shown = b->disp;
        } else {
            shown.x = shown.y = 0;
            shown.w = (int16_t)src->w;
            shown.h = (int16_t)src->h;
        }
        target_resolve(x, src, &shown);
        x->stats.presents++;
    }
    blit(x, src->tex, win_w, win_h, vp, s0, t0, s1, t1, !repl_live(b));
    if (overlay && overlay->nc) {
        upload_verts(x, overlay);
        run_cmds(x, overlay, dither, ivp);
    }
    note_err(x);
    return x->err ? BFM_PLAT_ERROR : BFM_PLAT_OK;
}
