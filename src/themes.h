/* ══════════════════════════════════════════════════════════════════
 *  themes.h  —  Colour type, theme data, state, and background API
 * ══════════════════════════════════════════════════════════════════ */
#ifndef THEMES_H
#define THEMES_H

#include <SDL2/SDL.h>
#include <math.h>

/* ── Core colour type ────────────────────────────────────────────── */
typedef struct { Uint8 r,g,b,a; } C4;
#define MK4(r,g,b,a) ((C4){(Uint8)(r),(Uint8)(g),(Uint8)(b),(Uint8)(a)})

static inline float clampf(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
static inline float lerpf (float a,float b,float t)  {return a+(b-a)*t;}
static inline C4 lerpc(C4 a,C4 b,float t){
    return MK4((Uint8)lerpf(a.r,b.r,t),(Uint8)lerpf(a.g,b.g,t),
               (Uint8)lerpf(a.b,b.b,t),(Uint8)lerpf(a.a,b.a,t));
}
static inline C4 tintc(C4 c,float f){
    return MK4((Uint8)clampf(c.r*f,0,255),(Uint8)clampf(c.g*f,0,255),
               (Uint8)clampf(c.b*f,0,255),c.a);
}

/* ── Layout constants ────────────────────────────────────────────── */
#define TITLE_H   34
#define TB_BTN_W  40
#define HDR_Y     TITLE_H
#define HDR_H     68

/* ── Theme & animation constants ─────────────────────────────────── */
#define N_THEMES   9
#define N_CSLOTS  20
#define TC_SPEED   5.5f
#define TDOT_R     9
#define TDOT_STEP  24

/* ── Background texture constants ────────────────────────────────── */
#define GRAIN_SZ    256
#define BLOB_TEX_R  256
#define BLOB_TEX_SZ (BLOB_TEX_R*2)

/* ── Theme type ──────────────────────────────────────────────────── */
typedef struct {
    const char *name;
    C4 bg,hdr,title,tbar,rowa,rowb,rowh;
    C4 acc,txt,sub,dim;
    C4 scrbg,scrth,scrto;
    C4 sbar,srch,srcha,sep,btni,close;
} Theme;

extern const Theme THEMES[N_THEMES];

/* ── Live colour slots ───────────────────────────────────────────── */
extern C4 C_BG,C_HDR,C_TITLE,C_TBAR,C_ROWA,C_ROWB,C_ROWH,
          C_ACC,C_TXT,C_SUB,C_DIM,
          C_SCRBG,C_SCRTH,C_SCRTO,
          C_SBAR,C_SRCH,C_SRCHA,C_SEP,C_BTNI,C_CLOSE;

/* ── Animation state ─────────────────────────────────────────────── */
extern int   cur_theme;
extern float tdot_hov   [N_THEMES];
extern float tdot_bounce[N_THEMES];
extern float sel_ring_if;
extern float theme_pulse;
extern int   dots_in_tb;
extern int   dots_in_tb_prev;
extern int   dots_x0;
extern int   dots_cy;
extern float tc_t;
extern float galaxy_dot_f;
extern float galaxy_anim_t;
extern int   gal_paused;

/* ── Search bar layout ───────────────────────────────────────────── */
extern int sr_x;
extern int sr_w;

/* ── Public API ──────────────────────────────────────────────────── */
void set_theme        (int idx);
void set_theme_instant(int idx);
void tick_theme       (float dt);
void compute_dot_layout(void);
int  hit_theme_dot    (int mx, int my);
void bg_init          (void);
void bg_free          (void);
void draw_background  (void);
void draw_galaxy_dot_fx(int cx, int cy, int r,
                        float gf, float hf, float pa, float dt);

#endif /* THEMES_H */