/* ══════════════════════════════════════════════════════════════════
 *  themes.c  —  Theme data, colour transitions, background rendering
 *
 *  Galaxy background  (theme 8)  —  cinematic deep-space renderer
 *  ─────────────────────────────────────────────────────────────────
 *  Pass 1  Star field  — 900 stars seeded along three logarithmic
 *          spiral arms + background scatter.  Three depth tiers with
 *          mouse parallax, per-star twinkle, diffraction spikes on
 *          bright near-stars.  Temperature palette: blue-white,
 *          ice-white, electric violet, warm amber, hot pink.
 *
 *  Pass 2  Cosmic dust wisps  — thin additive line arcs that trace
 *          the three spiral arms.  Each wisp drifts slowly and fades
 *          in/out for a living, breathing feel.
 *
 *  Pass 3  Nebula clouds  — 10 large additive Gaussian blobs in deep
 *          indigo, electric purple, cobalt blue, and hot magenta.
 *          Independent positional drift + breathing per blob.
 *
 *  Pass 4  Galactic core  — seven concentric additive halos, from a
 *          wide warm violet outer glow down to a razor-bright
 *          blue-white nucleus with four-arm diffraction spikes.
 *
 *  Pass 5  Core star cluster  — 80 bright stars densely packed around
 *          the nucleus, drawn with mini diffraction crosses.
 *
 *  Pass 6  Film-grain overlay  (shared with all themes).
 * ══════════════════════════════════════════════════════════════════ */
#include "themes.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern SDL_Renderer *ren;
extern int win_w, win_h;
extern void sc_   (C4 c);
extern void fblend(int x, int y, int w, int h, C4 c);
extern void frr_aa(int x, int y, int w, int h, int r, C4 c);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ══════════════════════════════════════════════════════════════════
 *  THEME DEFINITIONS
 * ══════════════════════════════════════════════════════════════════ */
const Theme THEMES[N_THEMES] = {
    {"Void",
     {10,10,22,255},{6,6,16,255},{4,4,11,255},{14,14,28,255},
     {18,18,36,255},{15,15,30,255},{30,30,62,255},
     {108,72,228,255},{225,225,240,255},{115,115,150,255},{52,52,82,255},
     {16,16,32,255},{55,55,100,255},{90,90,150,255},
     {7,7,17,255},{20,20,42,255},{26,26,50,255},{22,22,44,255},{18,18,40,255},{220,58,58,255}},
    {"Ocean",
     {6,12,24,255},{4,8,18,255},{3,6,14,255},{8,14,28,255},
     {10,18,38,255},{8,15,32,255},{18,32,64,255},
     {0,188,212,255},{220,235,245,255},{100,140,170,255},{40,60,90,255},
     {6,12,26,255},{30,70,110,255},{50,110,160,255},
     {3,8,18,255},{10,20,44,255},{14,26,52,255},{14,22,46,255},{10,18,40,255},{220,58,58,255}},
    {"Forest",
     {6,14,8,255},{4,10,6,255},{3,7,4,255},{8,18,10,255},
     {10,20,12,255},{8,16,10,255},{18,38,22,255},
     {56,200,80,255},{215,240,220,255},{100,150,110,255},{40,72,46,255},
     {6,14,8,255},{30,90,40,255},{50,140,60,255},
     {3,8,4,255},{10,22,12,255},{14,28,16,255},{14,26,16,255},{10,20,12,255},{220,58,58,255}},
    {"Crimson",
     {18,6,8,255},{13,4,6,255},{9,3,4,255},{22,8,10,255},
     {30,10,12,255},{24,8,10,255},{50,18,20,255},
     {230,56,72,255},{245,220,222,255},{160,100,105,255},{80,36,40,255},
     {14,4,6,255},{100,26,32,255},{150,40,48,255},
     {10,3,4,255},{24,10,12,255},{30,14,16,255},{28,10,12,255},{22,8,10,255},{220,58,58,255}},
    {"Amber",
     {18,12,4,255},{13,9,3,255},{9,6,2,255},{22,15,6,255},
     {28,18,8,255},{24,15,6,255},{46,30,12,255},
     {230,170,40,255},{248,238,210,255},{160,138,90,255},{80,62,28,255},
     {14,9,3,255},{100,70,20,255},{150,110,30,255},
     {10,6,2,255},{22,14,6,255},{28,18,8,255},{26,16,6,255},{20,13,5,255},{220,58,58,255}},
    {"Slate",
     {12,14,16,255},{8,10,12,255},{6,7,9,255},{16,18,21,255},
     {20,22,26,255},{17,19,22,255},{32,36,42,255},
     {100,160,220,255},{220,225,230,255},{120,128,138,255},{55,60,68,255},
     {10,12,14,255},{50,60,75,255},{80,95,115,255},
     {6,8,10,255},{18,20,24,255},{22,25,30,255},{22,24,28,255},{16,18,22,255},{220,58,58,255}},
    {"Rose",
     {18,6,14,255},{13,4,10,255},{9,3,7,255},{22,8,17,255},
     {30,10,24,255},{24,8,19,255},{50,18,40,255},
     {230,60,160,255},{245,220,238,255},{160,100,140,255},{80,36,66,255},
     {14,4,11,255},{100,26,80,255},{150,40,120,255},
     {10,3,8,255},{24,10,20,255},{30,14,24,255},{26,10,21,255},{22,8,18,255},{220,58,58,255}},
    {"Nord",
     {18,20,28,255},{14,16,22,255},{10,12,17,255},{22,24,32,255},
     {30,32,44,255},{26,28,38,255},{46,50,68,255},
     {136,192,208,255},{236,239,244,255},{130,140,160,255},{67,76,94,255},
     {16,18,26,255},{60,70,90,255},{90,105,130,255},
     {12,14,20,255},{26,28,40,255},{32,36,48,255},{30,32,46,255},{24,26,38,255},{220,58,58,255}},
    {"Galaxy",
     {3,2,12,255},{5,3,18,255},{2,1,8,255},{8,5,24,255},
     {10,6,28,255},{8,5,22,255},{20,12,52,255},
     {180,80,255,255},{235,220,255,255},{120,90,170,255},{55,38,88,255},
     {4,2,14,255},{60,20,120,255},{100,40,190,255},
     {2,1,10,255},{10,5,30,255},{14,8,40,255},{12,6,34,255},{8,4,26,255},{220,58,58,255}},
};

/* ══════════════════════════════════════════════════════════════════
 *  GLOBALS
 * ══════════════════════════════════════════════════════════════════ */
int   cur_theme       = 0;
float tdot_hov   [N_THEMES];
float tdot_bounce[N_THEMES];
float sel_ring_if     = 0.f;
float theme_pulse     = 0.f;
int   dots_in_tb      = 0;
int   dots_in_tb_prev = -1;
int   dots_x0         = 0;
int   dots_cy         = 0;
int   sr_x            = 380;
int   sr_w            = 380;
int   gal_paused      = 0;
float tc_t            = 1.f;
float galaxy_dot_f    = 0.f;
float galaxy_anim_t   = 0.f;

C4 C_BG,C_HDR,C_TITLE,C_TBAR,C_ROWA,C_ROWB,C_ROWH,
   C_ACC,C_TXT,C_SUB,C_DIM,
   C_SCRBG,C_SCRTH,C_SCRTO,
   C_SBAR,C_SRCH,C_SRCHA,C_SEP,C_BTNI,C_CLOSE;

static C4 tc_from[N_CSLOTS], tc_to[N_CSLOTS];

static void theme_pack(const Theme *t, C4 *s){
    s[0]=t->bg;   s[1]=t->hdr;  s[2]=t->title;s[3]=t->tbar;
    s[4]=t->rowa; s[5]=t->rowb; s[6]=t->rowh; s[7]=t->acc;
    s[8]=t->txt;  s[9]=t->sub;  s[10]=t->dim;
    s[11]=t->scrbg;s[12]=t->scrth;s[13]=t->scrto;
    s[14]=t->sbar;s[15]=t->srch;s[16]=t->srcha;s[17]=t->sep;
    s[18]=t->btni;s[19]=t->close;
}
static void theme_apply_slots(const C4 *s){
    C_BG=s[0];  C_HDR=s[1];  C_TITLE=s[2]; C_TBAR=s[3];
    C_ROWA=s[4];C_ROWB=s[5]; C_ROWH=s[6];  C_ACC=s[7];
    C_TXT=s[8]; C_SUB=s[9];  C_DIM=s[10];
    C_SCRBG=s[11];C_SCRTH=s[12];C_SCRTO=s[13];
    C_SBAR=s[14];C_SRCH=s[15];C_SRCHA=s[16];C_SEP=s[17];
    C_BTNI=s[18];C_CLOSE=s[19];
}

/* ══════════════════════════════════════════════════════════════════
 *  GALAXY DOT FX  —  orbital particles for the theme-picker dot
 * ══════════════════════════════════════════════════════════════════ */
#define N_GALAXY_ORBS 7
#define GORB_SPRING  22.f
#define GORB_DAMP    0.82f
#define GORB_REP_F  320.f
#define GORB_REP_R    5.8f

typedef struct {
    float angle,radius,speed,phase,size;
    float disp_x,disp_y,vel_x,vel_y;
} GalaxyOrb;

static GalaxyOrb g_orbs[N_GALAXY_ORBS];
static int       g_orbs_init = 0;

static void init_galaxy_orbs(void){
    g_orbs[0]=(GalaxyOrb){0.00f,0.52f,1.90f,0.00f,1.5f,0,0,0,0};
    g_orbs[1]=(GalaxyOrb){1.26f,0.88f,1.15f,1.40f,1.0f,0,0,0,0};
    g_orbs[2]=(GalaxyOrb){2.51f,0.65f,2.40f,2.62f,2.0f,0,0,0,0};
    g_orbs[3]=(GalaxyOrb){3.77f,1.00f,0.85f,0.78f,1.2f,0,0,0,0};
    g_orbs[4]=(GalaxyOrb){5.03f,0.40f,2.75f,3.14f,1.0f,0,0,0,0};
    g_orbs[5]=(GalaxyOrb){0.80f,0.72f,1.60f,1.95f,1.3f,0,0,0,0};
    g_orbs[6]=(GalaxyOrb){4.20f,0.45f,3.10f,4.50f,0.9f,0,0,0,0};
    g_orbs_init = 1;
}

/* ══════════════════════════════════════════════════════════════════
 *  GALAXY STAR FIELD
 *
 *  900 stars seeded along three logarithmic spiral arms plus a
 *  random background scatter field.  Position stored as (fx, fy)
 *  screen fractions + depth value.
 *
 *  Galaxy centre:  (GAL_CTR_X, GAL_CTR_Y) in normalised coords.
 *  Three arms at  0°, 120°, 240° with 2.8 rad of winding each.
 *
 *  Parallax: sx = fx*W + par_mx*depth*GAL_MAX_SHIFT*W
 *             sy = fy*H + par_my*depth*GAL_MAX_SHIFT*H
 *
 *  Depth tiers:
 *    far  [0.00, 0.28) — dim single pixels
 *    mid  [0.28, 0.62) — medium, optional soft cross
 *    near [0.62, 1.00] — bright, spikes + diffraction
 * ══════════════════════════════════════════════════════════════════ */
#define N_GAL_STARS     900
#define N_CLUSTER_STARS  80   /* dense core cluster */
#define GAL_MAX_SHIFT   0.072f
#define GAL_LERP_TAU    0.13f
#define GAL_CTR_X       0.40f
#define GAL_CTR_Y       0.47f

typedef struct {
    float fx, fy;
    float depth;
    float base_a;
    float freq, phase;
    Uint8 r, g, b;
    int   sz;   /* 0=dot  1=cross  2=brilliant+spikes */
    int   cluster; /* 1 = core cluster star */
} GalStar;

static GalStar gal_stars[N_GAL_STARS + N_CLUSTER_STARS];
static int     gal_stars_total  = 0;
static int     gal_stars_ready  = 0;
static float   par_mx = 0.f, par_my = 0.f;

/* ══════════════════════════════════════════════════════════════════
 *  COSMIC DUST WISPS  —  thin arc segments along spiral arms
 * ══════════════════════════════════════════════════════════════════ */
#define N_WISPS        120
#define WISP_SEGS        6   /* line segments per wisp */

typedef struct {
    float fx, fy;      /* root normalised pos */
    float angle;       /* wisp direction (rad) */
    float length;      /* arc length in normalised units */
    float depth;
    float phase;
    float speed;
    Uint8 r, g, b;
} DustWisp;

static DustWisp dust_wisps[N_WISPS];
static int      dust_ready = 0;

/* ══════════════════════════════════════════════════════════════════
 *  tick_theme
 * ══════════════════════════════════════════════════════════════════ */
void tick_theme(float dt){
    if(tc_t < 1.f){
        tc_t += dt * TC_SPEED;
        if(tc_t > 1.f) tc_t = 1.f;
        C4 cur[N_CSLOTS];
        for(int i = 0; i < N_CSLOTS; i++) cur[i] = lerpc(tc_from[i], tc_to[i], tc_t);
        theme_apply_slots(cur);
    }

    if(!gal_paused){
        galaxy_anim_t += dt;
        if(galaxy_anim_t > 628.f) galaxy_anim_t -= 628.f;

        /* Smooth mouse parallax */
        if(win_w > 0 && win_h > 0){
            int mx_r, my_r;
            SDL_GetMouseState(&mx_r, &my_r);
            float tx    = ((float)mx_r / (float)win_w) - 0.5f;
            float ty    = ((float)my_r / (float)win_h) - 0.5f;
            float alpha = clampf(dt / GAL_LERP_TAU, 0.f, 1.f);
            par_mx += (tx - par_mx) * alpha;
            par_my += (ty - par_my) * alpha;
        }

        /* Orbital particle physics for the dot FX */
        if(!g_orbs_init) init_galaxy_orbs();
        {
            float damp_f = powf(GORB_DAMP, dt);
            for(int i = 0; i < N_GALAXY_ORBS; i++){
                g_orbs[i].angle += g_orbs[i].speed * dt;
                if(g_orbs[i].angle > (float)(2.0*M_PI))
                    g_orbs[i].angle -= (float)(2.0*M_PI);
                g_orbs[i].vel_x += -GORB_SPRING * g_orbs[i].disp_x * dt;
                g_orbs[i].vel_y += -GORB_SPRING * g_orbs[i].disp_y * dt;
                g_orbs[i].vel_x *= damp_f;
                g_orbs[i].vel_y *= damp_f;
                g_orbs[i].disp_x += g_orbs[i].vel_x * dt;
                g_orbs[i].disp_y += g_orbs[i].vel_y * dt;
            }
        }
    } else {
        if(!g_orbs_init) init_galaxy_orbs();
    }
}

/* ══════════════════════════════════════════════════════════════════ */
void set_theme(int idx){
    cur_theme=idx;
    tc_from[0]=C_BG;    tc_from[1]=C_HDR;   tc_from[2]=C_TITLE;
    tc_from[3]=C_TBAR;  tc_from[4]=C_ROWA;  tc_from[5]=C_ROWB;
    tc_from[6]=C_ROWH;  tc_from[7]=C_ACC;   tc_from[8]=C_TXT;
    tc_from[9]=C_SUB;   tc_from[10]=C_DIM;  tc_from[11]=C_SCRBG;
    tc_from[12]=C_SCRTH;tc_from[13]=C_SCRTO;tc_from[14]=C_SBAR;
    tc_from[15]=C_SRCH; tc_from[16]=C_SRCHA;tc_from[17]=C_SEP;
    tc_from[18]=C_BTNI; tc_from[19]=C_CLOSE;
    theme_pack(&THEMES[idx], tc_to);
    tc_t=0.f; theme_pulse=1.f;
}
void set_theme_instant(int idx){
    cur_theme=idx;
    theme_pack(&THEMES[idx], tc_to);
    for(int i=0;i<N_CSLOTS;i++) tc_from[i]=tc_to[i];
    tc_t=1.f;
    theme_apply_slots(tc_to);
    sel_ring_if=(float)idx;
}
void compute_dot_layout(void){
    dots_in_tb=0; dots_x0=win_w/2; dots_cy=HDR_Y+HDR_H/2;
    sr_w=(win_w-280)>420?420:(win_w-280<80?80:win_w-280);
    sr_x=280+((win_w-16-280)-sr_w)/2;
}
int hit_theme_dot(int mx,int my){(void)mx;(void)my;return -1;}

/* ══════════════════════════════════════════════════════════════════
 *  draw_galaxy_dot_fx
 * ══════════════════════════════════════════════════════════════════ */
static SDL_Texture *blob_tex = NULL;

void draw_galaxy_dot_fx(int cx, int cy, int r,
                        float gf, float hf, float pa, float dt){
    if(gf <= 0.08f) return;
    if(!g_orbs_init) init_galaxy_orbs();

    float t=galaxy_anim_t, rf=(float)r;
    int mx_raw,my_raw;
    SDL_GetMouseState(&mx_raw,&my_raw);
    float mxf=(float)mx_raw, myf=(float)my_raw;

    /* Aurora hazes */
    if(blob_tex){
        float breath=1.f+0.20f*sinf(t*1.05f);
        int   hr=(int)((rf+3.f)*(2.0f+gf*0.45f)*breath+0.5f);
        float a0=gf*(38.f+14.f*sinf(t*0.85f+1.3f))*pa;
        SDL_SetTextureColorMod(blob_tex,155,40,230);
        SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(a0,0.f,255.f));
        {SDL_Rect d={cx-hr,cy-hr,hr*2,hr*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}
        int   hr2=(int)(hr*0.65f);
        float a1=gf*(58.f+22.f*sinf(t*1.30f+0.6f))*pa;
        SDL_SetTextureColorMod(blob_tex,210,75,255);
        SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(a1,0.f,255.f));
        {SDL_Rect d={cx-hr2,cy-hr2,hr2*2,hr2*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}
        if(hf>0.02f){
            float hp=hf*(0.85f+0.15f*sinf(t*3.1f));
            int br=(int)(rf*(1.20f+hp*0.30f)+0.5f);
            float ha=hp*72.f*pa;
            SDL_SetTextureColorMod(blob_tex,255,170,255);
            SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(ha,0.f,255.f));
            {SDL_Rect d={cx-br-2,cy-br-2,(br+2)*2,(br+2)*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}
        }
    }
    /* Inner corona */
    {
        float pulse=1.f+0.10f*sinf(t*2.55f+0.7f);
        int cr=(int)((rf+1.f)*pulse+0.5f);
        float a0=gf*(105.f+46.f*sinf(t*2.55f+0.7f)+hf*55.f)*pa;
        C4 corona={232,118,255,(Uint8)clampf(a0,0.f,255.f)};
        frr_aa(cx-cr-1,cy-cr-1,(cr+1)*2,(cr+1)*2,cr+1,corona);
    }
    /* Diffraction spikes */
    {
        float rot=t*0.13f, base_len=rf+1.f+gf*4.f;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        for(int s=0;s<4;s++){
            float ang=rot+(float)s*(float)(M_PI*0.5);
            float lpulse=0.76f+0.24f*sinf(t*1.85f+(float)s*1.28f);
            float tip=base_len*lpulse;
            float ca=cosf(ang),sa=sinf(ang);
            for(int seg=0;seg<3;seg++){
                float f0=(float)seg/3.f, f1=(float)(seg+1)/3.f;
                float fade=(1.f-f1)*(1.f-f1);
                int x0=cx+(int)(ca*(rf+(tip-rf)*f0)+0.5f);
                int y0=cy+(int)(sa*(rf+(tip-rf)*f0)+0.5f);
                int x1=cx+(int)(ca*(rf+(tip-rf)*f1)+0.5f);
                int y1=cy+(int)(sa*(rf+(tip-rf)*f1)+0.5f);
                Uint8 asp=(Uint8)clampf(gf*fade*245.f*pa,0.f,255.f);
                if(asp>2){SDL_SetRenderDrawColor(ren,255,218,255,asp);SDL_RenderDrawLine(ren,x0,y0,x1,y1);}
            }
        }
    }
    /* Orbital particles */
    for(int i=0;i<N_GALAXY_ORBS;i++){
        float orb_r=rf*(1.55f+g_orbs[i].radius);
        float ang=g_orbs[i].angle;
        float orb_x=(float)cx+cosf(ang)*orb_r;
        float orb_y=(float)cy+sinf(ang)*orb_r;
        float wx=orb_x+g_orbs[i].disp_x, wy=orb_y+g_orbs[i].disp_y;
        float dx_m=wx-mxf, dy_m=wy-myf;
        float dist2=dx_m*dx_m+dy_m*dy_m;
        float rep_r=rf*GORB_REP_R;
        if(dist2<rep_r*rep_r&&dist2>0.01f){
            float dist=sqrtf(dist2);
            float t_rep=1.f-dist/rep_r;
            float force=t_rep*t_rep*GORB_REP_F;
            g_orbs[i].vel_x+=(dx_m/dist)*force*dt;
            g_orbs[i].vel_y+=(dy_m/dist)*force*dt;
        }
        float pulse=0.55f+0.45f*sinf(t*g_orbs[i].speed*1.35f+g_orbs[i].phase);
        int orb_sz=(int)(g_orbs[i].size*(0.80f+0.40f*pulse)+0.5f);
        if(orb_sz<1)orb_sz=1;
        float base_a=gf*pa*(0.50f+0.50f*pulse)*225.f;
        Uint8 cr2,cg2,cb2;
        switch(i%4){
            case 0:cr2=195;cg2= 75;cb2=255;break;
            case 1:cr2=255;cg2=155;cb2=255;break;
            case 2:cr2=185;cg2=215;cb2=255;break;
            default:cr2=255;cg2=120;cb2=200;break;
        }
        for(int tr=2;tr>=1;tr--){
            float trl_ang=ang-g_orbs[i].speed*(float)tr*0.065f;
            float tbx=(float)cx+cosf(trl_ang)*orb_r+g_orbs[i].disp_x*(1.f-(float)tr*0.40f);
            float tby=(float)cy+sinf(trl_ang)*orb_r+g_orbs[i].disp_y*(1.f-(float)tr*0.40f);
            float t_a=base_a*(0.28f/(float)tr);
            int tsz=orb_sz>1?orb_sz-1:1;
            C4 trail={cr2,cg2,cb2,(Uint8)clampf(t_a,0.f,255.f)};
            frr_aa((int)(tbx+0.5f)-tsz,(int)(tby+0.5f)-tsz,tsz*2,tsz*2,tsz,trail);
        }
        C4 oc={cr2,cg2,cb2,(Uint8)clampf(base_a,0.f,255.f)};
        frr_aa((int)(wx+0.5f)-orb_sz,(int)(wy+0.5f)-orb_sz,orb_sz*2,orb_sz*2,orb_sz,oc);
        if(pulse>0.88f){
            float sp_t=(pulse-0.88f)/0.12f;
            C4 sp={255,242,255,(Uint8)clampf(sp_t*base_a*0.55f,0.f,255.f)};
            frr_aa((int)(wx+0.5f)-1,(int)(wy+0.5f)-1,2,2,1,sp);
        }
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* ══════════════════════════════════════════════════════════════════
 *  BACKGROUND TEXTURES  &  STAR INIT
 * ══════════════════════════════════════════════════════════════════ */
static SDL_Texture *grain_tex = NULL;

static void draw_blob(int cx,int cy,int r,float opacity){
    if(!blob_tex)return;
    C4 ac=C_ACC;
    SDL_SetTextureColorMod(blob_tex,ac.r,ac.g,ac.b);
    SDL_SetTextureAlphaMod(blob_tex,(Uint8)(opacity*255.f));
    SDL_Rect dst={cx-r,cy-r,r*2,r*2};
    SDL_RenderCopy(ren,blob_tex,NULL,&dst);
}

void bg_init(void){
    /* ── Grain texture ─────────────────────────────────────────── */
    grain_tex=SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STATIC,GRAIN_SZ,GRAIN_SZ);
    if(grain_tex){
        Uint32 *px=(Uint32*)malloc(GRAIN_SZ*GRAIN_SZ*sizeof(Uint32));
        if(px){
            Uint32 rng=0xDEADBEEF;
            for(int i=0;i<GRAIN_SZ*GRAIN_SZ;i++){
                rng=rng*1664525u+1013904223u;
                Uint8 v=(rng>>24)&0xFF;
                px[i]=((Uint32)255<<24)|((Uint32)255<<16)|((Uint32)255<<8)|(Uint8)(v*0.07f);
            }
            SDL_UpdateTexture(grain_tex,NULL,px,GRAIN_SZ*sizeof(Uint32));
            free(px);
        }
        SDL_SetTextureBlendMode(grain_tex,SDL_BLENDMODE_BLEND);
    }

    /* ── Blob: radial Gaussian ─────────────────────────────────── */
    blob_tex=SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_STATIC,BLOB_TEX_SZ,BLOB_TEX_SZ);
    if(blob_tex){
        Uint32 *px=(Uint32*)malloc(BLOB_TEX_SZ*BLOB_TEX_SZ*sizeof(Uint32));
        if(px){
            float rf=(float)BLOB_TEX_R;
            for(int y=0;y<BLOB_TEX_SZ;y++) for(int x=0;x<BLOB_TEX_SZ;x++){
                float dx=(float)(x-BLOB_TEX_R),dy=(float)(y-BLOB_TEX_R);
                float d=sqrtf(dx*dx+dy*dy)/rf;
                if(d>=1.f){px[y*BLOB_TEX_SZ+x]=0;continue;}
                px[y*BLOB_TEX_SZ+x]=0xFFFFFF00|(Uint8)(expf(-2.5f*d*d)*255.f);
            }
            SDL_UpdateTexture(blob_tex,NULL,px,BLOB_TEX_SZ*sizeof(Uint32));
            free(px);
        }
        SDL_SetTextureBlendMode(blob_tex,SDL_BLENDMODE_BLEND);
    }

    /* ── Galaxy star array ────────────────────────────────────────
     *
     *  Three logarithmic spiral arms at 0°, 120°, 240°.
     *  Each arm contributes ~30% of stars; the remaining ~10%
     *  is pure background scatter for depth and realism.
     *  A small inter-arm population gives bridge structure.
     *
     *  Spiral equation (polar, normalised-screen space):
     *    r  = r_min * exp( b * theta )
     *    ... but we use a simple linear winding for visual fidelity:
     *    r  = r_min + (r_max - r_min) * t
     *    a  = arm_base + t * WIND + scatter
     *  where t ∈ [0,1] and WIND = 2.6 rad (~150°).
     * ─────────────────────────────────────────────────────────────── */
    {
#define RNG1(r)  ((r)=(r)*1664525u+1013904223u)
#define RNDF(r)  (RNG1(r),(float)((r)>>16)/65535.f)
#define RNGN(r)  (RNDF(r)*2.f-1.f)   /* uniform [-1, 1] */

        Uint32 rng=0xF00DCAFE;

        /* Arm winding and radius range (in normalised screen space,
           aspect-corrected at draw time via win_w/win_h).            */
        const float ARM_BASE[3] = {0.f,
                                   (float)(2.0*M_PI/3.0),
                                   (float)(4.0*M_PI/3.0)};
        const float WIND        = 2.6f;   /* radians of spiral winding  */
        const float R_MIN       = 0.04f;  /* inner radius (normalised)  */
        const float R_MAX       = 0.50f;  /* outer radius               */
        const float ARM_W       = 0.055f; /* Gaussian scatter per arm   */

        int n = N_GAL_STARS;
        for(int i=0;i<n;i++){
            GalStar *s=&gal_stars[i];
            s->cluster=0;

            float roll=RNDF(rng);
            int   on_arm  = (roll < 0.80f);   /* 80% on an arm             */
            int   arm_idx = (int)(RNDF(rng)*3.f) % 3;

            if(on_arm){
                /* t biased toward middle radii (more interesting visually) */
                float t=RNDF(rng); t=t*t*(3.f-2.f*t); /* smoothstep         */
                t = 0.02f + t * 0.98f;
                float r    = R_MIN + (R_MAX - R_MIN) * t;
                float base = ARM_BASE[arm_idx];
                float ang  = base + WIND * t;

                /* Gaussian scatter perpendicular to arm spine */
                float scatter_r = RNGN(rng) * ARM_W * (0.3f + t);
                float scatter_a = RNDF(rng) * (float)(2.0*M_PI);
                float ex = cosf(scatter_a)*scatter_r;
                float ey = sinf(scatter_a)*scatter_r;

                /* Aspect correction: x is narrower in typical windows */
                float cx_ = GAL_CTR_X + cosf(ang)*r*0.72f + ex;
                float cy_ = GAL_CTR_Y + sinf(ang)*r       + ey;
                s->fx = cx_; s->fy = cy_;
            } else {
                /* Background scatter with slight core bias */
                if(RNDF(rng)<0.30f){
                    s->fx=GAL_CTR_X-0.22f+RNDF(rng)*0.44f;
                    s->fy=GAL_CTR_Y-0.28f+RNDF(rng)*0.56f;
                } else {
                    s->fx=RNDF(rng); s->fy=RNDF(rng);
                }
            }

            /* Depth tier: 35% far, 38% mid, 27% near */
            float tier=RNDF(rng);
            if     (tier<0.35f) s->depth=RNDF(rng)*0.28f;
            else if(tier<0.73f) s->depth=0.28f+RNDF(rng)*0.34f;
            else                s->depth=0.62f+RNDF(rng)*0.38f;

            /* Brightness by depth */
            float blo,bhi;
            if     (s->depth<0.28f){blo=0.10f;bhi=0.38f;}
            else if(s->depth<0.62f){blo=0.24f;bhi=0.62f;}
            else                   {blo=0.48f;bhi=1.00f;}
            s->base_a=blo+RNDF(rng)*(bhi-blo);

            /* Twinkle */
            s->freq =0.12f+s->depth*0.48f+RNDF(rng)*(0.60f+s->depth*2.20f);
            s->phase=RNDF(rng)*(float)(2.0*M_PI);

            /* Colour temperature — richer palette vs original */
            float tc2=RNDF(rng);
            if     (tc2<0.32f){s->r=162;s->g=198;s->b=255;}  /* blue-white    */
            else if(tc2<0.48f){s->r=220;s->g=235;s->b=255;}  /* ice white     */
            else if(tc2<0.60f){s->r=255;s->g=255;s->b=255;}  /* pure white    */
            else if(tc2<0.72f){s->r=200;s->g=130;s->b=255;}  /* electric vio  */
            else if(tc2<0.82f){s->r=255;s->g=140;s->b=220;}  /* hot pink      */
            else if(tc2<0.91f){s->r=255;s->g=185;s->b= 80;}  /* warm amber    */
            else              {s->r=140;s->g=200;s->b=255;}   /* aqua blue     */

            /* Size class */
            float sc=RNDF(rng);
            if     (s->depth<0.28f) s->sz=0;
            else if(s->depth<0.62f) s->sz=(sc<0.68f)?0:1;
            else                    s->sz=(sc<0.22f)?0:(sc<0.55f)?1:2;
        }

        /* ── Dense core cluster stars ─────────────────────────────── */
        for(int i=0;i<N_CLUSTER_STARS;i++){
            GalStar *s=&gal_stars[n+i];
            s->cluster=1;

            /* Tight Gaussian around galactic centre */
            float angle=RNDF(rng)*(float)(2.0*M_PI);
            float r    =RNDF(rng)*0.06f;  /* very tight radius */
            s->fx=GAL_CTR_X+cosf(angle)*r*0.70f;
            s->fy=GAL_CTR_Y+sinf(angle)*r;

            s->depth  =0.55f+RNDF(rng)*0.45f;
            s->base_a =0.65f+RNDF(rng)*0.35f;
            s->freq   =0.40f+RNDF(rng)*3.20f;
            s->phase  =RNDF(rng)*(float)(2.0*M_PI);

            /* Cluster stars: mostly blue-white / hot white */
            float tc3=RNDF(rng);
            if     (tc3<0.40f){s->r=185;s->g=215;s->b=255;}
            else if(tc3<0.65f){s->r=245;s->g=248;s->b=255;}
            else if(tc3<0.80f){s->r=210;s->g=150;s->b=255;}
            else              {s->r=255;s->g=160;s->b=210;}

            float sc=RNDF(rng);
            s->sz=(sc<0.35f)?1:2;
        }
        gal_stars_total = n + N_CLUSTER_STARS;
        gal_stars_ready = 1;

        /* ── Cosmic dust wisps ─────────────────────────────────────── */
        for(int i=0;i<N_WISPS;i++){
            DustWisp *w=&dust_wisps[i];

            int   arm_idx2= (int)(RNDF(rng)*3.f) % 3;
            float t2 = 0.10f + RNDF(rng)*0.88f;
            float r2 = R_MIN + (R_MAX - R_MIN) * t2;
            float ang2= ARM_BASE[arm_idx2] + WIND * t2 + RNGN(rng)*0.20f;

            w->fx   = GAL_CTR_X + cosf(ang2)*r2*0.72f;
            w->fy   = GAL_CTR_Y + sinf(ang2)*r2;
            w->angle= ang2 + (float)(M_PI*0.5) + RNGN(rng)*0.40f;
            w->length = 0.012f + RNDF(rng)*0.045f;
            w->depth  = 0.15f + RNDF(rng)*0.50f;
            w->phase  = RNDF(rng)*(float)(2.0*M_PI);
            w->speed  = 0.08f + RNDF(rng)*0.22f;

            /* Dust palette: purple, blue, magenta wisps */
            float wc=RNDF(rng);
            if     (wc<0.30f){w->r= 90;w->g= 40;w->b=180;}
            else if(wc<0.55f){w->r= 40;w->g= 60;w->b=200;}
            else if(wc<0.75f){w->r=180;w->g= 50;w->b=160;}
            else              {w->r=100;w->g= 80;w->b=220;}
        }
        dust_ready=1;

#undef RNG1
#undef RNDF
#undef RNGN
    }
}

void bg_free(void){
    if(grain_tex){SDL_DestroyTexture(grain_tex);grain_tex=NULL;}
    if(blob_tex) {SDL_DestroyTexture(blob_tex); blob_tex =NULL;}
}

/* ══════════════════════════════════════════════════════════════════
 *  draw_background
 * ══════════════════════════════════════════════════════════════════ */
void draw_background(void){
    sc_(C_BG);
    SDL_RenderClear(ren);

    if(cur_theme==8){
        /* ════════════════════════════════════════════════════════════
         *  GALAXY — six passes
         * ════════════════════════════════════════════════════════════ */
        float t=galaxy_anim_t;
        float sw=(float)win_w, sh=(float)win_h;
        int base=(win_w<win_h?win_w:win_h);
        float ds=(float)base;

        /* Galaxy core screen position (slight drift for living feel) */
        float cx_n = GAL_CTR_X + 0.018f*sinf(t*0.042f);
        float cy_n = GAL_CTR_Y + 0.010f*cosf(t*0.033f);
        int   gcx  = (int)(cx_n*sw+0.5f);
        int   gcy  = (int)(cy_n*sh+0.5f);

        /* ── Pass 1: Star field ─────────────────────────────────────
         *  Three sub-passes (far → near) so nearer stars render on
         *  top.  Cluster stars always render in the near pass.
         * ─────────────────────────────────────────────────────────── */
        if(gal_stars_ready){
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);

            for(int pass=0;pass<3;pass++){
                for(int i=0;i<gal_stars_total;i++){
                    GalStar *s=&gal_stars[i];
                    /* Cluster stars forced into pass 2 */
                    int eff_pass = s->cluster ? 2 : s->sz;
                    if(eff_pass!=pass) continue;

                    float twk=0.50f+0.50f*sinf(t*s->freq+s->phase);
                    float a_f=s->base_a*twk;
                    if(a_f<0.04f) continue;
                    Uint8 a=(Uint8)(a_f*255.f);
                    if(a<4) continue;

                    /* Parallax */
                    float shift=s->depth*GAL_MAX_SHIFT;
                    int sx=(int)(s->fx*sw+par_mx*shift*sw+0.5f);
                    int sy=(int)(s->fy*sh+par_my*shift*sh+0.5f);
                    if(sx<-4||sx>=win_w+4||sy<-4||sy>=win_h+4) continue;

                    /* Centre dot */
                    SDL_SetRenderDrawColor(ren,s->r,s->g,s->b,a);
                    SDL_RenderDrawPoint(ren,sx,sy);

                    /* sz≥1 or cluster: 5-point soft cross */
                    if(s->sz>=1||s->cluster){
                        Uint8 a2=(Uint8)((int)a*36/100);
                        SDL_SetRenderDrawColor(ren,s->r,s->g,s->b,a2);
                        SDL_RenderDrawPoint(ren,sx-1,sy);
                        SDL_RenderDrawPoint(ren,sx+1,sy);
                        SDL_RenderDrawPoint(ren,sx,sy-1);
                        SDL_RenderDrawPoint(ren,sx,sy+1);
                    }

                    /* sz==2 or cluster: wider cross + rotating diffraction */
                    if(s->sz>=2||s->cluster){
                        Uint8 a3=(Uint8)((int)a*16/100);
                        SDL_SetRenderDrawColor(ren,s->r,s->g,s->b,a3);
                        SDL_RenderDrawPoint(ren,sx-2,sy);
                        SDL_RenderDrawPoint(ren,sx+2,sy);
                        SDL_RenderDrawPoint(ren,sx,sy-2);
                        SDL_RenderDrawPoint(ren,sx,sy+2);

                        if(twk>0.28f){
                            float rot=t*0.038f+s->phase*0.5f;
                            float slen=3.0f+4.5f*twk+(s->cluster?1.5f:0.f);
                            for(int arm=0;arm<4;arm++){
                                float ang=rot+(float)arm*(float)(M_PI*0.5);
                                float lp=0.68f+0.32f*sinf(t*1.45f+(float)arm*1.28f+s->phase);
                                float tip=slen*lp;
                                float ca=cosf(ang),sa=sinf(ang);
                                for(int seg=0;seg<4;seg++){
                                    float f0=(float)seg/4.f,f1=(float)(seg+1)/4.f;
                                    float fade=(1.f-f1)*(1.f-f1);
                                    int x0=sx+(int)(ca*(1.f+(tip-1.f)*f0)+0.5f);
                                    int y0=sy+(int)(sa*(1.f+(tip-1.f)*f0)+0.5f);
                                    int x1=sx+(int)(ca*(1.f+(tip-1.f)*f1)+0.5f);
                                    int y1=sy+(int)(sa*(1.f+(tip-1.f)*f1)+0.5f);
                                    Uint8 asp=(Uint8)clampf(fade*twk*(float)a*0.92f,0.f,255.f);
                                    if(asp>2){
                                        SDL_SetRenderDrawColor(ren,s->r,s->g,s->b,asp);
                                        SDL_RenderDrawLine(ren,x0,y0,x1,y1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }

        /* ── Pass 2: Cosmic dust wisps ──────────────────────────────
         *  Thin additive line arcs tracing the spiral arms.
         *  Each wisp is a short polyline drawn in segments with
         *  sinusoidal alpha breathing.
         * ─────────────────────────────────────────────────────────── */
        if(dust_ready){
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_ADD);
            for(int i=0;i<N_WISPS;i++){
                DustWisp *w=&dust_wisps[i];
                float breath=0.35f+0.65f*fabsf(sinf(t*w->speed+w->phase));
                float shift=w->depth*GAL_MAX_SHIFT;
                float px0=w->fx*sw+par_mx*shift*sw;
                float py0=w->fy*sh+par_my*shift*sh;

                /* Slight positional drift */
                px0 += 0.008f*sw*sinf(t*w->speed*0.5f+w->phase+1.3f);
                py0 += 0.006f*sh*cosf(t*w->speed*0.5f+w->phase+0.7f);

                float seg_len = w->length * ds / (float)WISP_SEGS;
                Uint8 base_al = (Uint8)clampf(breath*22.f,0.f,255.f);
                if(base_al<3) continue;

                SDL_SetRenderDrawColor(ren,w->r,w->g,w->b,base_al);
                float cur_a=w->angle+t*w->speed*0.04f;
                for(int s2=0;s2<WISP_SEGS;s2++){
                    float fade=(1.f-(float)s2/(float)WISP_SEGS);
                    Uint8 al=(Uint8)clampf(base_al*fade*fade,0.f,255.f);
                    if(al<2) continue;
                    /* Gentle arc: angle nudges slightly each segment */
                    cur_a += 0.08f;
                    int x0=(int)(px0+0.5f);
                    int y0=(int)(py0+0.5f);
                    px0+=cosf(cur_a)*seg_len;
                    py0+=sinf(cur_a)*seg_len;
                    int x1=(int)(px0+0.5f);
                    int y1=(int)(py0+0.5f);
                    SDL_SetRenderDrawColor(ren,w->r,w->g,w->b,al);
                    SDL_RenderDrawLine(ren,x0,y0,x1,y1);
                }
            }
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }

        /* ── Pass 3: Nebula clouds ──────────────────────────────────
         *  10 large additive Gaussian blobs.  Three colour groups:
         *  deep indigo/cobalt (back layer), electric purple (mid),
         *  hot magenta/pink (front).  Each has independent drift +
         *  breathing so the nebulae are never static.
         * ─────────────────────────────────────────────────────────── */
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_ADD);
        if(blob_tex){

            /* ── Deep-field indigo halos (outermost, largest) ─────── */
            {float p=0.66f+0.34f*sinf(t*0.14f+0.3f);
             int r=(int)(ds*0.84f*p);
             int nx=win_w-r+(int)(ds*0.06f*sinf(t*0.055f+0.4f));
             int ny=-r/2   +(int)(ds*0.04f*cosf(t*0.070f));
             SDL_SetTextureColorMod(blob_tex, 35, 18,155);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(50.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            {float p=0.68f+0.32f*sinf(t*0.16f+2.8f);
             int r=(int)(ds*0.78f*p);
             int nx=-r/3   +(int)(ds*0.05f*cosf(t*0.062f+1.9f));
             int ny=win_h-r+(int)(ds*0.04f*sinf(t*0.075f+0.7f));
             SDL_SetTextureColorMod(blob_tex, 22, 28,175);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(44.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* ── Electric purple mid-layer ────────────────────────── */
            {float p=0.70f+0.30f*sinf(t*0.19f+1.2f);
             int r=(int)(ds*0.65f*p);
             int nx=win_w-r+(int)(ds*0.05f*sinf(t*0.072f+0.3f));
             int ny=-r/3   +(int)(ds*0.04f*cosf(t*0.088f));
             SDL_SetTextureColorMod(blob_tex,105, 18,210);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(62.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            {float p=0.72f+0.28f*sinf(t*0.17f+3.8f);
             int r=(int)(ds*0.58f*p);
             int nx=-r/2   +(int)(ds*0.052f*cosf(t*0.082f+1.8f));
             int ny=win_h-r+(int)(ds*0.042f*sinf(t*0.098f+0.6f));
             SDL_SetTextureColorMod(blob_tex,145, 22,195);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(58.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            {float p=0.62f+0.38f*sinf(t*0.22f+5.1f);
             int r=(int)(ds*0.46f*p);
             int nx=(int)(win_w*0.12f)-r+(int)(ds*0.048f*sinf(t*0.110f+2.5f));
             int ny=(int)(win_h*0.55f)-r+(int)(ds*0.036f*cosf(t*0.130f+0.9f));
             SDL_SetTextureColorMod(blob_tex, 30, 50,205);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(48.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* ── Hot magenta / pink nebulae (foreground) ──────────── */
            {float p=0.74f+0.26f*sinf(t*0.26f+1.7f);
             int r=(int)(ds*0.52f*p);
             int nx=win_w/2-r+(int)(ds*0.050f*cosf(t*0.092f+3.2f));
             int ny=-r/2     +(int)(ds*0.038f*sinf(t*0.138f+1.5f));
             SDL_SetTextureColorMod(blob_tex,200, 30,165);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(40.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            {float p=0.65f+0.35f*sinf(t*0.20f+4.6f);
             int r=(int)(ds*0.44f*p);
             int nx=(int)(win_w*0.72f)-r+(int)(ds*0.046f*sinf(t*0.128f+0.7f));
             int ny=(int)(win_h*0.36f)-r+(int)(ds*0.034f*cosf(t*0.155f+3.8f));
             SDL_SetTextureColorMod(blob_tex,170, 38,215);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(38.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            {float p=0.58f+0.42f*sinf(t*0.15f+0.9f);
             int r=(int)(ds*0.34f*p);
             int nx=win_w/2-r+(int)(ds*0.038f*cosf(t*0.118f+4.2f));
             int ny=win_h/2-r+(int)(ds*0.028f*sinf(t*0.175f+2.9f));
             SDL_SetTextureColorMod(blob_tex,225, 65,155);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(26.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* ── Aqua/teal accent nebula ───────────────────────────── */
            {float p=0.60f+0.40f*sinf(t*0.24f+6.2f);
             int r=(int)(ds*0.30f*p);
             int nx=(int)(win_w*0.82f)-r+(int)(ds*0.042f*sinf(t*0.105f+1.1f));
             int ny=(int)(win_h*0.68f)-r+(int)(ds*0.030f*cosf(t*0.145f+5.5f));
             SDL_SetTextureColorMod(blob_tex, 20,140,210);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(28.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* ── Distant rose-violet cloud ────────────────────────── */
            {float p=0.55f+0.45f*sinf(t*0.11f+3.3f);
             int r=(int)(ds*0.26f*p);
             int nx=(int)(win_w*0.18f)-r+(int)(ds*0.038f*cosf(t*0.095f+2.0f));
             int ny=(int)(win_h*0.25f)-r+(int)(ds*0.028f*sinf(t*0.135f+4.4f));
             SDL_SetTextureColorMod(blob_tex,178, 55,210);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(22.f*p,0,255));
             SDL_Rect d={nx,ny,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}
        }

        /* ── Pass 4: Galactic core ──────────────────────────────────
         *  Seven concentric additive layers from wide violet outer
         *  glow to a razor-sharp blue-white nucleus.
         * ─────────────────────────────────────────────────────────── */
        if(blob_tex){

            /* 1. Outermost warm violet halo */
            {float p=0.86f+0.14f*sinf(t*0.28f);
             int r=(int)(ds*0.78f*p);
             SDL_SetTextureColorMod(blob_tex, 80, 28,155);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(18.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 2. Mid-radius indigo glow */
            {float p=0.82f+0.18f*sinf(t*0.35f+0.8f);
             int r=(int)(ds*0.54f*p);
             SDL_SetTextureColorMod(blob_tex,110, 38,200);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(28.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 3. Bright violet ring */
            {float p=0.84f+0.16f*sinf(t*0.44f+2.1f);
             int r=(int)(ds*0.36f*p);
             SDL_SetTextureColorMod(blob_tex,160, 60,240);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(40.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 4. Warm amber/white transition */
            {float p=0.80f+0.20f*sinf(t*0.58f+1.3f);
             int r=(int)(ds*0.18f*p);
             SDL_SetTextureColorMod(blob_tex,240,160, 80);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(52.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 5. White-gold inner core */
            {float p=0.88f+0.12f*sinf(t*0.80f+0.4f);
             int r=(int)(ds*0.085f*p);
             SDL_SetTextureColorMod(blob_tex,255,220,160);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(72.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 6. Blue-white hot inner nucleus */
            {float p=0.84f+0.16f*sinf(t*1.25f+1.9f);
             int r=(int)(ds*0.036f*p);
             SDL_SetTextureColorMod(blob_tex,185,210,255);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(82.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}

            /* 7. Pure white pinpoint nucleus */
            {float p=0.92f+0.08f*sinf(t*2.20f+0.6f);
             int r=(int)(ds*0.014f*p);
             SDL_SetTextureColorMod(blob_tex,255,255,255);
             SDL_SetTextureAlphaMod(blob_tex,(Uint8)clampf(90.f*p,0,255));
             SDL_Rect d={gcx-r,gcy-r,r*2,r*2};SDL_RenderCopy(ren,blob_tex,NULL,&d);}
        }

        /* ── Core nucleus diffraction spikes ───────────────────────
         *  Four long bright spikes radiating from the nucleus,
         *  slowly counter-rotating.
         * ─────────────────────────────────────────────────────────── */
        {
            float rot   = -t * 0.018f;   /* slow counter-rotation   */
            float slen  = ds * 0.18f;    /* spike half-length       */
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_ADD);
            for(int arm=0;arm<4;arm++){
                float ang=rot+(float)arm*(float)(M_PI*0.5);
                float lpulse=0.72f+0.28f*sinf(t*0.65f+(float)arm*1.18f);
                float tip=slen*lpulse;
                float ca=cosf(ang),sa=sinf(ang);
                int NSEGS=8;
                for(int seg=0;seg<NSEGS;seg++){
                    float f0=(float)seg/(float)NSEGS;
                    float f1=(float)(seg+1)/(float)NSEGS;
                    float fade=(1.f-f1)*(1.f-f1)*(1.f-f1);
                    int x0=gcx+(int)(ca*tip*f0+0.5f);
                    int y0=gcy+(int)(sa*tip*f0+0.5f);
                    int x1=gcx+(int)(ca*tip*f1+0.5f);
                    int y1=gcy+(int)(sa*tip*f1+0.5f);
                    Uint8 asp=(Uint8)clampf(fade*160.f,0.f,255.f);
                    if(asp>3){
                        SDL_SetRenderDrawColor(ren,210,225,255,asp);
                        SDL_RenderDrawLine(ren,x0,y0,x1,y1);
                    }
                }
            }
        }
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    } else {
        /* Normal themes */
        int base2=(win_w<win_h?win_w:win_h);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        draw_blob(win_w,        0,            (int)(base2*0.70f),0.82f);
        draw_blob(0,            win_h,        (int)(base2*0.58f),0.65f);
        draw_blob(win_w*55/100, win_h*50/100, (int)(base2*0.42f),0.40f);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    /* ── Pass 6: Grain overlay (all themes) ─────────────────────── */
    if(grain_tex){
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        for(int ty=0;ty<win_h;ty+=GRAIN_SZ)
            for(int tx=0;tx<win_w;tx+=GRAIN_SZ){
                SDL_Rect dst={tx,ty,GRAIN_SZ,GRAIN_SZ};
                SDL_RenderCopy(ren,grain_tex,NULL,&dst);
            }
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }
}