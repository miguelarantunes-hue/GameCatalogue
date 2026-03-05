/*
 * ══════════════════════════════════════════════════════════════════
 *  GAME CATALOGUE  v5  ·  main.c
 *  + Spinning search box animation (rounded path + status colours)
 *  + AA rounded corners
 *  + Backlog removed
 *  + Resize / scrollbar conflict fixed
 *  + Click-through on focus fixed
 *  + No left colour strip on rows
 *  + Clean inactive button borders
 *  + Sort buttons: sliding pill + Z-A fix
 *
 *  BUILD (Windows MinGW):
 *    gcc -O2 -o gamelist.exe main.c games.c            \
 *        -I"C:/SDL2/include" -I"C:/SDL2/include/SDL2"  \
 *        -L"C:/SDL2/lib"                               \
 *        -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf        \
 *        -mwindows
 *
 *  BUILD (Linux/macOS):
 *    gcc -O2 -o gamelist main.c games.c                \
 *        $(sdl2-config --cflags --libs) -lSDL2_ttf -lm
 * ══════════════════════════════════════════════════════════════════
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "games.h"

#ifdef _WIN32
  #include <SDL2/SDL_syswm.h>
  #include <windows.h>
  #define WIN_RGN_R 8
#endif

/* ── Window state ─────────────────────────────────────────────── */
static int win_w = 1280;
static int win_h = 860;
#define MIN_W 700
#define MIN_H 480

/* ── Title bar ────────────────────────────────────────────────── */
#define TITLE_H   34
#define TB_BTN_W  40

/* ── Layout ───────────────────────────────────────────────────── */
#define HDR_Y    TITLE_H
#define HDR_H    68
#define TAB_Y   (TITLE_H + HDR_H)
#define TAB_H    44
#define SB_H     26
#define PG_BAR_Y (win_h - SB_H - PG_H)
#define LST_Y   (TITLE_H + HDR_H + TAB_H)
#define LST_H_  (win_h - LST_Y - SB_H - PG_H)
#define ROW_H    66
#define SCR_W    0
#define LIST_W_ (win_w)
#define PG_H     54   /* page bar height replaces scrollbar */
#define RESIZE_B  7

/* ── Status (Backlog removed) ─────────────────────────────────── */
#define N_STATUS   7
#define S_WISH     0
#define S_PLAYED   1
#define S_PLAYING  2
#define S_FINISHED 3
#define S_DROPPED  4
#define S_FAV      5
#define S_ROTATION 6

/* ── Buttons ──────────────────────────────────────────────────── */
#define BTN_SZ   30
#define BTN_GAP   4
#define BTN_AREA (N_STATUS * BTN_SZ + (N_STATUS - 1) * BTN_GAP)
#define BTN_LX_ (LIST_W_ - 12 - BTN_AREA)
#define BTN_YO  ((ROW_H - BTN_SZ) / 2)

/* ── Columns ──────────────────────────────────────────────────── */
#define YR_W     50
#define YR_X_   (BTN_LX_ - 14 - YR_W)
#define NM_X     16
#define NM_MW_  (YR_X_ - NM_X - 8)

/* ── Search bar ───────────────────────────────────────────────── */
#define SR_Y  (HDR_Y + 17)
#define SR_H   34
#define SR_R    9

/* ── Tabs ─────────────────────────────────────────────────────── */
#define N_TABS   8
#define TAB_GAP  2
#define TAB_W_  ((win_w - 8 - (N_TABS - 1) * TAB_GAP) / N_TABS)
#define TAB_X_(i) (8 + (i) * (TAB_W_ + TAB_GAP))

/* ── Title bar button centres ─────────────────────────────────── */
#define TB_CX (win_w - TB_BTN_W / 2)
#define TB_MX (win_w - TB_BTN_W - TB_BTN_W / 2)
#define TB_NX (win_w - 2 * TB_BTN_W - TB_BTN_W / 2)
#define IN_BTN(mx, cx) ((mx) >= (cx) - TB_BTN_W/2 && (mx) < (cx) + TB_BTN_W/2)

/* ── Misc ─────────────────────────────────────────────────────── */
#define MAX_G    2500
#define SAVEFILE_NAME "catalogue_save.bin"
#define SAVE_VER 0xCA7A2028u

static char save_path[512] = {0};

static void init_save_path(void){
    const char *appdata = getenv("APPDATA");
    if(!appdata) appdata = ".";
    snprintf(save_path, sizeof(save_path), "%s\\GameCatalogue", appdata);
    /* create directory if it doesn't exist */
    #ifdef _WIN32
    CreateDirectoryA(save_path, NULL);
    #else
    { char cmd[520]; snprintf(cmd,sizeof(cmd),"mkdir -p \"%s\"",save_path); system(cmd); }
    #endif
    snprintf(save_path + strlen(save_path),
             sizeof(save_path) - strlen(save_path),
             "\\%s", SAVEFILE_NAME);
}

/* ── Grid view ───────────────────────────────────────────────── */
#define GRID_W   180
#define GRID_H   158   /* enough for 2-line name + info + gap + buttons */
#define GRID_GAP     10
#define GRID_TOP_PAD 12   /* extra space between tabs and first grid row */
#define LIST_TOP_PAD  6   /* matches the row vertical inset (ay+3) */
#define GBSZ      22   /* grid card status button size */
#define GBGP       2   /* grid card status button gap  */

/* ── Titlebar controls ───────────────────────────────────────── */
#define TC_Y   6
#define TC_H  22
#define TC_W  68   /* sort pill width — fits "Newest"/"Oldest" in DejaVu Sans f14 */
#define VC_W  32   /* view button width — icon only, no text */
#define TC_GAP 4
#define TC_X0  8

/* ═══════════════════════ Types ═══════════════════════════════════ */
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

typedef struct {
    char name[128];
    char genre[32];
    int  year;
    char st[N_STATUS];
} Game;

typedef enum {
    T_ALL=0,T_WISH,T_PLAYED,T_PLAYING,
    T_FINISHED,T_DROPPED,T_FAV,T_ROTATION
} TabId;

typedef enum { RZ_NONE=0,RZ_N,RZ_NE,RZ_E,RZ_SE,RZ_S,RZ_SW,RZ_W,RZ_NW } RzDir;
typedef enum { TB_CLOSE_BTN, TB_MAX_BTN, TB_MIN_BTN } TBBtnType;

/* ═══════════════════════ Status metadata ═══════════════════════ */
static const char *SLBL[N_STATUS]  = {"WL","PD","IN","FN","DR","FV","RT"};
static const char *SNAME[N_TABS]   = {
    "All","Wishlist","Played","Playing",
    "Finished","Dropped","Favourites","Rotation"
};
static const C4 SCOL[N_STATUS] = {
    {255,200, 50,255},
    { 68,200, 98,255},
    { 40,212,178,255},
    { 52,140,255,255},
    {168, 56, 56,255},
    {255, 66,108,255},
    {152, 86,255,255},
};
static const int SPRIO[N_STATUS] = {
    S_FAV,S_PLAYING,S_FINISHED,S_PLAYED,
    S_ROTATION,S_WISH,S_DROPPED
};

/* ═══════════════════════ Globals ═══════════════════════════════ */
static Game  db[MAX_G];
static int   ndb=0, flt[MAX_G], nflt=0;
static int   flt_score[MAX_G];
static TabId cur_tab=T_ALL;
static char  srch[128]="";
static int   s_on=0, hov_db=-1;
/* ── Search edit state ──────────────────────────────────────────── */
static int   srch_cur    = 0;     /* cursor byte index               */
static int   srch_sel0   = 0;     /* selection anchor byte index     */
static float srch_blink  = 0.f;   /* cursor blink phase              */
static int   srch_scroll = 0;     /* horizontal scroll in pixels     */
static int   srch_drag   = 0;     /* mouse-dragging selection        */
static Uint32 srch_dbl_t = 0;     /* last click time for dbl-click   */
static int   srch_dbl_p  = -1;    /* last click char pos             */
static int   hsb=0, drag_sb=0, drag_sy=0, drag_sscr=0;
static int   win_maximized=0;
static int   pre_max_x=0, pre_max_y=0, pre_max_w=1280, pre_max_h=860;
static int   win_drag=0, win_drag_ox=0, win_drag_oy=0;
static RzDir rz_active=RZ_NONE;
static int   rz_drag=0,rz_gx0=0,rz_gy0=0,rz_wx0=0,rz_wy0=0,rz_ww0=0,rz_wh0=0;
static float row_ht[MAX_G];
static float btn_fl[MAX_G][N_STATUS];
static float btn_hov[MAX_G][N_STATUS];
static float scr_f=0.f, scr_tgt=0.f;
static int   cur_page=0;          /* current page (0-based)    */
static float page_slide=0.f;      /* animated page indicator   */
static float pg_prev_hov=0.f;     /* prev button hover         */
static float pg_next_hov=0.f;     /* next button hover         */
static float pg_num_hov[64];      /* per-dot hover             */
typedef enum { SORT_AZ=0, SORT_ZA, SORT_NEW, SORT_OLD } SortMode;
typedef enum { VIEW_LIST=0, VIEW_GRID } ViewMode;
static SortMode sort_mode = SORT_AZ;
static ViewMode view_mode = VIEW_LIST;
static float tc_sort_hov[4];
static float tc_view_hov[2];
static float sort_ind_f = 0.f;   /* sliding pill – index space */
static float view_ind_f = 0.f;   /* sliding pill – view mode   */
static float tab_ix=8.f, tab_itx=8.f;
static float tb_ch=0.f, tb_mh=0.f, tb_nh=0.f;
static float srch_spin=0.f;
static float srch_glow=0.f;
static Uint64 plast=0;
static float  dt_=0.016f;

/* ═══════════════════════ Themes + live colours ════════════════ */
#define N_THEMES 8

#define TDOT_R    9
#define TDOT_STEP 24

typedef struct {
    const char *name;
    C4 bg,hdr,title,tbar,rowa,rowb,rowh;
    C4 acc,txt,sub,dim;
    C4 scrbg,scrth,scrto;
    C4 sbar,srch,srcha,sep,btni,close;
} Theme;

static const Theme THEMES[N_THEMES] = {
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
};

static int   cur_theme = 0;
static float tdot_hov   [N_THEMES];
static float tdot_bounce[N_THEMES];
static float sel_ring_if = 0.f;
static float theme_pulse = 0.f;
static int   dots_in_tb  = 0;
static int   dots_in_tb_prev = -1;
static int   dots_x0     = 0;
static int   dots_cy     = 0;
static int   sr_x        = 380;
static int   sr_w        = 380;

#define N_CSLOTS 20
#define TC_SPEED 5.5f
static C4    tc_from[N_CSLOTS];
static C4    tc_to  [N_CSLOTS];
static float tc_t = 1.f;

static C4 C_BG,C_HDR,C_TITLE,C_TBAR,C_ROWA,C_ROWB,C_ROWH,
          C_ACC,C_TXT,C_SUB,C_DIM,
          C_SCRBG,C_SCRTH,C_SCRTO,
          C_SBAR,C_SRCH,C_SRCHA,C_SEP,C_BTNI,C_CLOSE;

static void theme_pack(const Theme *t, C4 *s){
    s[0]=t->bg;    s[1]=t->hdr;   s[2]=t->title; s[3]=t->tbar;
    s[4]=t->rowa;  s[5]=t->rowb;  s[6]=t->rowh;  s[7]=t->acc;
    s[8]=t->txt;   s[9]=t->sub;   s[10]=t->dim;
    s[11]=t->scrbg;s[12]=t->scrth;s[13]=t->scrto;
    s[14]=t->sbar; s[15]=t->srch; s[16]=t->srcha;s[17]=t->sep;
    s[18]=t->btni; s[19]=t->close;
}
static void theme_apply_slots(const C4 *s){
    C_BG=s[0];  C_HDR=s[1];   C_TITLE=s[2];  C_TBAR=s[3];
    C_ROWA=s[4];C_ROWB=s[5];  C_ROWH=s[6];   C_ACC=s[7];
    C_TXT=s[8]; C_SUB=s[9];   C_DIM=s[10];
    C_SCRBG=s[11];C_SCRTH=s[12];C_SCRTO=s[13];
    C_SBAR=s[14];C_SRCH=s[15];C_SRCHA=s[16]; C_SEP=s[17];
    C_BTNI=s[18];C_CLOSE=s[19];
}
static void tick_theme(float dt){
    if(tc_t>=1.f) return;
    tc_t+=dt*TC_SPEED; if(tc_t>1.f) tc_t=1.f;
    C4 cur[N_CSLOTS];
    for(int i=0;i<N_CSLOTS;i++) cur[i]=lerpc(tc_from[i],tc_to[i],tc_t);
    theme_apply_slots(cur);
}
static void set_theme(int idx){
    cur_theme=idx;
    tc_from[0]=C_BG;   tc_from[1]=C_HDR;   tc_from[2]=C_TITLE;
    tc_from[3]=C_TBAR; tc_from[4]=C_ROWA;  tc_from[5]=C_ROWB;
    tc_from[6]=C_ROWH; tc_from[7]=C_ACC;   tc_from[8]=C_TXT;
    tc_from[9]=C_SUB;  tc_from[10]=C_DIM;  tc_from[11]=C_SCRBG;
    tc_from[12]=C_SCRTH;tc_from[13]=C_SCRTO;tc_from[14]=C_SBAR;
    tc_from[15]=C_SRCH; tc_from[16]=C_SRCHA;tc_from[17]=C_SEP;
    tc_from[18]=C_BTNI; tc_from[19]=C_CLOSE;
    theme_pack(&THEMES[idx],tc_to);
    tc_t=0.f; theme_pulse=1.f;
}
static void set_theme_instant(int idx){
    cur_theme=idx;
    theme_pack(&THEMES[idx],tc_to);
    for(int i=0;i<N_CSLOTS;i++) tc_from[i]=tc_to[i];
    tc_t=1.f;
    theme_apply_slots(tc_to);
    sel_ring_if=(float)idx;
}

static void compute_dot_layout(void){
    int right_pad  = 14;
    int hdr_x0     = win_w - right_pad - (N_THEMES-1)*TDOT_STEP;
    int fh12       = 22;
    int total_h    = fh12 + 4 + TDOT_R*2 + 4 + fh12;
    int hdr_cy     = HDR_Y + (HDR_H - total_h)/2 + fh12 + 4 + TDOT_R;
    int need = 380 + 380 + 16 + (N_THEMES-1)*TDOT_STEP + TDOT_R + right_pad;
    if(win_w >= need){
        dots_in_tb = 0;
        dots_x0    = hdr_x0;
        dots_cy    = hdr_cy;
        sr_x       = 380;
        sr_w       = 380;
    } else {
        dots_in_tb = 1;
        int right_edge = win_w - 3*TB_BTN_W - 20;
        dots_x0    = right_edge - (N_THEMES-1)*TDOT_STEP;
        dots_cy    = TITLE_H/2;
        sr_w       = 380;
        sr_x       = win_w - right_pad - 10 - sr_w;
    }
}

static int hit_theme_dot(int mx,int my){
    for(int i=0;i<N_THEMES;i++){
        int cx=dots_x0+i*TDOT_STEP,dx=mx-cx,dy=my-dots_cy;
        if(dx*dx+dy*dy<=(TDOT_R+4)*(TDOT_R+4)) return i;
    }
    return -1;
}

/* ═══════════════════════ SDL globals ═══════════════════════════ */
static SDL_Window   *win=NULL;
static SDL_Renderer *ren=NULL;
static TTF_Font     *f22=NULL,*f18=NULL,*f14=NULL,*f12=NULL;
static SDL_Cursor   *cur_arr=NULL,*cur_ns=NULL,*cur_ew=NULL,
                    *cur_nwse=NULL,*cur_nesw=NULL;

/* ═══════════════════════ Audio / SFX ═══════════════════════════ */
static SDL_AudioDeviceID aud_dev=0;

#define SFX_RATE 44100

static void sfx_play(float freq,float dur,float vol,float decay){
    if(!aud_dev) return;
    int n=(int)(SFX_RATE*dur);
    Sint16 *buf=(Sint16*)malloc(n*sizeof(Sint16));
    if(!buf) return;
    for(int i=0;i<n;i++){
        float t=(float)i/SFX_RATE;
        float env=expf(-t*decay);
        float s=sinf(2.f*(float)M_PI*freq*t)*env*vol*32767.f;
        if(s> 32767.f) s= 32767.f;
        if(s<-32767.f) s=-32767.f;
        buf[i]=(Sint16)s;
    }
    SDL_QueueAudio(aud_dev,buf,n*sizeof(Sint16));
    free(buf);
}

/* Cooldown timestamps — one per sfx category */
static Uint32 sfx_last_click=0, sfx_last_toggle=0, sfx_last_tab=0, sfx_last_type=0, sfx_last_sort=0;
static void sfx_click (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_click <60)  return; sfx_last_click =now; sfx_play( 900.f,0.055f,0.10f,45.f); }
static void sfx_toggle(void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_toggle<80)  return; sfx_last_toggle=now; sfx_play( 660.f,0.075f,0.14f,35.f); }
static void sfx_tab   (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_tab  <50)  return; sfx_last_tab   =now; sfx_play(1100.f,0.045f,0.08f,55.f); }
static void sfx_type  (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_type <30)  return; sfx_last_type  =now; sfx_play(1400.f,0.025f,0.04f,80.f); }
static void sfx_sort  (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_sort <200) return; sfx_last_sort  =now; sfx_play(1100.f,0.045f,0.08f,55.f); }

/* ── Forward declarations ─────────────────────────────────────── */
static void rebuild(void);
static int  grid_cols(void);
static int  row_at(int mx,int my);
static int  btn_at(int mx,int my,int ri);

/* ═══════════════════════ Draw helpers ══════════════════════════ */
static void sc_(C4 c){ SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a); }
static void fr_(int x,int y,int w,int h,C4 c){
    sc_(c); SDL_Rect r={x,y,w,h}; SDL_RenderFillRect(ren,&r);
}

static void frr(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>w) r=w/2; if(2*r>h) r=h/2;
    sc_(c);
    float rf=(float)r;
    for(int row=0;row<h;row++){
        int x0=x,x1=x+w;
        if(row<r){ int d=r-row; int dx=(int)sqrtf(rf*rf-(float)(d*d)); x0=x+r-dx; x1=x+w-r+dx; }
        else if(row>=h-r){ int d=row-(h-r)+1; int dx=(int)sqrtf(rf*rf-(float)(d*d)); x0=x+r-dx; x1=x+w-r+dx; }
        if(x1>x0) SDL_RenderDrawLine(ren,x0,y+row,x1-1,y+row);
    }
}

static void frr_aa(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>w) r=w/2; if(2*r>h) r=h/2;
    float rf=(float)r;
    sc_(c);
    /* middle rows – full width */
    for(int row=r;row<h-r;row++)
        SDL_RenderDrawLine(ren,x,y+row,x+w-1,y+row);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    for(int row=0;row<r;row++){
        float d   = (float)(r-row) - 0.5f;
        float dxf = sqrtf(rf*rf - d*d);
        int   dxi = (int)dxf;
        float frac = dxf - (float)dxi;
        int lx = x + r - dxi;
        int rx = x + w - 1 - r + dxi;
        sc_(c);
        if(rx >= lx){
            SDL_RenderDrawLine(ren, lx, y+row,       rx, y+row);
            SDL_RenderDrawLine(ren, lx, y+h-1-row,   rx, y+h-1-row);
        }
        C4 aa=c; aa.a=(Uint8)(c.a*frac);
        sc_(aa);
        SDL_RenderDrawPoint(ren, lx-1, y+row);
        SDL_RenderDrawPoint(ren, rx+1, y+row);
        SDL_RenderDrawPoint(ren, lx-1, y+h-1-row);
        SDL_RenderDrawPoint(ren, rx+1, y+h-1-row);
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

static void bfrr(int x,int y,int w,int h,int r,int t,C4 bc,C4 ic){
    frr(x,y,w,h,r,bc);
    if(w>2*t&&h>2*t) frr(x+t,y+t,w-2*t,h-2*t,r>t?r-t:0,ic);
}
static void bfrr_aa(int x,int y,int w,int h,int r,int t,C4 bc,C4 ic){
    frr_aa(x,y,w,h,r,bc);
    if(w>2*t&&h>2*t) frr_aa(x+t,y+t,w-2*t,h-2*t,r>t?r-t:0,ic);
}
static void fblend(int x,int y,int w,int h,C4 c){
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    sc_(c); SDL_Rect r={x,y,w,h}; SDL_RenderFillRect(ren,&r);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}
static void rtx(TTF_Font *f,const char *s,int x,int y,C4 c){
    if(!f||!s||!*s) return;
    SDL_Color col={c.r,c.g,c.b,c.a};
    SDL_Surface *sf=TTF_RenderUTF8_Blended(f,s,col); if(!sf) return;
    SDL_Texture *tx=SDL_CreateTextureFromSurface(ren,sf);
    SDL_Rect d={x,y,sf->w,sf->h}; SDL_RenderCopy(ren,tx,NULL,&d);
    SDL_DestroyTexture(tx); SDL_FreeSurface(sf);
}
static void rtxclip(TTF_Font *f,const char *s,int x,int y,int mw,C4 c){
    if(!f||!s||!*s||mw<=0) return;
    SDL_Color col={c.r,c.g,c.b,c.a};
    SDL_Surface *sf=TTF_RenderUTF8_Blended(f,s,col); if(!sf) return;
    SDL_Texture *tx=SDL_CreateTextureFromSurface(ren,sf);
    int tw=sf->w,th=sf->h; SDL_FreeSurface(sf);
    SDL_Rect src={0,0,tw<mw?tw:mw,th},dst={x,y,src.w,src.h};
    SDL_RenderCopy(ren,tx,&src,&dst); SDL_DestroyTexture(tx);
}
static int txw_(TTF_Font *f,const char *s){
    int w=0,h=0; if(f&&s) TTF_SizeUTF8(f,s,&w,&h); return w;
}
static void rtxcen(TTF_Font *f,const char *s,int rx,int ry,int rw,int rh,C4 c){
    int w=txw_(f,s),h=f?TTF_FontHeight(f):0;
    rtx(f,s,rx+(rw-w)/2,ry+(rh-h)/2,c);
}

/* ═══════════════════════ Genre colours ════════════════════════ */
static C4 gcol(const char *g){
    if(!strcmp(g,"RPG"))           return MK4( 70, 40,135,255);
    if(!strcmp(g,"FPS"))           return MK4(150, 40, 40,255);
    if(!strcmp(g,"TPS"))           return MK4(130, 56, 30,255);
    if(!strcmp(g,"Horror"))        return MK4( 85, 10, 10,255);
    if(!strcmp(g,"Racing"))        return MK4( 30,100, 46,255);
    if(!strcmp(g,"Strategy"))      return MK4( 30, 60,125,255);
    if(!strcmp(g,"Adventure"))     return MK4( 36, 86,116,255);
    if(!strcmp(g,"Action"))        return MK4(126, 56, 20,255);
    if(!strcmp(g,"Fighting"))      return MK4(135, 30, 65,255);
    if(!strcmp(g,"Simulation"))    return MK4( 26,100, 65,255);
    if(!strcmp(g,"Sports"))        return MK4( 20, 90,135,255);
    if(!strcmp(g,"Puzzle"))        return MK4( 86, 76, 30,255);
    if(!strcmp(g,"Indie"))         return MK4( 80, 40,105,255);
    if(!strcmp(g,"Platformer"))    return MK4( 96, 66, 20,255);
    if(!strcmp(g,"MMORPG"))        return MK4( 36, 80, 46,255);
    if(!strcmp(g,"Open World"))    return MK4( 46, 80, 50,255);
    if(!strcmp(g,"Battle Royale")) return MK4(135, 70, 14,255);
    if(!strcmp(g,"Card Game"))     return MK4( 55, 92, 65,255);
    if(!strcmp(g,"Metroidvania"))  return MK4( 65, 36, 95,255);
    if(!strcmp(g,"Co-op"))         return MK4( 46, 80,100,255);
    return MK4(44,44,80,255);
}
static C4 strip_col(Game *g){
    for(int i=0;i<N_STATUS;i++) if(g->st[SPRIO[i]]) return SCOL[SPRIO[i]];
    return gcol(g->genre);
}

/* ═══════════════════════ Windows rounded region ════════════════ */
#ifdef _WIN32
static void apply_rgn(int w,int h,int rounded){
    SDL_SysWMinfo info; SDL_VERSION(&info.version);
    if(!SDL_GetWindowWMInfo(win,&info)) return;
    HWND hwnd=info.info.win.window;
    if(rounded){ int d=WIN_RGN_R*2; HRGN rgn=CreateRoundRectRgn(0,0,w+1,h+1,d,d); SetWindowRgn(hwnd,rgn,TRUE); }
    else SetWindowRgn(hwnd,NULL,TRUE);
}
#else
static void apply_rgn(int w,int h,int r){(void)w;(void)h;(void)r;}
#endif

/* ═══════════════════════ Resize ════════════════════════════════ */
static RzDir get_rz(int mx,int my){
    if(win_maximized) return RZ_NONE;
    int b=RESIZE_B;
    int in_sb = (mx >= LIST_W_ && my >= LST_Y && my < LST_Y+LST_H_);
    int l=(mx<b);
    int r=(!in_sb)&&(mx>=win_w-b);
    int t=(my<b);
    int bo=(my>=win_h-b);
    if(t&&l)return RZ_NW; if(t&&r)return RZ_NE;
    if(bo&&r)return RZ_SE; if(bo&&l)return RZ_SW;
    if(t)return RZ_N; if(r)return RZ_E; if(bo)return RZ_S; if(l)return RZ_W;
    return RZ_NONE;
}
static SDL_Cursor* cur_for(RzDir d){
    switch(d){
    case RZ_N: case RZ_S:   return cur_ns;
    case RZ_E: case RZ_W:   return cur_ew;
    case RZ_NW: case RZ_SE: return cur_nwse;
    case RZ_NE: case RZ_SW: return cur_nesw;
    default:                return cur_arr;
    }
}
static void do_rz_move(void){
    if(!rz_drag) return;
    int gx,gy; SDL_GetGlobalMouseState(&gx,&gy);
    int dx=gx-rz_gx0, dy=gy-rz_gy0;
    int nx=rz_wx0,ny=rz_wy0,nw=rz_ww0,nh=rz_wh0;
    switch(rz_active){
    case RZ_E:  nw=rz_ww0+dx; break;
    case RZ_W:  nx=rz_wx0+dx; nw=rz_ww0-dx; break;
    case RZ_S:  nh=rz_wh0+dy; break;
    case RZ_N:  ny=rz_wy0+dy; nh=rz_wh0-dy; break;
    case RZ_SE: nw=rz_ww0+dx; nh=rz_wh0+dy; break;
    case RZ_SW: nx=rz_wx0+dx; nw=rz_ww0-dx; nh=rz_wh0+dy; break;
    case RZ_NE: nw=rz_ww0+dx; ny=rz_wy0+dy; nh=rz_wh0-dy; break;
    case RZ_NW: nx=rz_wx0+dx; nw=rz_ww0-dx; ny=rz_wy0+dy; nh=rz_wh0-dy; break;
    default: break;
    }
    if(nw<MIN_W){ if(rz_active==RZ_W||rz_active==RZ_NW||rz_active==RZ_SW) nx=rz_wx0+rz_ww0-MIN_W; nw=MIN_W; }
    if(nh<MIN_H){ if(rz_active==RZ_N||rz_active==RZ_NW||rz_active==RZ_NE) ny=rz_wy0+rz_wh0-MIN_H; nh=MIN_H; }
    SDL_SetWindowPosition(win,nx,ny);
    SDL_SetWindowSize(win,nw,nh);
    win_w=nw; win_h=nh;
    tab_ix = tab_itx = (float)TAB_X_((int)cur_tab);
    apply_rgn(win_w,win_h,1);
}

/* ═══════════════════════ Maximize ══════════════════════════════ */
static void toggle_maximize(void){
    if(win_maximized){
        SDL_SetWindowPosition(win,pre_max_x,pre_max_y);
        SDL_SetWindowSize(win,pre_max_w,pre_max_h);
        win_w=pre_max_w; win_h=pre_max_h;
        win_maximized=0;
        apply_rgn(win_w,win_h,1);
    } else {
        SDL_GetWindowPosition(win,&pre_max_x,&pre_max_y);
        pre_max_w=win_w; pre_max_h=win_h;
        SDL_Rect r; int di=SDL_GetWindowDisplayIndex(win);
        if(di<0) di=0;
        SDL_GetDisplayUsableBounds(di,&r);
        SDL_SetWindowPosition(win,r.x,r.y);
        SDL_SetWindowSize(win,r.w,r.h);
        win_w=r.w; win_h=r.h;
        win_maximized=1;
        apply_rgn(win_w,win_h,0);
    }
    tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);
    rebuild();
}

/* ═══════════════════════ Animation tick ════════════════════════ */
/* ── Page helper forward declarations ───────────────────────────── */
static int page_size(void);
static int total_pages(void);
static void clamp_page(void);
static int page_first(void);
static int page_last(void);

/* returns 1 if anything is still animating (caller should redraw) */
static int anim_tick(void){
    compute_dot_layout();
    if(dots_in_tb != dots_in_tb_prev){
        sel_ring_if = (float)cur_theme;
        dots_in_tb_prev = dots_in_tb;
    }
    Uint64 now=SDL_GetPerformanceCounter();
    if(!plast){ plast=now; return 1; }
    dt_=(float)(now-plast)/(float)SDL_GetPerformanceFrequency();
    if(dt_>0.1f) dt_=0.1f;
    plast=now;

    int busy = 0; /* tracks whether any animation is still running */
#define SETTLE(val,tgt,spd) do{     float _d=(tgt)-(val);     if(fabsf(_d)<0.0015f){(val)=(tgt);}     else{(val)+= _d*(spd); busy=1;} }while(0)

    /* ── Scroll ── */
    { float sp=1.f-powf(0.00008f,dt_); SETTLE(scr_f,scr_tgt,sp); }

    /* ── Grid hover detection ── */
    if(view_mode==VIEW_GRID){
        int mx4,my4; SDL_GetMouseState(&mx4,&my4);
        hov_db=-1;
        if(my4>=LST_Y&&my4<LST_Y+LST_H_&&mx4>=0&&mx4<LIST_W_){
            int cols=grid_cols();
            int block_w=cols*(GRID_W+GRID_GAP)-GRID_GAP;
            int ox=(LIST_W_-block_w)/2;
            int pg_cnt2=page_last()-page_first()+1;
            int rows_pg=(pg_cnt2+cols-1)/cols; if(rows_pg<1) rows_pg=1;
            int used_h2=rows_pg*(GRID_H+GRID_GAP)-GRID_GAP;
            int oy2=LST_Y+(LST_H_-used_h2)/2; if(oy2<LST_Y) oy2=LST_Y;
            int row2=(my4-oy2)/(GRID_H+GRID_GAP);
            int col2=(mx4-ox)/(GRID_W+GRID_GAP);
            if(col2>=0&&col2<cols&&row2>=0){
                int local=row2*cols+col2;
                int ri=page_first()+local;
                int cy2=oy2+row2*(GRID_H+GRID_GAP);
                int cx2=ox+col2*(GRID_W+GRID_GAP);
                if(mx4>=cx2&&mx4<cx2+GRID_W&&my4>=cy2&&my4<cy2+GRID_H&&ri<=page_last()&&ri<nflt)
                    hov_db=flt[ri];
            }
        }
    }

    /* ── Row hover – only visible entries ── */
    {
        float hs=clampf(dt_*22.f,0.f,1.f);
        int first = page_first();
        int last  = page_last();

        /* decay any entry that went off-screen but still has hover */
        if(hov_db>=0 && (hov_db<first||hov_db>last)){
            if(row_ht[hov_db]>0.0015f){ row_ht[hov_db]-=hs; busy=1; }
            else row_ht[hov_db]=0.f;
        }
        for(int ri=first;ri<=last;ri++){
            int i=flt[ri];
            float tg=(i==hov_db)?1.f:0.f;
            if(fabsf(row_ht[i]-tg)<0.0015f){ row_ht[i]=tg; }
            else { row_ht[i]+=(tg-row_ht[i])*hs; busy=1; }
        }
    }

    /* ── Button flash fade (only non-zero entries) ── */
    {
        float bd=dt_*3.5f;
        for(int i=0;i<ndb;i++)
            for(int j=0;j<N_STATUS;j++)
                if(btn_fl[i][j]>0.f){
                    btn_fl[i][j]-=bd;
                    if(btn_fl[i][j]<0) btn_fl[i][j]=0;
                    busy=1;
                }
    }

    /* ── Tab pill ── */
    {
        float slide=clampf(dt_*20.f,0.f,1.f);
        tab_itx=(float)TAB_X_((int)cur_tab);
        if(rz_drag||win_drag){ tab_ix=tab_itx; }
        else { SETTLE(tab_ix,tab_itx,slide); }
        SETTLE(sort_ind_f,(float)sort_mode,slide);
        SETTLE(view_ind_f,(float)view_mode,slide);
    }

    /* ── Titlebar close/max/min hover ── */
    {
        int mx,my; SDL_GetMouseState(&mx,&my);
        int has_focus=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        float bs=1.f-powf(0.001f,dt_);
        float c_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_CX))?1.f:0.f;
        float m_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_MX))?1.f:0.f;
        float n_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_NX))?1.f:0.f;
        SETTLE(tb_ch,c_tgt,bs); SETTLE(tb_mh,m_tgt,bs); SETTLE(tb_nh,n_tgt,bs);
    }

    /* ── Theme ring slide ── */
    SETTLE(sel_ring_if,(float)cur_theme,clampf(dt_*20.f,0.f,1.f));

    /* ── Theme colour transition + dot animations ── */
    tick_theme(dt_);
    if(tc_t<1.f) busy=1;
    {
        int mx2,my2; SDL_GetMouseState(&mx2,&my2);
        int mf=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        float ths=1.f-powf(0.002f,dt_);
        for(int i=0;i<N_THEMES;i++){
            float tg=(mf&&hit_theme_dot(mx2,my2)==i)?1.f:0.f;
            SETTLE(tdot_hov[i],tg,ths);
        }
        for(int i=0;i<N_THEMES;i++){
            if(tdot_bounce[i]>0.f){
                tdot_bounce[i]-=dt_*5.5f;
                if(tdot_bounce[i]<0.f) tdot_bounce[i]=0.f;
                busy=1;
            }
        }
        if(theme_pulse>0.f){theme_pulse-=dt_*4.f;if(theme_pulse<0)theme_pulse=0; busy=1;}

        /* ── Sort/view button hover ── */
        float ths2=1.f-powf(0.003f,dt_);
        int mx3,my3; SDL_GetMouseState(&mx3,&my3);
        int hf2=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        for(int i=0;i<4;i++){
            int bx=TC_X0+i*(TC_W+TC_GAP);
            float tg=(hf2&&my3>=0&&my3<TITLE_H&&mx3>=bx&&mx3<bx+TC_W)?1.f:0.f;
            SETTLE(tc_sort_hov[i],tg,ths2);
        }
        for(int i=0;i<2;i++){
            int bx=TC_X0+4*(TC_W+TC_GAP)+TC_GAP+10+i*(VC_W+TC_GAP);
            float tg=(hf2&&my3>=0&&my3<TITLE_H&&mx3>=bx&&mx3<bx+VC_W)?1.f:0.f;
            SETTLE(tc_view_hov[i],tg,ths2);
        }

        /* ── Button hover (skip entries already at target) ── */
        float bspd=1.f-powf(0.0004f,dt_);
        int hov_gi=-1,hov_bj=-1;
        if(view_mode==VIEW_LIST){
            int hov_ri2=row_at(mx2,my2);
            if(hov_ri2>=0&&hov_ri2<nflt){
                int gi2=flt[hov_ri2];
                int j2=btn_at(mx2,my2,hov_ri2);
                if(j2>=0){hov_gi=gi2;hov_bj=j2;}
            }
        } else {
            if(hov_db>=0&&my2>=LST_Y&&my2<LST_Y+LST_H_){
                int cols=grid_cols();
                int block_w=cols*(GRID_W+GRID_GAP)-GRID_GAP;
                int ox=(LIST_W_-block_w)/2;
                int pg_cnt3=page_last()-page_first()+1;
                int rows_pg3=(pg_cnt3+cols-1)/cols; if(rows_pg3<1) rows_pg3=1;
                int used_h4=rows_pg3*(GRID_H+GRID_GAP)-GRID_GAP;
                int oy4=LST_Y+(LST_H_-used_h4)/2; if(oy4<LST_Y) oy4=LST_Y;
                for(int ri2=page_first();ri2<=page_last();ri2++){
                    if(flt[ri2]!=hov_db) continue;
                    int local3=ri2-page_first();
                    int row2=local3/cols,col2=local3%cols;
                    int cx2=ox+col2*(GRID_W+GRID_GAP);
                    int cy2=oy4+row2*(GRID_H+GRID_GAP);
                    int total_w2=N_STATUS*(GBSZ+GBGP)-GBGP;
                    int bx02=cx2+(GRID_W-total_w2)/2;
                    int by2=cy2+GRID_H-GBSZ-4;
                    if(my2>=by2&&my2<by2+GBSZ)
                        for(int j=0;j<N_STATUS;j++){
                            int bx=bx02+j*(GBSZ+GBGP);
                            if(mx2>=bx&&mx2<bx+GBSZ){hov_gi=hov_db;hov_bj=j;break;}
                        }
                    break;
                }
            }
        }
        /* Only iterate visible entries for btn_hov */
        {
                int vfirst=page_first();
            int vlast =page_last();
            for(int ri=vfirst;ri<=vlast;ri++){
                int i=flt[ri];
                for(int j=0;j<N_STATUS;j++){
                    float tgt=(i==hov_gi&&j==hov_bj)?1.f:0.f;
                    if(btn_hov[i][j]==0.f&&tgt==0.f) continue;
                    SETTLE(btn_hov[i][j],tgt,bspd);
                }
            }
            /* fade out any entry that's now off-screen but still lit */
            if(hov_gi>=0){
                int ri_hov=-1;
                for(int ri=0;ri<nflt;ri++) if(flt[ri]==hov_gi){ri_hov=ri;break;}
                if(ri_hov<vfirst||ri_hov>vlast){
                    for(int j=0;j<N_STATUS;j++)
                        if(btn_hov[hov_gi][j]>0.f){
                            SETTLE(btn_hov[hov_gi][j],0.f,bspd);
                        }
                }
            }
        }
    }

    /* ── Page bar hover ── */
    {
        int mx2,my2; SDL_GetMouseState(&mx2,&my2);
        int py=PG_BAR_Y, ph=PG_H;
        float spd=1.f-powf(0.003f,dt_);
        int bw=80,bh=34,cy2=py+(ph-TTF_FontHeight(f14)-6)/2;
        int prev_x=14, next_x_=win_w-14-bw;
        int in_bar=(my2>=py&&my2<py+ph);
        float prev_tgt=(in_bar&&mx2>=prev_x&&mx2<prev_x+bw&&abs(my2-(py+ph/2))<=bh/2&&cur_page>0)?1.f:0.f;
        float next_tgt=(in_bar&&mx2>=next_x_&&mx2<next_x_+bw&&abs(my2-(py+ph/2))<=bh/2&&cur_page<total_pages()-1)?1.f:0.f;
        SETTLE(pg_prev_hov,prev_tgt,spd);
        SETTLE(pg_next_hov,next_tgt,spd);

        /* dot hovers — only the closest dot lights up */
        int tp2=total_pages();
        {
            #define MD2 13
            int show2=tp2<MD2?tp2:MD2;
            int dot_r2=6,dot_gap2=18;
            int total_dot_w2=show2*dot_gap2-dot_gap2+dot_r2*2;
            int dot_x02=(win_w-total_dot_w2)/2;
            float half2f=(float)(MD2/2);
            float ws2=page_slide-half2f;
            if(ws2<0.f) ws2=0.f;
            if(ws2>(float)(tp2-show2)) ws2=(float)(tp2-show2);
            if(ws2<0.f) ws2=0.f;
            /* find closest dot to cursor */
            int closest_p=-1; int closest_dist=INT_MAX;
            for(int p=0;p<tp2&&p<64;p++){
                float local_f2=(float)p-ws2;
                int cx2=(int)((float)dot_x02+local_f2*(float)dot_gap2+(float)dot_r2+0.5f);
                int dist=abs(mx2-cx2);
                if(in_bar&&abs(my2-cy2)<=dot_r2+4&&dist<=dot_r2+4&&dist<closest_dist){
                    closest_dist=dist; closest_p=p;
                }
            }
            for(int p=0;p<tp2&&p<64;p++){
                float tgt=(p==closest_p)?1.f:0.f;
                SETTLE(pg_num_hov[p],tgt,spd);
            }
            #undef MD2
        }
        SETTLE(page_slide,(float)cur_page,clampf(dt_*14.f,0.f,1.f));
        if(fabsf(page_slide-(float)cur_page)>0.001f) busy=1;
    }

    /* ── Cursor blink (always busy when search focused) ── */
    if(s_on){ srch_blink+=dt_*1.8f; if(srch_blink>=2.f) srch_blink-=2.f; busy=1; }
    else srch_blink=0.f;

    /* ── Search spin ── */
    {
        float stgt=(s_on||*srch)?1.f:0.f;
        if(fabsf(srch_glow-stgt)<0.002f) srch_glow=stgt;
        else{ srch_glow+=(stgt-srch_glow)*(1.f-powf(0.0008f,dt_)); busy=1; }
        if(srch_glow>0.01f){
            float rf2=(float)SR_R;
            float arc2=rf2*(float)M_PI/2.f;
            float sx2=(float)sr_w-2.f*rf2;
            float sy2=(float)SR_H-2.f*rf2;
            float perim=2.f*(sx2+sy2)+4.f*arc2;
            float spd=s_on?460.f:220.f;
            srch_spin=fmodf(srch_spin+dt_*spd,perim);
            busy=1;
        }
    }

#undef SETTLE
    return busy;
}

/* ═══════════════════════ Page helpers ══════════════════════════ */
static int page_size_list(void){ int r=LST_H_/ROW_H; return r<1?1:r; }
static int page_size_grid(void){
    int cols=grid_cols();
    int rows=LST_H_/(GRID_H+GRID_GAP); if(rows<1) rows=1;
    return cols*rows;
}
static int page_size(void){ return (view_mode==VIEW_GRID)?page_size_grid():page_size_list(); }
static int total_pages(void){
    int ps=page_size(); if(ps<1) ps=1;
    int tp=(nflt+ps-1)/ps; return tp<1?1:tp;
}
static void clamp_page(void){
    int tp=total_pages();
    if(cur_page>=tp) cur_page=tp-1;
    if(cur_page<0)   cur_page=0;
}
static int page_first(void){ clamp_page(); return cur_page*page_size(); }
static int page_last (void){
    int last=page_first()+page_size()-1;
    if(last>=nflt) last=nflt-1;
    return last;
}
static void go_page(int p){
    int tp=total_pages();
    if(p<0) p=0; if(p>=tp) p=tp-1;
    cur_page=p;
}

/* ═══════════════════════ Search helpers ════════════════════════ */

/* pixel x of char at byte index pos (in text coordinate space) */
static int srch_pos_px(int pos){
    char tmp[128]; if(pos<=0) return 0;
    int l=(int)strlen(srch); if(pos>l) pos=l;
    strncpy(tmp,srch,pos); tmp[pos]=0;
    return txw_(f14,tmp);
}

/* byte index of char closest to pixel x in text coordinate space */
static int srch_px_to_pos(int px){
    int len=(int)strlen(srch);
    if(px<=0) return 0;
    /* binary search */
    int lo=0,hi=len;
    while(lo<hi){
        int mid=(lo+hi+1)/2;
        char tmp[128]; strncpy(tmp,srch,mid); tmp[mid]=0;
        if(txw_(f14,tmp)<=px) lo=mid; else hi=mid-1;
    }
    /* check if lo+1 is closer */
    if(lo<len){
        char t1[128],t2[128];
        strncpy(t1,srch,lo);   t1[lo]=0;
        strncpy(t2,srch,lo+1); t2[lo+1]=0;
        int w1=txw_(f14,t1), w2=txw_(f14,t2);
        if(px-w1 > w2-px) lo=lo+1;
    }
    return lo;
}

static void srch_ensure_visible(void){
    int tx_mw = sr_w - 24;
    int cpx = srch_pos_px(srch_cur);
    if(cpx - srch_scroll < 0) srch_scroll = cpx;
    if(cpx - srch_scroll > tx_mw - 4) srch_scroll = cpx - tx_mw + 4;
    if(srch_scroll<0) srch_scroll=0;
}

/* delete selected text; returns 1 if anything was deleted */
static int srch_delete_sel(void){
    int a=srch_sel0<srch_cur?srch_sel0:srch_cur;
    int b=srch_sel0<srch_cur?srch_cur :srch_sel0;
    if(a==b) return 0;
    int len=(int)strlen(srch);
    memmove(srch+a,srch+b,len-b+1);
    srch_cur=a; srch_sel0=a;
    return 1;
}

/* insert text at cursor (replaces selection) */
static void srch_insert(const char *txt){
    srch_delete_sel();
    int ins=(int)strlen(txt);
    int len=(int)strlen(srch);
    if(len+ins>=127){ ins=127-1-len; if(ins<=0) return; }
    memmove(srch+srch_cur+ins,srch+srch_cur,len-srch_cur+1);
    memcpy(srch+srch_cur,txt,ins);
    srch_cur+=ins; srch_sel0=srch_cur;
    srch_ensure_visible();
}

static void srch_copy(void){
    int a=srch_sel0<srch_cur?srch_sel0:srch_cur;
    int b=srch_sel0<srch_cur?srch_cur :srch_sel0;
    if(a==b) return;
    char tmp[128]; strncpy(tmp,srch+a,b-a); tmp[b-a]=0;
    SDL_SetClipboardText(tmp);
}

/* move cursor, optionally extending selection */
static void srch_move(int new_cur, int extend){
    if(!extend) srch_sel0=new_cur;
    srch_cur=new_cur;
    srch_blink=0.f;
    srch_ensure_visible();
}

/* word boundary navigation */
static int srch_word_left(int p){
    while(p>0 && !isalnum((unsigned char)srch[p-1])) p--;
    while(p>0 &&  isalnum((unsigned char)srch[p-1])) p--;
    return p;
}
static int srch_word_right(int p){
    int l=(int)strlen(srch);
    while(p<l && !isalnum((unsigned char)srch[p])) p++;
    while(p<l &&  isalnum((unsigned char)srch[p])) p++;
    return p;
}

static void srch_focus(void){
    s_on=1; SDL_StartTextInput(); srch_blink=0.f;
}
static void srch_blur(void){
    s_on=0; srch_drag=0;
}

/* ═══════════════════════ Filter / counts ═══════════════════════ */
/* ── Intelligent search scoring ─────────────────────────────────
   Returns 0 = no match, higher = better match.
   Layers (cumulative):
     1000  exact substring in name
      800  all query tokens found as substrings (any order)
      600  acronym match (query == first letters of each word)
      300  fuzzy subsequence (query chars appear in order in name)
      +bonus for early position, token prefix hits, genre match
   ──────────────────────────────────────────────────────────────── */
static void strlower(const char *src, char *dst, int max){
    int i=0;
    for(;src[i]&&i<max-1;i++) dst[i]=(char)tolower((unsigned char)src[i]);
    dst[i]=0;
}

static int search_score(const char *name_l, const char *genre_l,
                        const char *q, int qlen){
    if(qlen==0) return 1;

    int score=0;

    /* ── 1. Exact substring in name ── */
    const char *hit=strstr(name_l,q);
    if(hit){
        score+=1000;
        /* bonus: match starts at word boundary */
        if(hit==name_l||(hit>name_l&&*(hit-1)==' ')) score+=200;
        /* bonus: earlier position */
        score+=(int)(100-(hit-name_l)*2);
        return score; /* no need to check further */
    }

    /* ── 2. Exact substring in genre ── */
    if(strstr(genre_l,q)) score+=400;

    /* ── 3. All tokens found as substrings ── */
    {
        char qcopy[128]; strncpy(qcopy,q,127); qcopy[127]=0;
        int all_found=1, tok_score=0;
        char *tok=strtok(qcopy," ");
        while(tok){
            const char *th=strstr(name_l,tok);
            if(!th){ all_found=0; break; }
            /* bonus: token starts at word boundary */
            if(th==name_l||(th>name_l&&*(th-1)==' ')) tok_score+=50;
            else tok_score+=20;
            tok=strtok(NULL," ");
        }
        if(all_found&&tok_score>0){ score+=800+tok_score; return score; }
    }

    /* ── 4. Acronym: query matches first letter of each word ── */
    {
        char initials[64]; int ic=0;
        initials[ic++]=(char)name_l[0];
        for(int i=1;name_l[i]&&ic<63;i++)
            if(name_l[i-1]==' '||name_l[i-1]==':'||name_l[i-1]=='-')
                initials[ic++]=name_l[i];
        initials[ic]=0;
        if(ic>=qlen&&strstr(initials,q)) score+=600;
    }

    /* ── 5. Fuzzy subsequence: all query chars appear in order ── */
    if(score==0){
        int qi=0;
        for(int ni=0;name_l[ni]&&qi<qlen;ni++)
            if(name_l[ni]==q[qi]) qi++;
        if(qi==qlen){
            /* score by how compact the match is (fewer skipped = better) */
            int compactness=(int)(strlen(name_l))-qlen;
            score+=300+(100-compactness<0?0:100-compactness);
        }
    }

    /* ── 6. Partial token prefix ── */
    if(score==0){
        /* at least one query token is a prefix of a word in the name */
        char qcopy[128]; strncpy(qcopy,q,127); qcopy[127]=0;
        char *tok=strtok(qcopy," ");
        while(tok){
            int tl=(int)strlen(tok);
            /* scan word starts in name */
            if(strncmp(name_l,tok,tl)==0){ score+=150; break; }
            for(int i=1;name_l[i];i++)
                if(name_l[i-1]==' '&&strncmp(name_l+i,tok,tl)==0){ score+=150; break; }
            tok=strtok(NULL," ");
        }
    }

    return score;
}

/* scored filter index */
static int  flt_score[MAX_G];

static void rebuild(void){
    nflt=0;
    char ql[128]; int qlen=(int)strlen(srch);
    strlower(srch,ql,128);

    for(int i=0;i<ndb;i++){
        if(cur_tab!=T_ALL&&!db[i].st[(int)cur_tab-1]) continue;
        if(qlen>0){
            char nl[128], gl[32];
            strlower(db[i].name, nl,128);
            strlower(db[i].genre,gl, 32);
            int s=search_score(nl,gl,ql,qlen);
            if(s==0) continue;
            flt_score[nflt]=s;
        } else {
            flt_score[nflt]=0;
        }
        flt[nflt++]=i;
    }

    /* sort by relevance score descending when query active,
       otherwise apply normal sort */
    if(qlen>0){
        /* insertion sort (stable, fast for small N after filter) */
        for(int a=1;a<nflt;a++){
            int ki=flt[a], ks=flt_score[a], b=a-1;
            while(b>=0&&flt_score[b]<ks){
                flt[b+1]=flt[b]; flt_score[b+1]=flt_score[b]; b--;
            }
            flt[b+1]=ki; flt_score[b+1]=ks;
        }
    } else {
    /* apply sort to flt[] */
    if(sort_mode==SORT_ZA){
        for(int a=0;a<nflt-1;a++) for(int b=a+1;b<nflt;b++)
            if(strcasecmp(db[flt[a]].name,db[flt[b]].name)<0){int t=flt[a];flt[a]=flt[b];flt[b]=t;}
    } else if(sort_mode==SORT_AZ){
        for(int a=0;a<nflt-1;a++) for(int b=a+1;b<nflt;b++)
            if(strcasecmp(db[flt[a]].name,db[flt[b]].name)>0){int t=flt[a];flt[a]=flt[b];flt[b]=t;}
    } else if(sort_mode==SORT_NEW){
        for(int a=0;a<nflt-1;a++) for(int b=a+1;b<nflt;b++)
            if(db[flt[a]].year<db[flt[b]].year){int t=flt[a];flt[a]=flt[b];flt[b]=t;}
    } else if(sort_mode==SORT_OLD){
        for(int a=0;a<nflt-1;a++) for(int b=a+1;b<nflt;b++)
            if(db[flt[a]].year>db[flt[b]].year){int t=flt[a];flt[a]=flt[b];flt[b]=t;}
    }
    } /* end else (no query) */
    scr_f=0.f; scr_tgt=0.f;
    clamp_page();
}
static int tcnt(int si){ int n=0; for(int i=0;i<ndb;i++) if(db[i].st[si]) n++; return n; }

/* ═══════════════════════ Save / Load ═══════════════════════════ */
static void save_d(void){
    FILE *fp=fopen(save_path,"wb"); if(!fp) return;
    Uint32 v=SAVE_VER; fwrite(&v,4,1,fp);
    fwrite(&ndb,sizeof(int),1,fp);
    for(int i=0;i<ndb;i++) fwrite(db[i].st,1,N_STATUS,fp);
    fwrite(&cur_theme,sizeof(int),1,fp);
    fwrite(&sort_mode,sizeof(int),1,fp);
    fwrite(&view_mode,sizeof(int),1,fp);
    fclose(fp);
}
static void load_d(void){
    FILE *fp=fopen(save_path,"rb"); if(!fp){ set_theme_instant(0); return; }
    Uint32 v=0; fread(&v,4,1,fp);
    if(v!=SAVE_VER){ fclose(fp); set_theme_instant(0); return; }
    int n; fread(&n,sizeof(int),1,fp);
    int lim=n<ndb?n:ndb;
    for(int i=0;i<lim;i++) fread(db[i].st,1,N_STATUS,fp);
    int t=0;
    if(fread(&t,sizeof(int),1,fp)==1 && t>=0 && t<N_THEMES)
        set_theme_instant(t);
    else
        set_theme_instant(0);
    { int sm=0; if(fread(&sm,sizeof(int),1,fp)==1&&sm>=0&&sm<4) sort_mode=(SortMode)sm; }
    { int vm=0; if(fread(&vm,sizeof(int),1,fp)==1&&vm>=0&&vm<2) view_mode=(ViewMode)vm; }
    fclose(fp);
}

/* ═══════════════════════ Scrollbar ════════════════════════════ */
static int grid_cols(void){
    int c=(LIST_W_+GRID_GAP)/(GRID_W+GRID_GAP); return c<1?1:c;
}
/* grid_cols kept for page size calc */

/* ═══════════════════════ Search spin FX ════════════════════════ */
static void draw_srch_spin_fx(void){
    if(srch_glow<0.005f) return;

    float rf   = (float)SR_R;
    float arc  = rf*(float)M_PI/2.f;
    float sx   = (float)sr_w - 2.f*rf;
    float sy   = (float)SR_H - 2.f*rf;
    float perim = 2.f*(sx+sy) + 4.f*arc;

    float x0=(float)sr_x, y0=(float)SR_Y;

    float seg_len[8] = { sx, arc, sy, arc, sx, arc, sy, arc };

    int tail = (int)(perim*0.40f);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    for(int i=0; i<=tail; i++){
        float pos = fmodf(srch_spin - (float)i + perim*200.f, perim);
        float t   = 1.f - (float)i/(float)tail;
        float a   = t*t * srch_glow;
        if(a < 0.003f) continue;

        float acc=0.f; int seg=-1; float loc=0.f;
        for(int k=0;k<8;k++){
            if(pos < acc+seg_len[k]){ seg=k; loc=pos-acc; break; }
            acc+=seg_len[k];
        }
        if(seg<0){ seg=7; loc=seg_len[7]; }

        float px_f, py_f;
        switch(seg){
        case 0: px_f = x0+rf+loc;                      py_f = y0;              break;
        case 1: { float ang = loc/rf;
            px_f = x0+sx+rf + sinf(ang)*rf;        py_f = y0+rf - cosf(ang)*rf; } break;
        case 2: px_f = x0+(float)sr_w;                 py_f = y0+rf+loc;       break;
        case 3: { float ang = loc/rf;
            px_f = x0+sx+rf + cosf(ang)*rf;        py_f = y0+sy+rf + sinf(ang)*rf; } break;
        case 4: px_f = x0+sx+rf-loc;                   py_f = y0+(float)SR_H;  break;
        case 5: { float ang = loc/rf;
            px_f = x0+rf - sinf(ang)*rf;           py_f = y0+sy+rf + cosf(ang)*rf; } break;
        case 6: px_f = x0;                             py_f = y0+sy+rf-loc;    break;
        case 7: { float ang = loc/rf;
            px_f = x0+rf - cosf(ang)*rf;           py_f = y0+rf - sinf(ang)*rf; } break;
        default: px_f=x0; py_f=y0; break;
        }
        int px=(int)(px_f+0.5f), py=(int)(py_f+0.5f);

        float cp  = fmodf(srch_spin/perim*(float)N_STATUS, (float)N_STATUS);
        int   ci  = ((int)cp) % N_STATUS;
        int   ci2 = (ci+1)   % N_STATUS;
        float cf  = cp - (float)(int)cp;
        C4 col = lerpc(SCOL[ci], SCOL[ci2], cf);

        { Uint8 ca=(Uint8)(a*255.f);
          C4 c2={col.r,col.g,col.b,ca}; sc_(c2);
          SDL_RenderDrawPoint(ren,px,py);
          if(t>0.85f){ sc_(c2);
              SDL_RenderDrawPoint(ren,px+1,py);
              SDL_RenderDrawPoint(ren,px,py+1); } }

        if(t>0.35f){
            float gt=(t-0.35f)/0.65f;
            Uint8 ga=(Uint8)(gt*gt*srch_glow*140.f);
            C4 g2={col.r,col.g,col.b,ga}; sc_(g2);
            SDL_RenderDrawPoint(ren,px-1,py);
            SDL_RenderDrawPoint(ren,px+1,py);
            SDL_RenderDrawPoint(ren,px,py-1);
            SDL_RenderDrawPoint(ren,px,py+1);
        }

        if(i==0){
            Uint8 fa=(Uint8)(srch_glow*255.f);
            C4 fl={255,255,255,fa}; sc_(fl);
            SDL_RenderDrawPoint(ren,px-1,py-1);
            SDL_RenderDrawPoint(ren,px+1,py-1);
            SDL_RenderDrawPoint(ren,px-1,py+1);
            SDL_RenderDrawPoint(ren,px+1,py+1);
            SDL_RenderDrawPoint(ren,px,py);
        }
    }

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* ═══════════════════════════════════════════════════════════════
   DRAW
   ═══════════════════════════════════════════════════════════════ */

static void draw_tbbtn(TBBtnType type, int cx2, float ht){
    int cy=TITLE_H/2, s=4;
    C4 hcol=(type==TB_CLOSE_BTN)?C_CLOSE:MK4(80,80,120,255);
    if(ht>0.02f){
        C4 bg=lerpc(MK4(0,0,0,0),hcol,ht*0.80f);
        frr(cx2-TB_BTN_W/2+4, 3, TB_BTN_W-8, TITLE_H-6, 5, bg);
    }
    C4 ic=lerpc(C_DIM,MK4(210,210,230,255),ht);
    sc_(ic);
    switch(type){
    case TB_CLOSE_BTN:
        SDL_RenderDrawLine(ren,cx2-s,cy-s,cx2+s,cy+s);
        SDL_RenderDrawLine(ren,cx2+s,cy-s,cx2-s,cy+s);
        SDL_RenderDrawLine(ren,cx2-s+1,cy-s,cx2+s+1,cy+s);
        SDL_RenderDrawLine(ren,cx2+s+1,cy-s,cx2-s+1,cy+s);
        break;
    case TB_MAX_BTN:
        if(win_maximized){
            SDL_Rect a2={cx2-s+2,cy-s,s*2-2,s*2-2};
            SDL_Rect b2={cx2-s,cy-s+2,s*2-2,s*2-2};
            SDL_RenderDrawRect(ren,&a2); SDL_RenderDrawRect(ren,&b2);
        } else {
            SDL_Rect r2={cx2-s,cy-s,s*2,s*2};
            SDL_RenderDrawRect(ren,&r2);
        }
        break;
    case TB_MIN_BTN:
        SDL_RenderDrawLine(ren,cx2-s,cy,  cx2+s,cy);
        SDL_RenderDrawLine(ren,cx2-s,cy-1,cx2+s,cy-1);
        break;
    }
}

static void draw_titlebar_dots(void);
static void draw_titlebar(void){
    fr_(0,0,win_w,TITLE_H,C_TITLE);
    {
        C4 tl=C_ACC; tl.a=120;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(tl);
        SDL_RenderDrawLine(ren,0,TITLE_H-1,win_w,TITLE_H-1);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    static const char *slbl[4]={"A \xe2\x86\x91 Z","Z \xe2\x86\x93 A","Newest","Oldest"};

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    {
        float px = TC_X0 + sort_ind_f*(float)(TC_W+TC_GAP);
        C4 pill = {C_ACC.r,C_ACC.g,C_ACC.b,200};
        frr_aa((int)(px+0.5f), TC_Y, TC_W, TC_H, 5, pill);
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    for(int i=0;i<4;i++){
        int bx=TC_X0+i*(TC_W+TC_GAP);
        float hv=tc_sort_hov[i];
        int act=(sort_mode==(SortMode)i);
        C4 lc = act ? C_TXT : lerpc(C_DIM, C_SUB, hv);
        rtxcen(f14,slbl[i],bx,TC_Y,TC_W,TC_H,lc);
    }

    {
        int sort_end = TC_X0 + 3*(TC_W+TC_GAP) + TC_W;
        int vx0_tmp  = TC_X0 + 4*(TC_W+TC_GAP) + TC_GAP + 10;
        int sx = (sort_end + vx0_tmp) / 2;
        C4 sc2=C_DIM; sc2.a=80; sc_(sc2);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(sc2);
        SDL_RenderDrawLine(ren,sx,TC_Y+2,sx,TC_Y+TC_H-2);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    {
        int vx0=TC_X0+4*(TC_W+TC_GAP)+TC_GAP+10;
        float px2 = vx0 + view_ind_f*(float)(VC_W+TC_GAP);
        C4 vpill = {C_ACC.r,C_ACC.g,C_ACC.b,200};
        frr_aa((int)(px2+0.5f), TC_Y, VC_W, TC_H, 5, vpill);
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    {
        int vx0=TC_X0+4*(TC_W+TC_GAP)+TC_GAP+10;
        for(int i=0;i<2;i++){
            int bx=vx0+i*(VC_W+TC_GAP);
            int act=(view_mode==(ViewMode)i);
            float hv=tc_view_hov[i];
            C4 ic = act ? C_TXT : lerpc(C_DIM, C_SUB, hv);
            sc_(ic);
            int cx=bx+VC_W/2, cy=TC_Y+TC_H/2;
            if(i==0){
                int lw2=16;
                for(int r2=-5;r2<=5;r2+=5){
                    SDL_RenderDrawLine(ren,cx-lw2/2,cy+r2,cx+lw2/2-1,cy+r2);
                    SDL_RenderDrawLine(ren,cx-lw2/2,cy+r2+1,cx+lw2/2-1,cy+r2+1);
                }
            } else {
                int sz=6, gap=4, half=(sz*2+gap)/2;
                SDL_Rect g1={cx-half,        cy-half,        sz,sz};
                SDL_Rect g2={cx-half+sz+gap, cy-half,        sz,sz};
                SDL_Rect g3={cx-half,        cy-half+sz+gap, sz,sz};
                SDL_Rect g4={cx-half+sz+gap, cy-half+sz+gap, sz,sz};
                SDL_RenderFillRect(ren,&g1); SDL_RenderFillRect(ren,&g2);
                SDL_RenderFillRect(ren,&g3); SDL_RenderFillRect(ren,&g4);
            }
        }
    }

    draw_titlebar_dots();
    draw_tbbtn(TB_CLOSE_BTN, TB_CX, tb_ch);
    draw_tbbtn(TB_MAX_BTN,   TB_MX, tb_mh);
    draw_tbbtn(TB_MIN_BTN,   TB_NX, tb_nh);
}

/* ── Header ───────────────────────────────────────────────────── */
static void draw_hdr(void){
    fr_(0,HDR_Y,win_w,HDR_H,C_HDR);
    rtx(f22,"GAME CATALOGUE",18,HDR_Y+(HDR_H-TTF_FontHeight(f22))/2,C_TXT);

    C4 bc=s_on?C_ACC:C_SEP;
    C4 ic=s_on?C_SRCHA:C_SRCH;
    bfrr(sr_x,SR_Y,sr_w,SR_H,SR_R,1,bc,ic);

    draw_srch_spin_fx();

    {
        int tx_off = sr_x+12;
        int tx_mw  = sr_w - 24;
        int ty     = SR_Y+(SR_H-TTF_FontHeight(f14))/2;
        int fh     = TTF_FontHeight(f14);

        if(!*srch&&!s_on){
            rtxclip(f14,"Search games...",tx_off,ty,tx_mw,C_DIM);
            rtx(f12,"[F]",sr_x+sr_w-28,SR_Y+(SR_H-TTF_FontHeight(f12))/2,C_DIM);
        } else {
            /* selection bounds (ordered) */
            int sel_a = srch_sel0 < srch_cur ? srch_sel0 : srch_cur;
            int sel_b = srch_sel0 < srch_cur ? srch_cur  : srch_sel0;
            int has_sel = (s_on && sel_a != sel_b);

            /* pixel positions in text space */
            char tmp_[128];
            strncpy(tmp_,srch,sel_a); tmp_[sel_a]=0;
            int sel_a_px = txw_(f14,tmp_) - srch_scroll;
            strncpy(tmp_,srch,sel_b); tmp_[sel_b]=0;
            int sel_b_px = txw_(f14,tmp_) - srch_scroll;
            strncpy(tmp_,srch,srch_cur); tmp_[srch_cur]=0;
            int cur_px = txw_(f14,tmp_) - srch_scroll;

            /* clip rect */
            SDL_Rect clip_r={tx_off,SR_Y,tx_mw,SR_H};
            SDL_RenderSetClipRect(ren,&clip_r);

            /* selection highlight */
            if(has_sel){
                int sx=tx_off+sel_a_px, sw=sel_b_px-sel_a_px;
                if(sx<tx_off){sw-=tx_off-sx;sx=tx_off;}
                if(sw>0&&sx<tx_off+tx_mw){
                    if(sx+sw>tx_off+tx_mw) sw=tx_off+tx_mw-sx;
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                    C4 sc2={C_ACC.r,C_ACC.g,C_ACC.b,110}; sc_(sc2);
                    SDL_Rect sr2={sx,SR_Y+3,sw,SR_H-6};
                    SDL_RenderFillRect(ren,&sr2);
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                }
            }

            /* text */
            rtx(f14,srch,tx_off-srch_scroll,ty,C_TXT);

            /* cursor */
            if(s_on && srch_blink<1.f){
                int cx=tx_off+cur_px;
                if(cx>=tx_off&&cx<=tx_off+tx_mw){
                    sc_(C_TXT);
                    SDL_RenderDrawLine(ren,cx,ty,cx,ty+fh-1);
                    SDL_RenderDrawLine(ren,cx+1,ty,cx+1,ty+fh-1);
                }
            }

            SDL_RenderSetClipRect(ren,NULL);
        }
    }

    {
        if(!dots_in_tb){
            int fh12 = TTF_FontHeight(f12);
            int dot_block = (N_THEMES-1)*TDOT_STEP + TDOT_R*2;
            int bx0 = dots_x0 - TDOT_R;

            {
                int lw2=txw_(f12,"THEME");
                rtx(f12,"THEME", bx0+(dot_block-lw2)/2,
                    dots_cy-TDOT_R-4-fh12, C_DIM);
            }

            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            {
                int   ri0 = (int)sel_ring_if;
                if(ri0 >= N_THEMES-1) ri0 = N_THEMES-2;
                float bnr = tdot_bounce[ri0];
                int   ryo = (bnr>0.f)?(int)(-sinf(bnr*(float)M_PI)*7.f):0;
                int   rx  = dots_x0+(int)(sel_ring_if*(float)TDOT_STEP+0.5f);
                C4 ac=THEMES[cur_theme].acc;
                C4 ring={ac.r,ac.g,ac.b,230};
                frr_aa(rx-TDOT_R-1,dots_cy+ryo-TDOT_R-1,(TDOT_R+1)*2,(TDOT_R+1)*2,TDOT_R+1,ring);
            }
            for(int i=0;i<N_THEMES;i++){
                int cx  = dots_x0+i*TDOT_STEP;
                float hf= tdot_hov[i];
                float bn= tdot_bounce[i];
                int yoff= (bn>0.f)?(int)(-sinf(bn*(float)M_PI)*7.f):0;
                C4 ac=THEMES[i].acc;
                if(hf>0.02f && i!=cur_theme){
                    C4 hover={ac.r,ac.g,ac.b,(Uint8)(hf*90)};
                    frr_aa(cx-TDOT_R-1,dots_cy+yoff-TDOT_R-1,(TDOT_R+1)*2,(TDOT_R+1)*2,TDOT_R+1,hover);
                }
                float br=0.45f+0.55f*hf;
                C4 fc={(Uint8)(ac.r*br),(Uint8)(ac.g*br),(Uint8)(ac.b*br),255};
                frr_aa(cx-TDOT_R,dots_cy+yoff-TDOT_R,TDOT_R*2,TDOT_R*2,TDOT_R,fc);
                if(i==cur_theme){C4 w={255,255,255,200};frr_aa(cx-3,dots_cy+yoff-3,6,6,3,w);}
            }
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

            {
                const char *nm=THEMES[cur_theme].name;
                int nw=txw_(f12,nm);
                rtx(f12,nm, bx0+(dot_block-nw)/2, dots_cy+TDOT_R+4, C_SUB);
            }
        }
    }
}

static void draw_titlebar_dots(void){
    if(!dots_in_tb) return;

    int r2=7;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    {
        int   ri0 = (int)sel_ring_if;
        if(ri0 >= N_THEMES-1) ri0 = N_THEMES-2;
        float bnr = tdot_bounce[ri0];
        int   ryo = (bnr>0.f)?(int)(-sinf(bnr*(float)M_PI)*5.f):0;
        int   rx  = dots_x0+(int)(sel_ring_if*(float)TDOT_STEP+0.5f);
        C4 ac=THEMES[cur_theme].acc;
        C4 ring={ac.r,ac.g,ac.b,220};
        frr_aa(rx-r2-1,dots_cy+ryo-r2-1,(r2+1)*2,(r2+1)*2,r2+1,ring);
    }
    for(int i=0;i<N_THEMES;i++){
        int cx=dots_x0+i*TDOT_STEP;
        float hf=tdot_hov[i];
        float bn=tdot_bounce[i];
        int yoff=(bn>0.f)?(int)(-sinf(bn*(float)M_PI)*5.f):0;
        C4 ac=THEMES[i].acc;
        if(hf>0.02f && i!=cur_theme){
            C4 hover={ac.r,ac.g,ac.b,(Uint8)(hf*90)};
            frr_aa(cx-r2-1,dots_cy+yoff-r2-1,(r2+1)*2,(r2+1)*2,r2+1,hover);
        }
        float br=0.45f+0.55f*hf;
        C4 fc={(Uint8)(ac.r*br),(Uint8)(ac.g*br),(Uint8)(ac.b*br),255};
        frr_aa(cx-r2,dots_cy+yoff-r2,r2*2,r2*2,r2,fc);
        if(i==cur_theme){C4 w={255,255,255,180};frr_aa(cx-2,dots_cy+yoff-2,4,4,2,w);}
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* ── Tabs ─────────────────────────────────────────────────────── */
static void draw_tabs(void){
    int tw=TAB_W_;
    fr_(0,TAB_Y,win_w,TAB_H,C_TBAR);

    {
        int ix=(int)tab_ix;
        C4 tab_bg = lerpc(C_BG, C_ACC, 0.18f); tab_bg.a=255;
        frr_aa(ix+2, TAB_Y+4, tw-4, TAB_H-8, 6, tab_bg);
        {
            int sx2=ix+6, sw2=tw-12, sh=5, r3=3;
            SDL_Rect clip2={sx2, TAB_Y+TAB_H-sh, sw2, sh};
            SDL_RenderSetClipRect(ren, &clip2);
            frr_aa(sx2, TAB_Y+TAB_H-sh*2, sw2, sh*2, r3, C_ACC);
            SDL_RenderSetClipRect(ren, NULL);
        }
    }

    for(int i=0;i<N_TABS;i++){
        int tx=TAB_X_(i);
        int act=(cur_tab==(TabId)i);
        char lbl[48];
        if(i==0) sprintf(lbl,"All");
        else      sprintf(lbl,"%s",SNAME[i]);
        C4 tc=(i==0)?(act?C_ACC:C_SUB):(act?SCOL[i-1]:C_SUB);
        rtxcen(f12,lbl,tx,TAB_Y,tw,TAB_H,tc);
    }
}

/* ── Row ──────────────────────────────────────────────────────── */
static void draw_row(int ri, int ay){
    int lw=LIST_W_;
    int gi=flt[ri]; Game *g=&db[gi];
    float ht=row_ht[gi];

    frr_aa(4,ay+3,lw-8,ROW_H-6,7,lerpc((ri%2==0)?C_ROWA:C_ROWB,C_ROWH,ht));

    rtxclip(f18,g->name,NM_X,ay+(ROW_H-TTF_FontHeight(f18))/2,NM_MW_,
            lerpc(C_TXT,MK4(255,255,255,255),ht*0.4f));

    char yr[8]; sprintf(yr,"%d",g->year);
    rtxcen(f12,yr,YR_X_,ay+(ROW_H-TTF_FontHeight(f12))/2,YR_W,TTF_FontHeight(f12),C_SUB);

    int bby=ay+BTN_YO;
    for(int j=0;j<N_STATUS;j++){
        int bx=BTN_LX_+j*(BTN_SZ+BTN_GAP);
        int active=g->st[j];
        float fl =btn_fl [gi][j];
        float hv =btn_hov[gi][j];
        C4 s3=SCOL[j], bg2, lc, brc;
        if(active){
            float bright=1.f+fl*0.45f+hv*0.18f;
            bg2=tintc(s3,bright);
            lc =MK4(255,255,255,255);
            brc=tintc(s3,1.6f+hv*0.3f);
        } else {
            float row_hv=ht*0.7f;
            bg2=lerpc(C_BTNI,MK4(s3.r/5,s3.g/5,s3.b/5,255),row_hv);
            bg2=lerpc(bg2,tintc(s3,0.55f),hv*0.75f);
            if(fl>0) bg2=lerpc(bg2,tintc(s3,0.4f),fl*0.35f);
            lc=lerpc(lerpc(C_DIM,tintc(s3,0.85f),row_hv),
                     MK4(255,255,255,255), hv*0.65f);
            brc=lerpc(C_SEP,tintc(s3,1.2f),hv*0.8f);
        }
        bfrr_aa(bx,bby,BTN_SZ,BTN_SZ,5,1,brc,bg2);
        rtxcen(f12,SLBL[j],bx,bby,BTN_SZ,BTN_SZ,lc);
    }
}



/* ── Grid card ────────────────────────────────────────────────── */
static void ellipsis(TTF_Font *f, const char *s, int px_max, char *buf, int bufsz){
    strncpy(buf,s,bufsz-1); buf[bufsz-1]=0;
    if(txw_(f,buf)<=px_max) return;
    const char *dots="...";
    int dl=txw_(f,dots);
    int len=(int)strlen(buf);
    while(len>0){
        buf[--len]=0;
        if(txw_(f,buf)+dl<=px_max){
            strncat(buf,dots,bufsz-len-1);
            return;
        }
    }
    strncpy(buf,dots,bufsz-1);
}

static void draw_grid_card(int ri, int x, int y){
    int gi=flt[ri]; Game *g=&db[gi];
    float ht=row_ht[gi];

    C4 bg=lerpc(C_ROWA,C_ROWH,ht*0.7f);
    frr_aa(x,y,GRID_W,GRID_H,10,bg);

    {
        int strip_h = 10;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        C4 strip={C_ACC.r,C_ACC.g,C_ACC.b,200};
        frr_aa(x, y, GRID_W, strip_h+10, 10, strip);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        C4 bg2=lerpc(C_ROWA,C_ROWH,ht*0.7f);
        fr_(x, y+strip_h, GRID_W, 10, bg2);
    }

    int tx  = x+8;
    int tw2 = GRID_W-16;
    int fh12= TTF_FontHeight(f12);
    int btn_by  = y + GRID_H - GBSZ - 4;
    int info_by = btn_by - fh12 - 5;
    int name_y  = y + 11;
    int name_h  = info_by - name_y - 4;

    {
        C4 tc = lerpc(C_TXT, MK4(255,255,255,255), ht*0.3f);
        C4 tc2= lerpc(C_SUB, C_TXT, ht*0.3f);
        const char *nm = g->name;
        int fw = txw_(f12,nm);
        if(fw <= tw2){
            int cy2 = name_y + (name_h - fh12)/2;
            rtxcen(f12,nm,x,cy2,GRID_W,fh12,tc);
        } else {
            int nlen=(int)strlen(nm);
            int split=-1;
            for(int k=nlen-1;k>0;k--){
                if(nm[k]==' '){
                    char tmp[128]; strncpy(tmp,nm,k); tmp[k]=0;
                    if(txw_(f12,tmp)<=tw2){ split=k; break; }
                }
            }
            if(split<0){
                char buf[128];
                ellipsis(f12,nm,tw2,buf,sizeof(buf));
                int cy2=name_y+(name_h-fh12)/2;
                rtxcen(f12,buf,x,cy2,GRID_W,fh12,tc);
            } else {
                char l1[128], l2[128];
                strncpy(l1,nm,split); l1[split]=0;
                strncpy(l2,nm+split+1,sizeof(l2)-1); l2[sizeof(l2)-1]=0;
                char l2e[128];
                ellipsis(f12,l2,tw2,l2e,sizeof(l2e));
                int block_h = fh12*2+2;
                int ty2 = name_y + (name_h - block_h)/2;
                if(ty2 < name_y) ty2=name_y;
                rtxcen(f12,l1, x,ty2,        GRID_W,fh12,tc);
                rtxcen(f12,l2e,x,ty2+fh12+2, GRID_W,fh12,tc);
            }
        }
    }

    {
        char info[48]; sprintf(info,"%d  %s",g->year,g->genre);
        char infoe[64]; ellipsis(f12,info,tw2,infoe,sizeof(infoe));
        rtxcen(f12,infoe,x,info_by,GRID_W,fh12,C_SUB);
    }

    {
        int total_w = N_STATUS*(GBSZ+GBGP)-GBGP;
        int bx0 = x + (GRID_W - total_w)/2;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        for(int j=0;j<N_STATUS;j++){
            int bx=bx0+j*(GBSZ+GBGP);
            int active=g->st[j];
            float hv=btn_hov[gi][j];
            float fl=btn_fl[gi][j];
            C4 s3=SCOL[j];
            C4 bg2,lc,brc;
            if(active){
                float bright=1.f+fl*0.45f+hv*0.18f;
                bg2=tintc(s3,bright);
                lc =MK4(255,255,255,255);
                brc=tintc(s3,1.6f+hv*0.3f);
            } else {
                bg2=lerpc(C_BTNI,MK4(s3.r/6,s3.g/6,s3.b/6,255),ht*0.5f);
                bg2=lerpc(bg2,tintc(s3,0.5f),hv*0.8f);
                lc =lerpc(C_DIM,MK4(255,255,255,200),hv*0.7f+ht*0.2f);
                brc=lerpc(C_SEP,tintc(s3,1.1f),hv*0.7f);
            }
            bfrr_aa(bx,btn_by,GBSZ,GBSZ,4,1,brc,bg2);
            rtxcen(f12,SLBL[j],bx,btn_by,GBSZ,GBSZ,lc);
        }
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }
}

/* ── Page bar ────────────────────────────────────────────────────── */
/* draw a filled left or right arrow triangle at (cx,cy) size ~sz */
static void draw_arrow(int cx, int cy, int sz, int right, C4 col){
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    sc_(col);
    /* scanline fill */
    for(int dy=-sz; dy<=sz; dy++){
        float t=1.f-(float)abs(dy)/(float)(sz+1);
        int hw=(int)(t*(float)sz+0.5f);
        int x0,x1;
        if(right){ x0=cx-hw; x1=cx+hw; }
        else      { x0=cx-hw; x1=cx+hw; }
        /* for right-pointing: wide at left, narrow at right */
        if(right){ x0=cx-sz; x1=cx-sz+hw*2; }
        else      { x0=cx+sz-hw*2; x1=cx+sz; }
        SDL_RenderDrawLine(ren, x0, cy+dy, x1, cy+dy);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void draw_page_bar(void){
    int tp=total_pages();
    int py=PG_BAR_Y, ph=PG_H;
    fr_(0,py,win_w,ph,C_SBAR);

    /* separator line */
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    C4 sep2={C_ACC.r,C_ACC.g,C_ACC.b,40}; sc_(sep2);
    SDL_RenderDrawLine(ren,0,py,win_w,py);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    if(tp<=1) return;

    int cy=py+ph/2;

    /* ── Prev / Next buttons ── */
    int bw=80, bh=34, br=10;
    int prev_x=14, next_x=win_w-14-bw;
    int prev_active=(cur_page>0);
    int next_active=(cur_page<tp-1);

    /* helper: draw a bold chevron arrow  (dir: 0=left, 1=right) */
    #define DRAW_CHEVRON(cx2,cy2,sz2,dir2,col2) do { \
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND); \
        sc_(col2); \
        for(int _t=0;_t<2;_t++){ \
            int _cx=(cx2)+_t*(dir2?1:-1); \
            for(int _dy=-(sz2);_dy<=(sz2);_dy++){ \
                int _dx=(int)((float)(sz2)-fabsf((float)_dy)+0.5f); \
                int _x0=(dir2)?_cx-_dx:_cx+_dx-(dir2?0:0); \
                SDL_RenderDrawPoint(ren,(dir2)?_cx+_dx:_cx-_dx,cy2+_dy); \
            } \
        } \
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); \
    } while(0)

    /* prev button */
    {
        float hv=pg_prev_hov;
        /* outer accent glow on hover */
        if(prev_active && hv>0.02f){
            C4 glow={C_ACC.r,C_ACC.g,C_ACC.b,(Uint8)(hv*60)};
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            frr_aa(prev_x-3,cy-bh/2-3,bw+6,bh+6,br+3,glow);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }
        /* border: 1px idle, 2px on hover, lights up with accent */
        int bt=prev_active?(hv>0.5f?2:1):1;
        C4 bc=prev_active?lerpc(C_SEP,C_ACC,hv*0.9f):C_SEP;
        C4 ic=prev_active?lerpc(C_SRCH,C_SRCHA,hv*0.55f):C_BTNI;
        ic.a=255;
        bfrr_aa(prev_x,cy-bh/2,bw,bh,br,bt,bc,ic);
        /* arrow + label */
        C4 lc=prev_active?lerpc(C_SUB,C_TXT,hv):C_DIM;
        C4 ac2=prev_active?lerpc(C_SUB,C_ACC,hv):C_DIM;
        int fh14=TTF_FontHeight(f14);
        int lw2=txw_(f14,"Prev");
        int content_w=12+6+lw2;  /* arrow(12) + gap(6) + text */
        int cx2=prev_x+(bw-content_w)/2;
        /* draw bigger chevron */
        draw_arrow(cx2+5,cy,5,0,ac2);
        rtx(f14,"Prev",cx2+12+6,cy-fh14/2,lc);
    }
    /* next button */
    {
        float hv=pg_next_hov;
        if(next_active && hv>0.02f){
            C4 glow={C_ACC.r,C_ACC.g,C_ACC.b,(Uint8)(hv*60)};
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            frr_aa(next_x-3,cy-bh/2-3,bw+6,bh+6,br+3,glow);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }
        int bt=next_active?(hv>0.5f?2:1):1;
        C4 bc=next_active?lerpc(C_SEP,C_ACC,hv*0.9f):C_SEP;
        C4 ic=next_active?lerpc(C_SRCH,C_SRCHA,hv*0.55f):C_BTNI;
        ic.a=255;
        bfrr_aa(next_x,cy-bh/2,bw,bh,br,bt,bc,ic);
        C4 lc=next_active?lerpc(C_SUB,C_TXT,hv):C_DIM;
        C4 ac2=next_active?lerpc(C_SUB,C_ACC,hv):C_DIM;
        int fh14=TTF_FontHeight(f14);
        int lw2=txw_(f14,"Next");
        int content_w=lw2+6+12;
        int cx2=next_x+(bw-content_w)/2;
        rtx(f14,"Next",cx2,cy-fh14/2,lc);
        draw_arrow(cx2+lw2+6+5,cy,5,1,ac2);
    }
    #undef DRAW_CHEVRON

    /* ── Page counter centered between the buttons ── */
    char pg_lbl[32]; sprintf(pg_lbl,"%d / %d",cur_page+1,tp);
    int lw3=txw_(f14,pg_lbl);
    int label_x=(win_w-lw3)/2;
    int label_y=py+ph-TTF_FontHeight(f14)-6;
    rtx(f14,pg_lbl,label_x,label_y,C_DIM);

    /* ── Page indicator dots ── */
    #define MAX_DOTS 13
    int show=tp<MAX_DOTS?tp:MAX_DOTS;
    int dot_r=6, dot_gap=18;
    int total_dot_w=show*dot_gap-dot_gap+dot_r*2;
    int dot_x0=(win_w-total_dot_w)/2;
    int dot_cy=py+(ph-TTF_FontHeight(f14)-6)/2;  /* vertically centred in upper portion */

    float half_f=(float)(MAX_DOTS/2);
    float ws_f=page_slide-half_f;
    if(ws_f<0.f) ws_f=0.f;
    if(ws_f>(float)(tp-show)) ws_f=(float)(tp-show);
    if(ws_f<0.f) ws_f=0.f;

    SDL_Rect dot_clip={prev_x+bw+8, py, win_w-2*(prev_x+bw+8), ph};
    SDL_RenderSetClipRect(ren, &dot_clip);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);

    for(int i=0;i<show+2;i++){
        float pg_f=ws_f+(float)i;
        int pg=(int)(pg_f+0.5f);
        if(pg<0||pg>=tp) continue;

        float local_f=(float)pg-ws_f;
        float fcx=(float)dot_x0+local_f*(float)dot_gap+(float)dot_r;
        int cx=(int)(fcx+0.5f);

        float edge_fade=1.f;
        int left_edge =(ws_f > 0.2f);
        int right_edge=(ws_f < (float)(tp-show)-0.2f);
        if(left_edge  && local_f<0.8f)                 edge_fade=local_f/0.8f;
        if(right_edge && local_f>(float)(show-1)-0.8f) edge_fade=((float)(show-1)-local_f+0.8f)/0.8f;
        if(edge_fade<0.f) edge_fade=0.f;
        if(edge_fade>1.f) edge_fade=1.f;

        int is_cur=(pg==cur_page);
        float hv=(pg<64)?pg_num_hov[pg]:0.f;

        float dist=fabsf(page_slide-(float)pg);
        float pulse=(dist<1.f)?(1.f-dist):0.f;
        float sz=(float)dot_r+pulse*(float)dot_r*0.35f;
        int sr=(int)(sz+0.5f);

        C4 ac=C_ACC;
        if(is_cur){
            C4 fc={ac.r,ac.g,ac.b,(Uint8)(230*edge_fade)};
            frr_aa(cx-sr,dot_cy-sr,sr*2,sr*2,sr,fc);
            int wr=sr/3; if(wr<1) wr=1;
            C4 wc={255,255,255,(Uint8)(180*edge_fade*pulse)};
            frr_aa(cx-wr,dot_cy-wr,wr*2,wr*2,wr,wc);
        } else {
            float br2=0.25f+0.45f*hv;
            C4 fc={(Uint8)(ac.r*br2),(Uint8)(ac.g*br2),(Uint8)(ac.b*br2),
                   (Uint8)((70+hv*90)*edge_fade)};
            frr_aa(cx-dot_r,dot_cy-dot_r,dot_r*2,dot_r*2,dot_r,fc);
        }
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren,NULL);
    #undef MAX_DOTS
}

/* ── Grid view ─────────────────────────────────────────────────── */
static void draw_grid(void){
    int lh=LST_H_;
    SDL_Rect clip={0,LST_Y,win_w,lh};
    SDL_RenderSetClipRect(ren,&clip);
    fr_(0,LST_Y,LIST_W_,lh,C_BG);

    int cols=grid_cols();
    int block_w=cols*(GRID_W+GRID_GAP)-GRID_GAP;
    int ox=(LIST_W_-block_w)/2;

    int first=page_first(), last=page_last();
    int pg_count=last-first+1;
    /* centre based on rows actually being drawn, not theoretical max rows */
    int actual_rows=(pg_count+cols-1)/cols; if(actual_rows<1) actual_rows=1;
    int used_h=actual_rows*(GRID_H+GRID_GAP)-GRID_GAP;
    int oy=LST_Y+(lh-used_h)/2; if(oy<LST_Y) oy=LST_Y;

    int row_h=GRID_H+GRID_GAP;
    for(int ri=first;ri<=last;ri++){
        int local=ri-first;
        int row=local/cols, col=local%cols;
        int cx=ox+col*(GRID_W+GRID_GAP);
        int ay=oy+row*row_h;
        draw_grid_card(ri,cx,ay);
    }
    if(nflt==0){
        rtxcen(f18,"No games found.",0,LST_Y,LIST_W_,lh*2/3,C_SUB);
        rtxcen(f14,"Try a different search or tab.",0,LST_Y+lh*2/3,LIST_W_,lh/3,C_DIM);
    }
    SDL_RenderSetClipRect(ren,NULL);
    draw_page_bar();
}

/* ── List ─────────────────────────────────────────────────────── */
static void draw_list(void){
    if(view_mode==VIEW_GRID){ draw_grid(); return; }
    int lh=LST_H_;
    SDL_Rect clip={0,LST_Y,win_w,lh};
    SDL_RenderSetClipRect(ren,&clip);
    fr_(0,LST_Y,LIST_W_,lh,C_BG);
    int first=page_first(), last=page_last();
    for(int r=first;r<=last;r++)
        draw_row(r, LST_Y+LIST_TOP_PAD+(r-first)*ROW_H);
    if(nflt==0){
        rtxcen(f18,"No games found.",0,LST_Y,LIST_W_,lh*2/3,C_SUB);
        rtxcen(f14,"Try a different search or tab.",0,LST_Y+lh*2/3,LIST_W_,lh/3,C_DIM);
    }
    SDL_RenderSetClipRect(ren,NULL);
    draw_page_bar();
}

/* ── Status bar ───────────────────────────────────────────────── */
static void draw_sbar(void){
    fr_(0,win_h-SB_H,win_w,SB_H,C_SBAR);
    /* top separator */
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    C4 sep3={C_ACC.r,C_ACC.g,C_ACC.b,25}; sc_(sep3);
    SDL_RenderDrawLine(ren,0,win_h-SB_H,win_w,win_h-SB_H);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    int fh=TTF_FontHeight(f12);
    int y=win_h-SB_H+(SB_H-fh)/2;

    /* "nflt games" on the left */
    char left_lbl[32];
    if(nflt==ndb) sprintf(left_lbl,"%d games",ndb);
    else          sprintf(left_lbl,"%d of %d",nflt,ndb);
    rtx(f12,left_lbl,10,y,C_SUB);

    /* status counts on the right — coloured dot + number */
    int rx=win_w-10;
    for(int i=N_STATUS-1;i>=0;i--){
        int cnt=tcnt(i);
        if(cnt==0) continue;
        char b[12]; sprintf(b,"%d",cnt);
        int tw=txw_(f12,b);
        rx-=tw;
        rtx(f12,b,rx,y,C_DIM);
        rx-=4;
        /* colour dot */
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        frr_aa(rx-7,y+(fh-6)/2,6,6,3,SCOL[i]);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        rx-=12;
    }
}

/* ═══════════════════════ Helpers ═══════════════════════════════ */
static void do_tab(int i){
    cur_tab=(TabId)i; scr_tgt=0; scr_f=0; cur_page=0;
    tab_itx=(float)TAB_X_(i);
    rebuild();
}
static int hit_tab(int mx,int my){
    if(my<TAB_Y||my>=TAB_Y+TAB_H) return -1;
    int tw=TAB_W_;
    for(int i=0;i<N_TABS;i++){ int tx=TAB_X_(i); if(mx>=tx&&mx<tx+tw) return i; }
    return -1;
}
static int row_at(int mx,int my){
    if(mx>=LIST_W_||my<LST_Y||my>=LST_Y+LST_H_) return -1;
    int r=(my-LST_Y-LIST_TOP_PAD)/ROW_H+page_first();
    return (r>=page_first()&&r<=page_last())?r:-1;
}
static int btn_at(int mx,int my,int ri){
    int ry=LST_Y+LIST_TOP_PAD+(ri-page_first())*ROW_H, by2=ry+BTN_YO;
    if(my<by2||my>=by2+BTN_SZ) return -1;
    for(int j=0;j<N_STATUS;j++){
        int bx=BTN_LX_+j*(BTN_SZ+BTN_GAP);
        if(mx>=bx&&mx<bx+BTN_SZ) return j;
    }
    return -1;
}

/* ═══════════════════════ Font loader ══════════════════════════ */
static TTF_Font* load_font(int sz){
    static const char *p[]={
        "font.ttf","DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf","C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",NULL
    };
    for(int i=0;p[i];i++){ TTF_Font *f=TTF_OpenFont(p[i],sz); if(f) return f; }
    return NULL;
}

/* ═══════════════════════ DB sort / init ═══════════════════════ */
static int cmp_game(const void *a,const void *b){
    return strcasecmp(((const Game*)a)->name,((const Game*)b)->name);
}
static void init_db(void){
    int lim=N_GDB<MAX_G?N_GDB:MAX_G;
    for(int i=0;i<lim;i++){
        strncpy(db[i].name, GDB[i].n,127); db[i].name[127]=0;
        strncpy(db[i].genre,GDB[i].g, 31); db[i].genre[31]=0;
        db[i].year=GDB[i].y; memset(db[i].st,0,N_STATUS);
    }
    ndb=lim;
    qsort(db,ndb,sizeof(Game),cmp_game);
    memset(row_ht,0,sizeof(row_ht));
    memset(btn_fl,0,sizeof(btn_fl));
}

/* ═══════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════ */
int main(int argc,char **argv){
    (void)argc;(void)argv;

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,"1");

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0){ fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 1; }
    if(TTF_Init()<0){               fprintf(stderr,"TTF: %s\n",TTF_GetError()); return 1; }

    win=SDL_CreateWindow("Game Catalogue",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        win_w,win_h,
        SDL_WINDOW_SHOWN|SDL_WINDOW_BORDERLESS|SDL_WINDOW_RESIZABLE);
    if(!win) return 1;
    SDL_SetWindowMinimumSize(win,MIN_W,MIN_H);
    apply_rgn(win_w,win_h,1);

    ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|0x00000004);
    if(!ren) return 1;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);

    {
        SDL_AudioSpec want={0};
        want.freq=SFX_RATE; want.format=AUDIO_S16SYS;
        want.channels=1; want.samples=512; want.callback=NULL;
        SDL_AudioSpec got;
        aud_dev=SDL_OpenAudioDevice(NULL,0,&want,&got,0);
        if(aud_dev) SDL_PauseAudioDevice(aud_dev,0);
    }

    f22=load_font(22); f18=load_font(18);
    f14=load_font(14); f12=load_font(12);
    if(!f22||!f18||!f14||!f12){
        fprintf(stderr,"ERROR: No font found. Place DejaVuSans.ttf next to the exe.\n");
        return 1;
    }

    cur_arr  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    cur_ns   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    cur_ew   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    cur_nwse = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    cur_nesw = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);

    init_save_path(); init_db(); load_d(); rebuild();

    /* Snap pill indicators to loaded state */
    sort_ind_f=(float)sort_mode;
    view_ind_f=(float)view_mode;

    memset(tdot_hov,0,sizeof(tdot_hov));
    memset(tdot_bounce,0,sizeof(tdot_bounce));
    memset(btn_hov,0,sizeof(btn_hov));
    memset(tc_sort_hov,0,sizeof(tc_sort_hov));
    memset(tc_view_hov,0,sizeof(tc_view_hov));
    compute_dot_layout();
    sel_ring_if=(float)cur_theme;
    tab_ix=tab_itx=8.f;
    plast=SDL_GetPerformanceCounter();
    SDL_StartTextInput();

    int run=1; SDL_Event ev;
    int needs_redraw = 1;
    while(run){
        /* If nothing is animating, block until next event (saves CPU).
           Use a 50ms timeout so blink/spin still wake up when active. */
        if(!needs_redraw){
            SDL_WaitEventTimeout(&ev, 16);
        }

        needs_redraw = anim_tick();

        if(!rz_drag&&!win_drag&&!drag_sb){
            int mx,my; SDL_GetMouseState(&mx,&my);
            SDL_SetCursor(cur_for(get_rz(mx,my)));
        }

        while(SDL_PollEvent(&ev)){
            needs_redraw = 1;
            switch(ev.type){
            case SDL_QUIT: run=0; break;

            case SDL_WINDOWEVENT:
                if(ev.window.event==SDL_WINDOWEVENT_RESIZED||
                   ev.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){
                    SDL_GetWindowSize(win,&win_w,&win_h);
                    tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);
                    apply_rgn(win_w,win_h,!win_maximized);
                    rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_MAXIMIZED){
                    win_maximized=1; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,0);
                    tab_ix=tab_itx=(float)TAB_X_((int)cur_tab); rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_RESTORED){
                    win_maximized=0; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,1);
                    tab_ix=tab_itx=(float)TAB_X_((int)cur_tab); rebuild();
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if(ev.button.button==SDL_BUTTON_LEFT){
                    int mx=ev.button.x, my=ev.button.y;

                        if(my<TITLE_H){
                            if(IN_BTN(mx,TB_CX)){ sfx_click(); run=0; break; }
                            if(IN_BTN(mx,TB_MX)){ sfx_click(); toggle_maximize(); break; }
                            if(IN_BTN(mx,TB_NX)){ sfx_click(); SDL_MinimizeWindow(win); break; }
                            int hit_tb_btn=0;
                            if(my>=TC_Y&&my<TC_Y+TC_H){
                                for(int i=0;i<4;i++){
                                    int bx=TC_X0+i*(TC_W+TC_GAP);
                                    if(mx>=bx&&mx<bx+TC_W){
                                        hit_tb_btn=1;
                                        if(sort_mode!=(SortMode)i){ sort_mode=(SortMode)i; sfx_sort(); rebuild(); save_d(); }
                                        break;
                                    }
                                }
                                for(int i=0;i<2;i++){
                                    int bx=TC_X0+4*(TC_W+TC_GAP)+TC_GAP+10+i*(VC_W+TC_GAP);
                                    if(mx>=bx&&mx<bx+VC_W){
                                        hit_tb_btn=1;
                                        if(view_mode!=(ViewMode)i){
                                            view_mode=(ViewMode)i; sfx_tab();
                                            scr_tgt=0; scr_f=0; save_d();
                                        }
                                        break;
                                    }
                                }
                            }
                        if(dots_in_tb){
                            int td=hit_theme_dot(mx,my);
                            if(td>=0){
                                hit_tb_btn=1;
                                tdot_bounce[td]=1.f;
                                if(td!=cur_theme){set_theme(td);save_d();sfx_tab();}
                                break;
                            }
                        }
                        RzDir rd=get_rz(mx,my);
                        if(rd!=RZ_NONE){
                            rz_drag=1; rz_active=rd;
                            SDL_GetGlobalMouseState(&rz_gx0,&rz_gy0);
                            SDL_GetWindowPosition(win,&rz_wx0,&rz_wy0);
                            rz_ww0=win_w; rz_wh0=win_h; break;
                        }
                        if(!win_maximized&&!hit_tb_btn){ win_drag=1; win_drag_ox=mx; win_drag_oy=my; }
                        break;
                    }

                    {
                        RzDir rd=get_rz(mx,my);
                        if(rd!=RZ_NONE){
                            rz_drag=1; rz_active=rd;
                            SDL_GetGlobalMouseState(&rz_gx0,&rz_gy0);
                            SDL_GetWindowPosition(win,&rz_wx0,&rz_wy0);
                            rz_ww0=win_w; rz_wh0=win_h; break;
                        }
                    }

                    { int t=hit_tab(mx,my); if(t>=0){ if(t!=(int)cur_tab) sfx_tab(); do_tab(t); break; } }

                    if(mx>=sr_x&&mx<sr_x+sr_w&&my>=SR_Y&&my<SR_Y+SR_H){
                        SDL_Keymod km=SDL_GetModState();
                        int shift2=(km&KMOD_SHIFT)!=0;
                        if(!s_on){ sfx_click(); srch_focus(); }
                        /* map click x to char pos */
                        int tx_off_=sr_x+12;
                        int click_px=(mx-tx_off_)+srch_scroll;
                        int cpos=srch_px_to_pos(click_px);
                        /* double-click: select word */
                        Uint32 now_ms=SDL_GetTicks();
                        if(!shift2 && srch_dbl_p==cpos && now_ms-srch_dbl_t<400){
                            /* double-click: select word */
                            srch_sel0=srch_word_left(cpos);
                            srch_cur =srch_word_right(cpos);
                            srch_dbl_p=-1;
                        } else {
                            if(shift2){ srch_cur=cpos; }
                            else { srch_cur=cpos; srch_sel0=cpos; }
                            srch_dbl_t=now_ms; srch_dbl_p=cpos;
                        }
                        srch_blink=0.f;
                        srch_drag=1;
                        srch_ensure_visible();
                        break;
                    } else { srch_blur(); }

                    if(!dots_in_tb&&my>=HDR_Y&&my<HDR_Y+HDR_H){
                        int td=hit_theme_dot(mx,my);
                        if(td>=0){
                            tdot_bounce[td]=1.f;
                            if(td!=cur_theme){set_theme(td);save_d();sfx_tab();}
                            break;
                        }
                    }

                    /* page bar clicks */
                    if(my>=PG_BAR_Y&&my<PG_BAR_Y+PG_H){
                        int py=PG_BAR_Y,ph=PG_H,bw=80,bh=34;
                        int prev_x2=14, next_x2=win_w-14-bw;
                        int cy2=py+ph/2;
                        if(mx>=prev_x2&&mx<prev_x2+bw&&abs(my-cy2)<=bh/2){ int p=cur_page; go_page(cur_page-1); if(cur_page!=p) sfx_tab(); }
                        else if(mx>=next_x2&&mx<next_x2+bw&&abs(my-cy2)<=bh/2){ int p=cur_page; go_page(cur_page+1); if(cur_page!=p) sfx_tab(); }
                        else {
                            /* dot click — use same ws_f formula as draw */
                            int tp2=total_pages();
                            #define MD3 13
                            int show=tp2<MD3?tp2:MD3;
                            int dot_r=6,dot_gap=18;
                            int total_dot_w=show*dot_gap-dot_gap+dot_r*2;
                            int dot_x0=(win_w-total_dot_w)/2;
                            int dot_cy2=py+(ph-TTF_FontHeight(f14)-6)/2;
                            float ws_fc=page_slide-(float)(MD3/2);
                            if(ws_fc<0.f) ws_fc=0.f;
                            if(ws_fc>(float)(tp2-show)) ws_fc=(float)(tp2-show);
                            if(ws_fc<0.f) ws_fc=0.f;
                            for(int i=0;i<show+2;i++){
                                int pg2=(int)(ws_fc+(float)i+0.5f);
                                if(pg2<0||pg2>=tp2) continue;
                                float lf=(float)pg2-ws_fc;
                                int cx2=(int)((float)dot_x0+lf*(float)dot_gap+(float)dot_r+0.5f);
                                if(abs(mx-cx2)<=dot_r+4&&abs(my-dot_cy2)<=dot_r+4){
                                    go_page(pg2); sfx_tab(); break;
                                }
                            }
                            #undef MD3
                        }
                        break;
                    }

                    if(view_mode==VIEW_LIST){
                        int r=row_at(mx,my);
                        if(r>=0){
                            int b=btn_at(mx,my,r);
                            if(b>=0){ int gi=flt[r]; db[gi].st[b]^=1; sfx_toggle(); btn_fl[gi][b]=1; rebuild(); save_d(); }
                        }
                    } else {
                        if(hov_db>=0){
                            int cols=grid_cols();
                            int block_w=cols*(GRID_W+GRID_GAP)-GRID_GAP;
                            int ox=(LIST_W_-block_w)/2;
                            int pg_cnt4=page_last()-page_first()+1;
                            int rows_pg2=(pg_cnt4+cols-1)/cols; if(rows_pg2<1) rows_pg2=1;
                            int used_h3=rows_pg2*(GRID_H+GRID_GAP)-GRID_GAP;
                            int oy3=LST_Y+(LST_H_-used_h3)/2; if(oy3<LST_Y) oy3=LST_Y;
                            for(int ri2=page_first();ri2<=page_last();ri2++){
                                if(flt[ri2]!=hov_db) continue;
                                int local2=ri2-page_first();
                                int row2=local2/cols, col2=local2%cols;
                                int cx2=ox+col2*(GRID_W+GRID_GAP);
                                int cy2=oy3+row2*(GRID_H+GRID_GAP);
                                int total_w2=N_STATUS*(GBSZ+GBGP)-GBGP;
                                int bx02=cx2+(GRID_W-total_w2)/2;
                                int by2=cy2+GRID_H-GBSZ-4;
                                if(my>=by2&&my<by2+GBSZ){
                                    for(int j=0;j<N_STATUS;j++){
                                        int bx=bx02+j*(GBSZ+GBGP);
                                        if(mx>=bx&&mx<bx+GBSZ){
                                            db[hov_db].st[j]^=1; sfx_toggle();
                                            btn_fl[hov_db][j]=1; rebuild(); save_d();
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                if(ev.button.button==SDL_BUTTON_LEFT&&ev.button.clicks==2
                   &&ev.button.y<TITLE_H&&ev.button.x<win_w-3*TB_BTN_W){
                    toggle_maximize();
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if(ev.button.button==SDL_BUTTON_LEFT){
                    if(rz_drag){ SDL_GetWindowSize(win,&win_w,&win_h); apply_rgn(win_w,win_h,!win_maximized); }
                    win_drag=0; rz_drag=0; rz_active=RZ_NONE; drag_sb=0;
                    srch_drag=0;
                    SDL_SetCursor(cur_arr);
                }
                break;

            case SDL_MOUSEMOTION:{
                int mx=ev.motion.x, my=ev.motion.y;
                /* If left button was released outside the window, cancel drag/resize */
                if((win_drag||rz_drag||drag_sb||srch_drag) &&
                   !(SDL_GetGlobalMouseState(NULL,NULL) & SDL_BUTTON_LMASK)){
                    if(rz_drag){ SDL_GetWindowSize(win,&win_w,&win_h); apply_rgn(win_w,win_h,!win_maximized); }
                    win_drag=0; rz_drag=0; rz_active=RZ_NONE; drag_sb=0; srch_drag=0;
                    SDL_SetCursor(cur_arr);
                }
                hov_db=-1;
                int r=row_at(mx,my); if(r>=0) hov_db=flt[r];
                hsb=(mx>=LIST_W_&&mx<win_w&&my>=LST_Y&&my<LST_Y+LST_H_);
                if(win_drag){
                    int gx,gy; SDL_GetGlobalMouseState(&gx,&gy);
                    SDL_SetWindowPosition(win,gx-win_drag_ox,gy-win_drag_oy);
                }
                if(rz_drag) do_rz_move();
                /* drag_sb removed (pagination) */
                if(srch_drag && s_on){
                    int tx_off_=sr_x+12;
                    int drag_px=(mx-tx_off_)+srch_scroll;
                    srch_cur=srch_px_to_pos(drag_px);
                    srch_blink=0.f;
                    srch_ensure_visible();
                }
                break;
            }

            case SDL_MOUSEWHEEL:
                if(ev.wheel.y){
                    go_page(cur_page-(ev.wheel.y>0?1:-1));
                }
                break;

            case SDL_KEYDOWN:{
                SDL_Keymod km=SDL_GetModState();
                int ctrl =(km&KMOD_CTRL) !=0;
                int shift=(km&KMOD_SHIFT)!=0;
                SDL_Keycode sym=ev.key.keysym.sym;
                if(s_on){
                    int len=(int)strlen(srch);
                    if(sym==SDLK_ESCAPE){
                        sfx_click(); srch[0]=0; srch_cur=0; srch_sel0=0;
                        srch_scroll=0; s_on=0; scr_tgt=0; scr_f=0; rebuild();
                    } else if(sym==SDLK_RETURN||sym==SDLK_KP_ENTER){
                        sfx_click(); srch_blur();
                    } else if(sym==SDLK_BACKSPACE){
                        if(!srch_delete_sel()){
                            if(ctrl){
                                /* Ctrl+Backspace: delete word left */
                                int np=srch_word_left(srch_cur);
                                memmove(srch+np,srch+srch_cur,len-srch_cur+1);
                                srch_cur=np; srch_sel0=np;
                            } else if(srch_cur>0){
                                memmove(srch+srch_cur-1,srch+srch_cur,len-srch_cur+1);
                                srch_cur--; srch_sel0=srch_cur;
                            }
                        }
                        sfx_type(); srch_blink=0.f; srch_ensure_visible();
                        scr_tgt=0; scr_f=0; rebuild();
                    } else if(sym==SDLK_DELETE){
                        if(!srch_delete_sel()){
                            if(srch_cur<len){
                                memmove(srch+srch_cur,srch+srch_cur+1,len-srch_cur);
                            }
                        }
                        sfx_type(); srch_blink=0.f; srch_ensure_visible();
                        scr_tgt=0; scr_f=0; rebuild();
                    } else if(sym==SDLK_LEFT){
                        if(!shift && srch_sel0!=srch_cur){
                            /* collapse selection to left end */
                            int a=srch_sel0<srch_cur?srch_sel0:srch_cur;
                            srch_move(a,0);
                        } else {
                            int np=ctrl?srch_word_left(srch_cur):srch_cur>0?srch_cur-1:0;
                            srch_move(np,shift);
                        }
                    } else if(sym==SDLK_RIGHT){
                        if(!shift && srch_sel0!=srch_cur){
                            /* collapse selection to right end */
                            int b=srch_sel0>srch_cur?srch_sel0:srch_cur;
                            srch_move(b,0);
                        } else {
                            int np=ctrl?srch_word_right(srch_cur):srch_cur<len?srch_cur+1:len;
                            srch_move(np,shift);
                        }
                    } else if(sym==SDLK_HOME){
                        srch_move(0,shift);
                    } else if(sym==SDLK_END){
                        srch_move(len,shift);
                    } else if(ctrl&&sym==SDLK_a){
                        srch_sel0=0; srch_cur=len; srch_blink=0.f;
                        srch_ensure_visible();
                    } else if(ctrl&&sym==SDLK_c){
                        srch_copy();
                    } else if(ctrl&&sym==SDLK_x){
                        srch_copy(); srch_delete_sel();
                        sfx_type(); scr_tgt=0; scr_f=0; rebuild();
                    } else if(ctrl&&sym==SDLK_v){
                        char *clip=SDL_GetClipboardText();
                        if(clip&&*clip){ srch_insert(clip); sfx_type(); scr_tgt=0; scr_f=0; rebuild(); }
                        if(clip) SDL_free(clip);
                    }
                } else {
                    if(sym==SDLK_ESCAPE){ sfx_click(); srch[0]=0; do_tab(0); }
                    if(sym==SDLK_LEFT||sym==SDLK_PAGEUP)  { int p=cur_page; go_page(cur_page-1); if(cur_page!=p) sfx_tab(); }
                    if(sym==SDLK_RIGHT||sym==SDLK_PAGEDOWN){ int p=cur_page; go_page(cur_page+1); if(cur_page!=p) sfx_tab(); }
                    if(sym==SDLK_f){ sfx_click(); srch_focus();
                        /* Ctrl+F: select all existing text */
                        if(ctrl){ srch_sel0=0; srch_cur=(int)strlen(srch); }
                    }
                    if(sym==SDLK_LEFTBRACKET){
                        int nt=(cur_theme-1+N_THEMES)%N_THEMES;
                        set_theme(nt); save_d(); sfx_tab();
                    }
                    if(sym==SDLK_RIGHTBRACKET){
                        int nt=(cur_theme+1)%N_THEMES;
                        set_theme(nt); save_d(); sfx_tab();
                    }
                }
                break;
            }

            case SDL_TEXTINPUT:
                if(s_on){
                    srch_insert(ev.text.text);
                    sfx_type(); scr_tgt=0; scr_f=0; rebuild();
                }
                break;
            }
        }

        if(needs_redraw || rz_drag || win_drag || drag_sb || srch_drag){
            sc_(C_BG); SDL_RenderClear(ren);
            draw_titlebar();
            draw_hdr();
            draw_tabs();
            draw_list();
            draw_sbar();
            SDL_RenderPresent(ren);
        }
    }

    save_d();
    if(aud_dev) SDL_CloseAudioDevice(aud_dev);
    TTF_CloseFont(f22); TTF_CloseFont(f18);
    TTF_CloseFont(f14); TTF_CloseFont(f12);
    TTF_Quit();
    SDL_FreeCursor(cur_arr); SDL_FreeCursor(cur_ns); SDL_FreeCursor(cur_ew);
    SDL_FreeCursor(cur_nwse); SDL_FreeCursor(cur_nesw);
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}