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
 *    gcc -O2 -o gamelist.exe main.c themes.c games.c   \
 *        -I"C:/SDL2/include" -I"C:/SDL2/include/SDL2"  \
 *        -L"C:/SDL2/lib"                               \
 *        -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf        \
 *        -mwindows
 *
 *  BUILD (Linux/macOS):
 *    gcc -O2 -o gamelist main.c themes.c games.c       \
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
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "games.h"
#include "themes.h"
#include "state.h"
#include "save.h"
#include "audio.h"
#include "search.h"

#ifdef _WIN32
  #include <SDL2/SDL_syswm.h>
  #include <windows.h>
  #define WIN_RGN_R 8
#endif

/* ── Window state ─────────────────────────────────────────────── */
int win_w = 1280;
int win_h = 860;
#define MIN_W 700
#define MIN_H 480

/* ── Layout ───────────────────────────────────────────────────── */
#define CHIP_H   26                            /* filter-chip row height        */
/* chip_band is 0 (no chips) or CHIP_H — declared in globals below */
#define TAB_Y   (TITLE_H + HDR_H + chip_band)
#define TAB_H    44
#define SB_H     26
#define PG_BAR_Y (win_h - SB_H - PG_H)
#define LST_Y   (TITLE_H + HDR_H + chip_band + TAB_H)
#define COL_HDR_H  28                          /* sticky column header height   */
#define LST_DATA_Y (LST_Y + COL_HDR_H)        /* Y where list rows begin       */
#define LST_H_  (win_h - LST_Y - SB_H - PG_H)
#define ROW_H    66
#define SCR_W    0
#define LIST_W_ (win_w)
#define PG_H     54   /* page bar height replaces scrollbar */
#define RESIZE_B  7

/* ── Status (Backlog removed) ─────────────────────────────────── */

/* ── Status badge (replaces segmented strip in list view) ─────────── */
#define BADGE_W   96   /* pill width: icon + label                     */
#define BADGE_H   26   /* same height as old strip                      */
#define BADGE_X_  (LIST_W_ - 12 - BADGE_W)
#define BADGE_YO  ((ROW_H - BADGE_H) / 2)
/* BTN_YO kept as alias — note button still vertically centres on this  */
#define BTN_SZ    BADGE_H
#define BTN_YO    BADGE_YO

/* ── Status-badge popup ────────────────────────────────────────────── */
#define BPOP_W        130
#define BPOP_ITEM_H    28
#define BPOP_H        (N_STATUS * BPOP_ITEM_H + 8)

/* ── Columns ──────────────────────────────────────────────────── */
#define YR_W     50
#define NOTE_BTN_SZ  BTN_SZ                               /* same height as badge             */
#define NOTE_BTN_GAP 10                                   /* gap between note btn and badge   */
#define NOTE_BTN_X_  (BADGE_X_ - NOTE_BTN_SZ - NOTE_BTN_GAP)
#define RAT_W    40                       /* star rating column              */
#define RAT_X_  (NOTE_BTN_X_ - RAT_W - 18)  /* wider gap from note button   */
#define YR_X_   (RAT_X_ - 12 - YR_W)
#define NM_X     16
#define NM_MW_  (YR_X_ - NM_X - 8)

/* ── Search bar ───────────────────────────────────────────────── */
#define SR_Y  (HDR_Y + 17)
#define SR_H   34
#define SR_R    9

/* ── Shared corner radii ──────────────────────────────────────── */
#define R_SM     5   /* small buttons, notes btn, grid strip      */
#define R_MD     7   /* search bar, list strip, list rows, tabs   */
#define R_LG    10   /* grid cards, notes overlay, stats pills    */
#define BRD_T    2   /* standard border thickness (px)            */

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



/* ── Grid view ───────────────────────────────────────────────── */
#define GRID_W   180
#define GRID_H   158   /* enough for 2-line name + info + gap + buttons */
#define GRID_GAP     10
#define GRID_TOP_PAD 12   /* extra space between tabs and first grid row */
#define LIST_TOP_PAD  6   /* matches the row vertical inset (ay+3) */
#define GBSZ      22   /* grid card status button size */
#define GBGP       2   /* grid card status button gap  */

/* ── Titlebar controls (legacy — kept for DD_ITEM_H / DD_BTN_H) ─── */
#define TC_Y   6
#define TC_H  22
#define TC_W  68
#define VC_W  32
#define TC_GAP 4
#define TD_BTN_W     42
#define TD_W         110
#define TD_ITEM_H    DD_ITEM_H
#define TC_X0  8
/* These old per-button macros are no longer used for layout but kept so
   any residual hover-settle code compiles without errors.              */
#define GENRE_BTN_X_ (TC_X0+DD_BTN_W+20)
#define GENRE_BTN_W  52
#define GENRE_DD_W   170
#define GENRE_DD_ITEM_H 22
#define TD_BTN_X_    (GENRE_BTN_X_+GENRE_BTN_W+20)
#define VIEW_X0_     (TD_BTN_X_+TD_BTN_W+20)
#define STATS_BTN_X_ (VIEW_X0_+2*(VC_W+TC_GAP)+4)

/* ── Command Center panel ────────────────────────────────────────── */
#define CMD_BTN_X     8          /* trigger button left edge            */
#define CMD_BTN_W    34          /* trigger button width                */
#define CMD_BTN_H    TC_H        /* trigger button height               */
#define CMD_BTN_Y    TC_Y        /* trigger button top edge             */
#define CMD_OW       430         /* panel width                         */
#define CMD_PAD       16         /* panel inner padding                 */
#define CMD_LBL_W     62         /* section label column width          */
#define CMD_SEC_H     52         /* height of one section row           */
#define CMD_PILL_H    30         /* inline control pill height          */
#define CMD_PILL_GAP   6         /* gap between adjacent pills          */
#define CMD_OH       (CMD_PAD*2 + 4*CMD_SEC_H + 4)  /* panel height (4 rows) */
#define CMD_PANEL_OY (TITLE_H + 2)                  /* panel top y     */
#define CMD_PILL_X0_ (CMD_BTN_X + CMD_PAD + CMD_LBL_W) /* pill start x */
#define CMD_SORT_PW  ((CMD_OW - CMD_PAD*2 - CMD_LBL_W - CMD_PILL_GAP*4) / 5)
#define CMD_VIEW_PW   80         /* List / Grid pill width              */
#define CMD_STATS_PW  112         /* Stats pill width — wide enough for icon + label */
#define CMD_GENRE_BTN_W 130
/* Section row y-start helpers — 4 rows: Sort / Display / Genre / Theme */
#define CMD_SORT_RY    (CMD_PANEL_OY + CMD_PAD)
#define CMD_DISPLAY_RY (CMD_SORT_RY    + CMD_SEC_H)
#define CMD_GENRE_RY   (CMD_DISPLAY_RY + CMD_SEC_H)
#define CMD_THEME_RY   (CMD_GENRE_RY   + CMD_SEC_H)
/* Keep old aliases so handle_cmd_click compiles without change */
#define CMD_VIEW_RY    CMD_DISPLAY_RY
#define CMD_STATS_RY   CMD_DISPLAY_RY
/* Hit-test rect helper */
#define IN_RECT(mx_,my_,x_,y_,w_,h_) \
    ((mx_)>=(x_)&&(mx_)<(x_)+(w_)&&(my_)>=(y_)&&(my_)<(y_)+(h_))

/* ═══════════════════════ Types ═══════════════════════════════════ */


typedef enum { RZ_NONE=0,RZ_N,RZ_NE,RZ_E,RZ_SE,RZ_S,RZ_SW,RZ_W,RZ_NW } RzDir;
typedef enum { TB_CLOSE_BTN, TB_MAX_BTN, TB_MIN_BTN } TBBtnType;

/* ═══════════════════════ Status metadata ═══════════════════════ */
static const char *SLBL[N_STATUS]  = {"WL","PD","IN","FN","DR","FV","RT"};
static const char *SNAME[N_TABS]   = {
    "All","Wishlist","Played","Playing",
    "Finished","Dropped","Favourites","Rotation"
};
static const C4 SCOL[N_STATUS] = {
    {255, 210,  0, 255},  /* S_WISH     — vivid yellow     */
    { 50, 220,  80, 255},  /* S_PLAYED   — vivid green      */
    {  0, 200, 255, 255},  /* S_PLAYING  — electric cyan    */
    { 60, 100, 255, 255},  /* S_FINISHED — deep blue        */
    {230,  50,  50, 255},  /* S_DROPPED  — bright red       */
    {255,  60, 160, 255},  /* S_FAV      — hot magenta      */
    {180,  80, 255, 255},  /* S_ROTATION — vivid violet     */
};
static const int SPRIO[N_STATUS] = {
    S_FAV,S_PLAYING,S_FINISHED,S_PLAYED,
    S_ROTATION,S_WISH,S_DROPPED
};
/* Tab display order — matches status-popup priority (SPRIO) */
static const int TAB_ORDER[N_TABS] = {
    T_ALL, T_FAV, T_PLAYING, T_FINISHED, T_PLAYED, T_ROTATION, T_WISH, T_DROPPED
};
/* Returns display position (0..N_TABS-1) for a given tab id */
static int tab_disp(int tab){
    for(int i=0;i<N_TABS;i++) if(TAB_ORDER[i]==tab) return i;
    return 0;
}

/* ═══════════════════════ Globals ═══════════════════════════════ */
Game  db[MAX_G];
int   ndb=0, flt[MAX_G], nflt=0;
int   flt_score[MAX_G];
TabId cur_tab=T_ALL;
TabId prev_tab=T_ALL;
char  srch[128]="";
int          s_on=0;
static int   hov_db=-1;
static int   tip_gi=-1, tip_bj=-1; /* hovered status segment for tooltip */
/* ── Search edit state ──────────────────────────────────────────── */
int   srch_cur    = 0;     /* cursor byte index               */
int   srch_sel0   = 0;     /* selection anchor byte index     */
float srch_blink  = 0.f;   /* cursor blink phase              */
int   srch_scroll = 0;     /* horizontal scroll in pixels     */
int   srch_drag   = 0;     /* mouse-dragging selection        */
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
float scr_f=0.f, scr_tgt=0.f;
int   cur_page=0;          /* current page (0-based)    */
static float page_slide=0.f;      /* animated page indicator   */
static float pg_prev_hov=0.f;     /* prev button hover         */
static float pg_next_hov=0.f;     /* next button hover         */
static float pg_num_hov[64];      /* per-dot hover             */
SortMode sort_mode = SORT_AZ;
ViewMode view_mode = VIEW_LIST;
static float tc_sort_hov[4];
static float tc_view_hov[2];
static float tc_stats_hov = 0.f;  /* stats button hover */
static float tab_hov[N_TABS];          /* per-tab hover                     */
/* ── Filter chips ───────────────────────────────────────────────────── */
int   chip_band     = 0;        /* 0 or CHIP_H — shifts TAB_Y/LST_Y */
char  filt_genres[8][32];       /* active genre filters (multi-select)     */
int   n_filt_genres = 0;        /* number of active genre filters          */
int   filt_year     = 0;        /* 0 = inactive                      */
static float chip_genre_hov= 0.f;
static float chip_year_hov = 0.f;
static float sort_ind_f = 0.f;   /* sliding pill – index space */
static float view_ind_f = 0.f;   /* sliding pill – view mode   */
/* ── Notes overlay ──────────────────────────────────────────────── */
static int   note_open = -1;   /* db[] index of open note, -1=closed */
static int   note_scroll = 0;  /* vertical pixel scroll inside text area */
static float note_anim = 0.f;  /* 0..1 fade in/out                   */
static int   note_cur  = 0;    /* cursor position in notes string     */
static int   note_sel0 = 0;    /* selection anchor (like srch_sel0)   */
static int   note_drag = 0;    /* 1 while drag-selecting inside notes */
/* sort dropdown */
static int   sort_dd_open  = 0;   /* 1 = dropdown visible              */
static float sort_dd_anim  = 0.f; /* 0..1 open/close animation         */
static float sort_btn_hov  = 0.f; /* button hover                      */
static float sort_item_hov[5];    /* per-item hover                     */
static int   theme_dd_open = 0;        /* theme dropdown open               */
static float theme_dd_anim = 0.f;      /* 0..1 open/close                   */
static float theme_btn_hov = 0.f;      /* theme button hover                */
static float theme_item_hov[N_THEMES]; /* per-item hover                    */
static float theme_ind_f   = 0.f;      /* sliding selection pill            */
/* ── Genre filter dropdown ──────────────────────────────────────────── */
static int   genre_dd_open = 0;        /* dropdown open flag                */
static float genre_dd_anim = 0.f;      /* 0..1 open/close animation         */
static float genre_btn_hov = 0.f;      /* genre button hover                */
static float genre_item_hov[64];       /* per-genre item hover              */
char  genre_list[64][32];       /* sorted unique genre strings       */
int   genre_counts[64];         /* game count per genre              */
int   n_genres = 0;             /* number of unique genres           */
/* Genre dropdown anchor — set each frame by draw_cmd_panel              */
static int   genre_dd_ax   = CMD_PILL_X0_;  /* abs x anchor                */
static int   genre_dd_ay   = CMD_GENRE_RY;  /* abs y anchor (below button) */
/* ── Command Center overlay ─────────────────────────────────────────── */
static int   cmd_open      = 0;        /* 1 = panel visible                 */
static float cmd_anim      = 0.f;      /* 0..1 open/close animation         */
static float cmd_btn_hov   = 0.f;      /* trigger button hover              */
static float cmd_sort_hov[5];          /* sort pill hovers inside panel     */
static float cmd_stats_hov2= 0.f;      /* stats btn hover inside panel      */
/* ── Search highlight ───────────────────────────────────────────────── */
char  srch_lc[128] = "";        /* lowercase srch, updated in rebuild*/
/* ── Status badge popup ─────────────────────────────────────────────── */
static int   badge_open_gi  = -1;      /* db[] index whose popup is open    */
static int   badge_open_ri  = -1;      /* flt[] row — kept alive during close anim */
static int   badge_last_gi  = -1;      /* last open gi, used while anim closes */
static float badge_anim     = 0.f;     /* 0..1 open/close, same as titlebar dropdowns */
static float badge_hov[MAX_G];         /* per-game badge hover              */
static float badge_item_hov[N_STATUS]; /* per-item hover inside popup       */
static int   badge_grid_bx  = 0;       /* grid badge pill anchor x          */
static int   badge_grid_by  = 0;       /* grid badge pill anchor y          */
static ViewMode badge_open_vm = VIEW_LIST; /* which view mode opened the popup */
/* Close: clear open state but keep ri/last_gi alive for the close animation */
#define BADGE_CLOSE()      do{ badge_open_gi=-1; }while(0)
/* Instant close — skips fade-out animation; use on context changes (view switch, tab change, page) */
#define BADGE_CLOSE_NOW() do{ badge_open_gi=-1; badge_open_ri=-1; badge_last_gi=-1; badge_anim=0.f; }while(0)
#define DD_ITEM_H  26
#define DD_W       100
#define DD_BTN_H   TC_H
#define DD_BTN_W   42
static float tab_ix=8.f, tab_itx=8.f;
static float tb_ch=0.f, tb_mh=0.f, tb_nh=0.f;
static float tb_close_fl=0.f, tb_max_fl=0.f, tb_min_fl=0.f;
static float tab_fl[N_TABS];          /* tab click flash              */
static float tc_fl[5];                /* titlebar ctrl flash: sort,theme,v0,v1,stats */
static float srch_spin=0.f;
static float srch_glow=0.f;
static Uint64 plast=0;
static float  dt_=0.016f;



/* ═══════════════════════ SDL globals ═══════════════════════════ */
static SDL_Window   *win=NULL;
SDL_Renderer *ren=NULL;
static TTF_Font     *f22=NULL,*f18=NULL,*f14=NULL,*f12=NULL;
static TTF_Font     *f_icon=NULL;  /* Material Symbols icon font */
static SDL_Cursor   *cur_arr=NULL,*cur_ns=NULL,*cur_ew=NULL,
                    *cur_nwse=NULL,*cur_nesw=NULL;



/* ═══════════════════════ Audio / SFX ═══════════════════════════ */






/* ── Forward declarations ─────────────────────────────────────── */
static int  grid_cols(void);
static int  row_at(int mx,int my);
static int  badge_popup_pos(int *px_out, int *py_out);
static void draw_note_overlay(void);
static void draw_stats(void);

/* ═══════════════════════ Draw helpers ══════════════════════════ */
void sc_(C4 c){ SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a); }
static void fr_(int x,int y,int w,int h,C4 c){
    sc_(c); SDL_Rect r={x,y,w,h}; SDL_RenderFillRect(ren,&r);
}
/* Galaxy theme transparency: scales any fill alpha down to ~48 %
   so the star-field and nebulae bleed through every surface.       */
static inline C4 gal_bg(C4 c){
    if(cur_theme==8) c.a=(Uint8)((int)c.a*48/100);
    return c;
}

static void frr(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>w) r=w/2; if(2*r>h) r=h/2;
    sc_(c);
    float rf=(float)r;
    for(int row=0;row<h;row++){
        int x0=x,x1=x+w;
        if(row<r){
            /* use same pixel-centred float calc as frr_aa so inner fill
               curve matches outer AA curve exactly in bfrr_aa */
            float d=(float)(r-row)-0.5f; int dx=(int)sqrtf(rf*rf-d*d);
            x0=x+r-dx; x1=x+w-r+dx;
        } else if(row>=h-r){
            float d=(float)(row-(h-r))+0.5f; int dx=(int)sqrtf(rf*rf-d*d);
            x0=x+r-dx; x1=x+w-r+dx;
        }
        if(x1>x0) SDL_RenderDrawLine(ren,x0,y+row,x1-1,y+row);
    }
}
/* round left corners only — right edge is straight */
static void frr_left(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>h) r=h/2;
    sc_(c); float rf=(float)r;
    for(int row=0;row<h;row++){
        int x0=x;
        if(row<r){ float d=(float)(r-row)-0.5f; x0=x+r-(int)sqrtf(rf*rf-d*d); }
        else if(row>=h-r){ float d=(float)(row-(h-r))+0.5f; x0=x+r-(int)sqrtf(rf*rf-d*d); }
        SDL_RenderDrawLine(ren,x0,y+row,x+w-1,y+row);
    }
}
/* round right corners only — left edge is straight */
static void frr_right(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>h) r=h/2;
    sc_(c); float rf=(float)r;
    for(int row=0;row<h;row++){
        int x1=x+w;
        if(row<r){ float d=(float)(r-row)-0.5f; x1=x+w-r+(int)sqrtf(rf*rf-d*d); }
        else if(row>=h-r){ float d=(float)(row-(h-r))+0.5f; x1=x+w-r+(int)sqrtf(rf*rf-d*d); }
        SDL_RenderDrawLine(ren,x,y+row,x1-1,y+row);
    }
}

void frr_aa(int x,int y,int w,int h,int r,C4 c){
    if(w<=0||h<=0) return;
    if(r<=0){ fr_(x,y,w,h,c); return; }
    if(2*r>w) r=w/2; if(2*r>h) r=h/2;
    float rf=(float)r;
    /* always BLEND so semi-transparent colours render correctly in all rows */
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
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
            SDL_RenderDrawLine(ren, lx, y+row,     rx, y+row);
            SDL_RenderDrawLine(ren, lx, y+h-1-row, rx, y+h-1-row);
        }
        /* 1st AA pixel – partial edge coverage */
        C4 aa1=c; aa1.a=(Uint8)(c.a*frac);
        sc_(aa1);
        SDL_RenderDrawPoint(ren, lx-1, y+row);
        SDL_RenderDrawPoint(ren, rx+1, y+row);
        SDL_RenderDrawPoint(ren, lx-1, y+h-1-row);
        SDL_RenderDrawPoint(ren, rx+1, y+h-1-row);
        /* 2nd AA pixel – feathered falloff for smoother edge */
        Uint8 a2=(Uint8)(c.a*frac*frac*0.30f);
        if(a2>1){
            C4 aa2=c; aa2.a=a2; sc_(aa2);
            SDL_RenderDrawPoint(ren, lx-2, y+row);
            SDL_RenderDrawPoint(ren, rx+2, y+row);
            SDL_RenderDrawPoint(ren, lx-2, y+h-1-row);
            SDL_RenderDrawPoint(ren, rx+2, y+h-1-row);
        }
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
void fblend(int x,int y,int w,int h,C4 c){
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    sc_(c); SDL_Rect r={x,y,w,h}; SDL_RenderFillRect(ren,&r);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}
/* ═══════════════════════ Text Texture Cache ════════════════════
   Strings are rendered once as white and cached by (font, string).
   Colour is applied at draw-time via SDL_SetTextureColorMod so
   the same cached texture works for every colour variant.
   Eliminates per-frame Surface/Texture alloc+free (was ~150/frame).
   ════════════════════════════════════════════════════════════════ */
#define TC_SLOTS  2048          /* must be power of two             */
#define TC_MASK   (TC_SLOTS-1)
#define TC_PROBE  16            /* max linear-probe steps           */
#define TC_KEYLEN 192

typedef struct {
    char        key[TC_KEYLEN];
    TTF_Font   *font;           /* NULL = empty slot                */
    SDL_Texture*tex;
    int         w, h;
    Uint32      lru;
} TxEntry;

static TxEntry  tc_pool[TC_SLOTS];
static Uint32   tc_clock = 0;

static Uint32 tc_hash(TTF_Font *f, const char *s){
    Uint32 h = (Uint32)(uintptr_t)f * 2654435761u;
    while(*s) h = h*31u + (unsigned char)*s++;
    return h;
}

/* Returns a cached white texture for (font, string), creating it if needed. */
static TxEntry *tc_get(TTF_Font *f, const char *s){
    if(!f||!s||!*s) return NULL;
    Uint32 start = tc_hash(f,s) & TC_MASK;

    /* 1. Search probe window for existing entry or first empty slot */
    int empty_slot = -1;
    for(int i=0;i<TC_PROBE;i++){
        int idx=(int)((start+i) & TC_MASK);
        TxEntry *e=&tc_pool[idx];
        if(!e->font){ if(empty_slot<0) empty_slot=idx; break; }
        if(e->font==f && strncmp(e->key,s,TC_KEYLEN-1)==0){
            e->lru=++tc_clock; return e;
        }
    }

    /* 2. Need to insert — find slot: empty if available, else evict oldest in window */
    int slot = empty_slot;
    if(slot<0){
        Uint32 oldest=UINT32_MAX;
        for(int i=0;i<TC_PROBE;i++){
            int idx=(int)((start+i) & TC_MASK);
            if(tc_pool[idx].lru < oldest){ oldest=tc_pool[idx].lru; slot=idx; }
        }
        SDL_DestroyTexture(tc_pool[slot].tex);
        tc_pool[slot].font=NULL;
    }

    /* 3. Render as white (colour applied at draw time via ColorMod) */
    SDL_Color white={255,255,255,255};
    SDL_Surface *sf=TTF_RenderUTF8_Blended(f,s,white); if(!sf) return NULL;
    SDL_Texture *tx=SDL_CreateTextureFromSurface(ren,sf);
    SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);
    TxEntry *e=&tc_pool[slot];
    strncpy(e->key,s,TC_KEYLEN-1); e->key[TC_KEYLEN-1]=0;
    e->font=f; e->tex=tx; e->w=sf->w; e->h=sf->h; e->lru=++tc_clock;
    SDL_FreeSurface(sf);
    return e;
}

static void tc_free_all(void){
    for(int i=0;i<TC_SLOTS;i++)
        if(tc_pool[i].font){ SDL_DestroyTexture(tc_pool[i].tex); tc_pool[i].font=NULL; }
}

/* ── Drawing helpers (use cache) ─────────────────────────────── */
static void rtx(TTF_Font *f,const char *s,int x,int y,C4 c){
    TxEntry *e=tc_get(f,s); if(!e) return;
    SDL_SetTextureColorMod(e->tex,c.r,c.g,c.b);
    SDL_SetTextureAlphaMod(e->tex,c.a);
    SDL_Rect d={x,y,e->w,e->h}; SDL_RenderCopy(ren,e->tex,NULL,&d);
}
static void rtxclip(TTF_Font *f,const char *s,int x,int y,int mw,C4 c){
    if(mw<=0) return;
    TxEntry *e=tc_get(f,s); if(!e) return;
    SDL_SetTextureColorMod(e->tex,c.r,c.g,c.b);
    SDL_SetTextureAlphaMod(e->tex,c.a);
    int cw=e->w<mw?e->w:mw;
    SDL_Rect src={0,0,cw,e->h},dst={x,y,cw,e->h};
    SDL_RenderCopy(ren,e->tex,&src,&dst);
}
static int txw_(TTF_Font *f,const char *s){
    TxEntry *e=tc_get(f,s); return e?e->w:0;
}
static void rtxcen(TTF_Font *f,const char *s,int rx,int ry,int rw,int rh,C4 c){
    TxEntry *e=tc_get(f,s); if(!e) return;
    SDL_SetTextureColorMod(e->tex,c.r,c.g,c.b);
    SDL_SetTextureAlphaMod(e->tex,c.a);
    int x=rx+(rw-e->w)/2, y=ry+(rh-e->h)/2;
    SDL_Rect d={x,y,e->w,e->h}; SDL_RenderCopy(ren,e->tex,NULL,&d);
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
    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));}
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
    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));}
    rebuild();
}

/* ═══════════════════════ Animation tick ════════════════════════ */
/* ── Page helper forward declarations ───────────────────────────── */
static int page_size(void);
static int total_pages(void);
void clamp_page(void);
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
    { float sp=1.f-powf(0.00004f,dt_); SETTLE(scr_f,scr_tgt,sp); }

    /* ── Grid/List hover detection (polled every tick) ── */
    if(note_open>=0){
        hov_db=-1;
    } else {
        int mx4,my4; SDL_GetMouseState(&mx4,&my4);
        int dd_over=(cmd_open||cmd_anim>0.05f||
                    genre_dd_open||genre_dd_anim>0.05f);
        hov_db=-1;
        if(!dd_over){
            if(view_mode==VIEW_GRID){
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
            } else {
                int r4=row_at(mx4,my4);
                /* don't highlight rows beneath the open popup */
                int over_pop4=0;
                if(badge_open_gi>=0&&badge_anim>0.05f){
                    int ppx4,ppy4; if(badge_popup_pos(&ppx4,&ppy4))
                        over_pop4=(mx4>=ppx4&&mx4<ppx4+BPOP_W&&my4>=ppy4&&my4<ppy4+BPOP_H);
                }
                if(r4>=0&&!over_pop4) hov_db=flt[r4];
            }
        }
    }

    /* ── Row hover – only visible entries ── */
    {
        float hs=clampf(dt_*40.f,0.f,1.f);
        int first = page_first();
        int last  = page_last();
        /* animate all visible entries toward their target */
        for(int ri=first;ri<=last;ri++){
            int i=flt[ri];
            float tg=(i==hov_db)?1.f:0.f;
            if(fabsf(row_ht[i]-tg)<0.0015f){ row_ht[i]=tg; }
            else { row_ht[i]+=(tg-row_ht[i])*hs; busy=1; }
        }
        /* decay previously-hovered entry if it's off the current page */
        if(hov_db>=0){
            int found=0;
            for(int ri=first;ri<=last;ri++) if(flt[ri]==hov_db){found=1;break;}
            if(!found){
                if(row_ht[hov_db]>0.0015f){ row_ht[hov_db]-=hs; busy=1; }
                else row_ht[hov_db]=0.f;
            }
        }
    }

    /* ── Button + UI flash fade ── */
    {
        float bd=dt_*8.0f;
        for(int i=0;i<N_TABS;i++){ if(tab_fl[i]>0.f){tab_fl[i]-=bd;if(tab_fl[i]<0)tab_fl[i]=0;busy=1;}}
        for(int i=0;i<5;i++){ if(tc_fl[i]>0.f){tc_fl[i]-=bd;if(tc_fl[i]<0)tc_fl[i]=0;busy=1;}}
        if(tb_close_fl>0.f){tb_close_fl-=bd;if(tb_close_fl<0)tb_close_fl=0.f;busy=1;}
        if(tb_max_fl  >0.f){tb_max_fl  -=bd;if(tb_max_fl  <0)tb_max_fl  =0.f;busy=1;}
        if(tb_min_fl  >0.f){tb_min_fl  -=bd;if(tb_min_fl  <0)tb_min_fl  =0.f;busy=1;}
        bd=dt_*5.0f;
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
        float slide=clampf(dt_*30.f,0.f,1.f);
        if(cur_tab<N_TABS) tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));
        if(rz_drag||win_drag){ tab_ix=tab_itx; }
        else { SETTLE(tab_ix,tab_itx,slide); }
        SETTLE(sort_ind_f,(float)sort_mode,slide);
        SETTLE(view_ind_f,(float)view_mode,slide);
    }

    /* ── Titlebar close/max/min hover ── */
    {
        int mx,my; SDL_GetMouseState(&mx,&my);
        int has_focus=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        float bs=1.f-powf(0.0004f,dt_);
        float c_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_CX))?1.f:0.f;
        float m_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_MX))?1.f:0.f;
        float n_tgt=(has_focus&&my<TITLE_H&&IN_BTN(mx,TB_NX))?1.f:0.f;
        SETTLE(tb_ch,c_tgt,bs); SETTLE(tb_mh,m_tgt,bs); SETTLE(tb_nh,n_tgt,bs);
    }

    /* ── Theme ring slide ── */
    SETTLE(sel_ring_if,(float)cur_theme,clampf(dt_*30.f,0.f,1.f));

    /* ── Galaxy dot selection animation ── */
    { float tgt=(cur_theme==8)?1.f:0.f; SETTLE(galaxy_dot_f,tgt,1.f-powf(0.00005f,dt_)); }

    /* ── Theme colour transition + dot animations ── */
    tick_theme(dt_);
    if(tc_t<1.f) busy=1;
    {
        int mx2,my2; SDL_GetMouseState(&mx2,&my2);
        int mf=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        float ths=1.f-powf(0.0008f,dt_);
        for(int i=0;i<N_THEMES;i++){
            float tg=(mf&&hit_theme_dot(mx2,my2)==i)?1.f:0.f;
            SETTLE(tdot_hov[i],tg,ths);
        }
        for(int i=0;i<N_THEMES;i++){
            if(tdot_bounce[i]>0.f){
                tdot_bounce[i]-=dt_*8.0f;
                if(tdot_bounce[i]<0.f) tdot_bounce[i]=0.f;
                busy=1;
            }
        }
        if(theme_pulse>0.f){theme_pulse-=dt_*4.f;if(theme_pulse<0)theme_pulse=0; busy=1;}

        /* ── Hover detection for CMD trigger + panel controls ── */
        float ths2=1.f-powf(0.001f,dt_);
        int mx3,my3; SDL_GetMouseState(&mx3,&my3);
        int hf2=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;

        /* CMD trigger button hover */
        {
            int in_btn=(hf2&&my3>=CMD_BTN_Y&&my3<CMD_BTN_Y+CMD_BTN_H
                        &&mx3>=CMD_BTN_X&&mx3<CMD_BTN_X+CMD_BTN_W);
            SETTLE(cmd_btn_hov,(float)(in_btn||cmd_open),ths2);
        }

        /* CMD panel — sort pill hover */
        {
            int pw=CMD_SORT_PW, ph=CMD_PILL_H;
            int py=CMD_SORT_RY+(CMD_SEC_H-ph)/2;
            int panel_vis=(cmd_open||cmd_anim>0.05f);
            for(int i=0;i<5;i++){
                int px2=CMD_PILL_X0_+i*(pw+CMD_PILL_GAP);
                float tg=(panel_vis&&hf2&&IN_RECT(mx3,my3,px2,py,pw,ph))?1.f:0.f;
                SETTLE(cmd_sort_hov[i],tg,ths2);
            }
        }

        /* CMD panel — view pill hover (reusing tc_view_hov) */
        {
            int pw=CMD_VIEW_PW, ph=CMD_PILL_H;
            int py=CMD_VIEW_RY+(CMD_SEC_H-ph)/2;
            int panel_vis=(cmd_open||cmd_anim>0.05f);
            for(int i=0;i<2;i++){
                int px2=CMD_PILL_X0_+i*(pw+CMD_PILL_GAP);
                float tg=(panel_vis&&hf2&&IN_RECT(mx3,my3,px2,py,pw,ph))?1.f:0.f;
                SETTLE(tc_view_hov[i],tg,ths2);
            }
        }

        /* CMD panel — stats button hover (reusing tc_stats_hov) — sits after view pills */
        {
            int stats_px=CMD_PILL_X0_+2*(CMD_VIEW_PW+CMD_PILL_GAP)+CMD_PILL_GAP;
            int pw=CMD_STATS_PW, ph=CMD_PILL_H;
            int py=CMD_DISPLAY_RY+(CMD_SEC_H-ph)/2;
            int panel_vis=(cmd_open||cmd_anim>0.05f);
            float tg=(panel_vis&&hf2&&IN_RECT(mx3,my3,stats_px,py,pw,ph))?1.f:0.f;
            SETTLE(tc_stats_hov,tg,ths2);
        }

        /* CMD panel — genre button hover (reusing genre_btn_hov) */
        {
            int pw=CMD_GENRE_BTN_W, ph=CMD_PILL_H;
            int py=CMD_GENRE_RY+(CMD_SEC_H-ph)/2;
            int panel_vis=(cmd_open||cmd_anim>0.05f);
            float tg=(panel_vis&&hf2&&IN_RECT(mx3,my3,CMD_PILL_X0_,py,pw,ph))?1.f:0.f;
            SETTLE(genre_btn_hov,(float)(tg>0.5f||genre_dd_open),ths2);
        }

        /* sort dropdown animation settle (always closed now — decays to 0) */
        { float dd_spd=1.f-powf(0.00002f,dt_); SETTLE(sort_dd_anim,0.f,dd_spd); }

        /* CMD panel — cmd_anim open/close */
        { float tgt=cmd_open?1.f:0.f;
          float spd=1.f-powf(cmd_open?0.000003f:0.00003f,dt_);
          float d=tgt-cmd_anim;
          if(fabsf(d)<0.0015f) cmd_anim=tgt; else{ cmd_anim+=d*spd; busy=1; } }

        /* Theme dot hover — hit against the CMD panel dot row */
        {
            float ths_dot=1.f-powf(0.0008f,dt_);
            int panel_vis=(cmd_open||cmd_anim>0.05f);
            int r2=7, dot_step=TDOT_STEP;
            int dot_x0=CMD_PILL_X0_+r2;
            int dot_cy=CMD_THEME_RY+CMD_SEC_H/2;
            for(int i=0;i<N_THEMES;i++){
                int dcx=dot_x0+i*dot_step;
                float tg=(panel_vis&&hf2&&mx3>=dcx-r2-2&&mx3<dcx+r2+2
                           &&my3>=dot_cy-r2-2&&my3<dot_cy+r2+2)?1.f:0.f;
                SETTLE(tdot_hov[i],tg,ths_dot);
            }
            /* update dots_x0/dots_cy for hit_theme_dot compatibility */
            dots_x0=dot_x0; dots_cy=dot_cy;
            dots_in_tb=0; /* dots live only in CMD panel */
        }

        /* Theme dropdown animation (always closed — decays to 0) */
        { SETTLE(theme_btn_hov,0.f,ths2);
          float dd_spd=1.f-powf(0.00002f,dt_);
          SETTLE(theme_dd_anim,0.f,dd_spd);
          SETTLE(theme_ind_f,(float)cur_theme,clampf(dt_*30.f,0.f,1.f)); }

        /* Genre dropdown animation */
        {
            float dd_tgt=genre_dd_open?1.f:0.f;
            float dd_spd=1.f-powf(genre_dd_open?0.000002f:0.00002f,dt_);
            SETTLE(genre_dd_anim,dd_tgt,dd_spd);
            float gi_spd=1.f-powf(0.000005f,dt_);
            for(int i=0;i<n_genres;i++){
                int item_y=genre_dd_ay+4+i*DD_ITEM_H;
                int in_item=(genre_dd_open&&genre_dd_anim>0.05f&&hf2&&
                             mx3>=genre_dd_ax&&mx3<genre_dd_ax+GENRE_DD_W&&
                             my3>=item_y&&my3<item_y+DD_ITEM_H);
                SETTLE(genre_item_hov[i],(float)in_item,gi_spd);
            }
        }

        /* ── Button hover (skip entries already at target) ── */
        float bspd=1.f-powf(0.00015f,dt_);
        int hov_gi=-1,hov_bj=-1;
        /* suppress ALL row/tab/button hover whenever any dropdown or CMD panel is open */
        int dd_blocks_hover = (cmd_open||cmd_anim>0.05f||
                               genre_dd_open||genre_dd_anim>0.05f);
        if(!dd_blocks_hover){
        /* ── Tab hover ── */
        { int tw2=TAB_W_;
          for(int i=0;i<N_TABS;i++){
              int tx2=TAB_X_(i);
              float tg=(hf2&&my3>=TAB_Y&&my3<TAB_Y+TAB_H&&mx3>=tx2&&mx3<tx2+tw2)?1.f:0.f;
              SETTLE(tab_hov[i],tg,ths2);
          }
        }
        if(1){
        if(view_mode==VIEW_LIST){
            int hov_ri2=row_at(mx2,my2);
            /* suppress badge hover when the mouse is inside the open popup */
            int over_popup=0;
            if(badge_open_gi>=0 && badge_anim>0.05f){
                int ppx,ppy; if(badge_popup_pos(&ppx,&ppy))
                    over_popup=(mx2>=ppx&&mx2<ppx+BPOP_W&&my2>=ppy&&my2<ppy+BPOP_H);
            }
            if(!over_popup && hov_ri2>=0&&hov_ri2<nflt){
                int gi2=flt[hov_ri2];
                int ay_h=LST_DATA_Y+LIST_TOP_PAD+(hov_ri2-page_first())*ROW_H;
                int bby_h=ay_h+BADGE_YO;
                if(mx2>=BADGE_X_&&mx2<BADGE_X_+BADGE_W&&my2>=bby_h&&my2<bby_h+BADGE_H)
                    hov_gi=gi2;
            }
        } else {
            /* Grid: detect badge pill hover */
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
                    /* Match draw_grid_card: chip_h=24, chip_y = cy2+GRID_H-24-6 */
                    int gbh = 24, gby = cy2 + GRID_H - gbh - 6;
                    int gbx = cx2 + 8, gbw = GRID_W - 16;
                    if(mx2>=gbx&&mx2<gbx+gbw&&my2>=gby&&my2<gby+gbh)
                        hov_gi=hov_db;
                    break;
                }
            }
        }
        /* ── Badge hover (list + grid) ── */
        {
            int vfirst2=page_first(), vlast2=page_last();
            for(int ri=vfirst2;ri<=vlast2;ri++){
                int i=flt[ri];
                float tgt=(i==hov_gi)?1.f:0.f;
                if(badge_hov[i]==0.f&&tgt==0.f) continue;
                SETTLE(badge_hov[i],tgt,bspd);
                if(fabsf(badge_hov[i]-tgt)>0.001f) busy=1;
            }
        }
        /* Only iterate visible entries for btn_hov (grid cards) */
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
        } /* end button hover */
        } /* end !dd_blocks_hover */
        tip_gi=hov_gi; tip_bj=hov_bj;

        /* ── Filter chip hover ── */
        if(chip_band > 0){
            int mx_c,my_c; SDL_GetMouseState(&mx_c,&my_c);
            float cspd = 1.f-powf(0.001f,dt_);
            int cy_chip = TITLE_H + HDR_H;
            int ih_c = CHIP_H - 8;
            int chip_y = cy_chip + (CHIP_H - ih_c) / 2;
            const char *XG = "\xc3\x97";
            int xw_c = txw_(f12, XG);
            int ccx = 10;
            if(n_filt_genres>0){
                char lbl[96]; int pos3=snprintf(lbl,sizeof(lbl),"Genre: ");
                for(int gi=0;gi<n_filt_genres&&pos3<(int)sizeof(lbl)-2;gi++){
                    if(gi>0) pos3+=snprintf(lbl+pos3,sizeof(lbl)-pos3,", ");
                    pos3+=snprintf(lbl+pos3,sizeof(lbl)-pos3,"%s",filt_genres[gi]);
                }
                int cw = txw_(f12,lbl) + xw_c + 20;
                float tgt_g = (my_c>=chip_y&&my_c<chip_y+ih_c&&mx_c>=ccx&&mx_c<ccx+cw) ? 1.f:0.f;
                SETTLE(chip_genre_hov,tgt_g,cspd);
                if(fabsf(chip_genre_hov-tgt_g)>0.001f) busy=1;
                ccx += cw + 6;
            }
            if(filt_year){
                char lbl[32]; snprintf(lbl,sizeof(lbl),"Year: %d",filt_year);
                int cw = txw_(f12,lbl) + xw_c + 20;
                float tgt_y = (my_c>=chip_y&&my_c<chip_y+ih_c&&mx_c>=ccx&&mx_c<ccx+cw) ? 1.f:0.f;
                SETTLE(chip_year_hov,tgt_y,cspd);
                if(fabsf(chip_year_hov-tgt_y)>0.001f) busy=1;
            }
        }
        { float dd_tgt = (badge_open_gi>=0) ? 1.f : 0.f;
          float dd_spd = 1.f-powf(badge_open_gi>=0 ? 0.000002f : 0.00002f, dt_);
          SETTLE(badge_anim, dd_tgt, dd_spd);
          if(fabsf(badge_anim-dd_tgt)>0.001f) busy=1;
          /* once fully closed, release the position data */
          if(badge_open_gi<0 && badge_anim<0.01f) badge_open_ri=-1;
          if(badge_open_gi>=0 && badge_anim>0.05f){
              int mx_b,my_b; SDL_GetMouseState(&mx_b,&my_b);
              int px_b, py_b;
              if(badge_popup_pos(&px_b,&py_b)){
                  float ispd=1.f-powf(0.0001f,dt_);
                  for(int pi_h=0;pi_h<N_STATUS;pi_h++){
                      int iy=py_b+4+pi_h*BPOP_ITEM_H;
                      float tgt2=(mx_b>=px_b&&mx_b<px_b+BPOP_W&&my_b>=iy&&my_b<iy+BPOP_ITEM_H)?1.f:0.f;
                      SETTLE(badge_item_hov[pi_h],tgt2,ispd);
                      if(fabsf(badge_item_hov[pi_h]-tgt2)>0.001f) busy=1;
                  }
              }
          }
        }
    } /* end theme/hover block */

    /* ── Page bar hover ── */
    {
        int mx2,my2; SDL_GetMouseState(&mx2,&my2);
        int py=PG_BAR_Y, ph=PG_H;
        float spd=1.f-powf(0.001f,dt_);
        int bw=40,bh=34,cy2=py+ph/2;
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
        SETTLE(page_slide,(float)cur_page,clampf(dt_*22.f,0.f,1.f));
        if(fabsf(page_slide-(float)cur_page)>0.001f) busy=1;
    }

    /* ── Cursor blink (always busy when search focused) ── */
    if(s_on){ srch_blink+=dt_*1.8f; if(srch_blink>=2.f) srch_blink-=2.f; busy=1; }
    else srch_blink=0.f;
    /* ── Note overlay blink / fade ── */
    if(note_open>=0) busy=1;
    { float tgt=(note_open>=0)?1.f:0.f;
      float spd=1.f-powf(0.00002f,dt_);
      float d=tgt-note_anim;
      if(fabsf(d)<0.0015f) note_anim=tgt; else { note_anim+=d*spd; busy=1; } }

    /* ── Search spin ── */
    {
        float stgt=(s_on||*srch)?1.f:0.f;
        if(fabsf(srch_glow-stgt)<0.002f) srch_glow=stgt;
        else{ srch_glow+=(stgt-srch_glow)*(1.f-powf(0.0008f,dt_)); busy=1; }
        if(srch_glow>0.01f){
            float rf2=(float)SR_R;
            float arc2=rf2*(float)M_PI/2.f;
            float sx2=(float)(sr_w-1)-2.f*rf2;
            float sy2=(float)(SR_H-1)-2.f*rf2;
            float perim=2.f*(sx2+sy2)+4.f*arc2;
            float spd = (cur_theme == 8)
                        ? (s_on ? 700.f : 350.f)   /* Galaxy: stronger comet */
                        : (s_on ? 460.f : 220.f);
            srch_spin=fmodf(srch_spin+dt_*spd,perim);
            busy=1;
        }
    }

    /* ── Galaxy theme: always run continuously — never sleep the render loop.
     * The parallax drift, nebula breathing, and star twinkle all require a
     * steady frame clock independent of user input.  gal_paused is set by
     * window-focus events so we still respect minimise/tab-switch. ── */
    if(cur_theme == 8 && !gal_paused) busy = 1;

#undef SETTLE
    return busy;
}

/* ═══════════════════════ Page helpers ══════════════════════════ */
static int page_size_list(void){ int r=(LST_H_-COL_HDR_H)/ROW_H; return r<1?1:r; }
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
void clamp_page(void){
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
    if(p!=cur_page){ BADGE_CLOSE_NOW(); }
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




/* ── Build sorted unique genre list from entire db ─────────────────────── */

static int tcnt(int si){ int n=0; for(int i=0;i<ndb;i++) if(db[i].st[si]) n++; return n; }


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
    /* Pixel boundary: 0..(w-1) and 0..(h-1) */
    float W    = (float)(sr_w - 1);
    float H    = (float)(SR_H - 1);
    float sx   = W - 2.f*rf;   /* top/bottom straight length */
    float sy   = H - 2.f*rf;   /* left/right straight length */
    float perim = 2.f*(sx+sy) + 4.f*arc;

    float x0   = (float)sr_x;
    float y0   = (float)SR_Y;

    float seg_len[8] = { sx, arc, sy, arc, sx, arc, sy, arc };

    int tail = (cur_theme == 8)
               ? (int)(perim * 0.58f)   /* Galaxy: long comet tail */
               : (int)(perim * 0.40f);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    for(int i=0; i<=tail; i++){
        float pos = fmodf(srch_spin - (float)i + perim*200.f, perim);
        float t   = 1.f - (float)i / (float)tail;
        float a   = t*t * srch_glow;
        if(a < 0.003f) continue;

        float acc=0.f; int seg=-1; float loc=0.f;
        for(int k=0;k<8;k++){
            if(pos < acc+seg_len[k]){ seg=k; loc=pos-acc; break; }
            acc += seg_len[k];
        }
        if(seg<0){ seg=7; loc=seg_len[7]; }

        float px_f, py_f;
        switch(seg){
        /* top edge: left → right */
        case 0: px_f = x0+rf+loc;               py_f = y0;              break;
        /* top-right arc: corner center (x0+W-rf, y0+rf) */
        case 1: { float ang = loc/rf;
            px_f = x0+W-rf + sinf(ang)*rf;
            py_f = y0+rf   - cosf(ang)*rf; }     break;
        /* right edge: top → bottom */
        case 2: px_f = x0+W;                    py_f = y0+rf+loc;       break;
        /* bottom-right arc: corner center (x0+W-rf, y0+H-rf) */
        case 3: { float ang = loc/rf;
            px_f = x0+W-rf + cosf(ang)*rf;
            py_f = y0+H-rf + sinf(ang)*rf; }     break;
        /* bottom edge: right → left */
        case 4: px_f = x0+W-rf-loc;             py_f = y0+H;            break;
        /* bottom-left arc: corner center (x0+rf, y0+H-rf) */
        case 5: { float ang = loc/rf;
            px_f = x0+rf   - sinf(ang)*rf;
            py_f = y0+H-rf + cosf(ang)*rf; }     break;
        /* left edge: bottom → top */
        case 6: px_f = x0;                      py_f = y0+H-rf-loc;     break;
        /* top-left arc: corner center (x0+rf, y0+rf) */
        case 7: { float ang = loc/rf;
            px_f = x0+rf - cosf(ang)*rf;
            py_f = y0+rf - sinf(ang)*rf; }       break;
        default: px_f=x0; py_f=y0;              break;
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
          if(t>0.85f || (cur_theme == 8 && t>0.70f)){ sc_(c2);
              SDL_RenderDrawPoint(ren,px+1,py);
              SDL_RenderDrawPoint(ren,px,py+1); } }

        if(t>0.35f){
            float gt=(t-0.35f)/0.65f;
            float glow_str = (cur_theme == 8) ? 200.f : 140.f;
            Uint8 ga=(Uint8)(gt*gt*srch_glow*glow_str);
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
            if(cur_theme == 8){
                /* Galaxy: wider sparkle cross at the comet head */
                Uint8 fa2=(Uint8)(srch_glow*180.f);
                C4 fl2={255,220,255,fa2}; sc_(fl2);
                SDL_RenderDrawPoint(ren,px-2,py);
                SDL_RenderDrawPoint(ren,px+2,py);
                SDL_RenderDrawPoint(ren,px,py-2);
                SDL_RenderDrawPoint(ren,px,py+2);
                SDL_RenderDrawPoint(ren,px-2,py-2);
                SDL_RenderDrawPoint(ren,px+2,py-2);
                SDL_RenderDrawPoint(ren,px-2,py+2);
                SDL_RenderDrawPoint(ren,px+2,py+2);
            }
        }
    }

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* ═══════════════════════════════════════════════════════════════
   DRAW
   ═══════════════════════════════════════════════════════════════ */

/* ── Wu antialiased line — smooth 1.8px stroke ──────────────────────
   Xiaolin Wu's algorithm: for each step along the major axis, two
   adjacent pixels are blended with complementary alpha so the line
   looks smooth at any angle.  No dependencies beyond SDL_RenderDrawPoint.
   ─────────────────────────────────────────────────────────────────── */
static void wu_pt(int x,int y,float br,C4 c){
    Uint8 a=(Uint8)(c.a*br);
    if(a<2) return;
    C4 p={c.r,c.g,c.b,a}; sc_(p);
    SDL_RenderDrawPoint(ren,x,y);
}
static void rline(int x0,int y0,int x1,int y1,C4 c){
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    int steep=abs(y1-y0)>abs(x1-x0);
    if(steep){ int t=x0;x0=y0;y0=t; t=x1;x1=y1;y1=t; }
    if(x0>x1){ int t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
    float dx=(float)(x1-x0), dy=(float)(y1-y0);
    float grad=(dx==0.f)?1.f:dy/dx;
    /* start endpoint */
    float xend=(float)x0, yend=(float)y0+grad*(xend-(float)x0);
    float xgap=1.f; float xpxl1=(float)x0; float ypxl1=floorf(yend);
    float frac1=yend-ypxl1;
    if(steep){ wu_pt((int)ypxl1,(int)xpxl1,xgap*(1.f-frac1),c);
               wu_pt((int)ypxl1+1,(int)xpxl1,xgap*frac1,c); }
    else     { wu_pt((int)xpxl1,(int)ypxl1,xgap*(1.f-frac1),c);
               wu_pt((int)xpxl1,(int)ypxl1+1,xgap*frac1,c); }
    float intery=yend+grad;
    /* end endpoint */
    xend=(float)x1; yend=(float)y1+grad*(xend-(float)x1);
    float xpxl2=(float)x1;
    /* main loop */
    for(int x=(int)xpxl1+1;x<=(int)xpxl2-1;x++){
        float iy=floorf(intery), fr=intery-iy;
        if(steep){ wu_pt((int)iy,  x, 1.f-fr, c);
                   wu_pt((int)iy+1,x, fr,      c); }
        else     { wu_pt(x,(int)iy,   1.f-fr, c);
                   wu_pt(x,(int)iy+1, fr,      c); }
        intery+=grad;
    }
    /* end cap */
    frac1=yend-floorf(yend);
    if(steep){ wu_pt((int)floorf(yend),  (int)xpxl2,xgap*(1.f-frac1),c);
               wu_pt((int)floorf(yend)+1,(int)xpxl2,xgap*frac1,c); }
    else     { wu_pt((int)xpxl2,(int)floorf(yend),  xgap*(1.f-frac1),c);
               wu_pt((int)xpxl2,(int)floorf(yend)+1,xgap*frac1,c); }
}

static void ric(const char *cp, int cx, int cy, C4 col);

static void draw_tbbtn(TBBtnType type, int cx2, float ht, float fl){
    int cy  = TITLE_H / 2;
    int bw  = TB_BTN_W;   /* full button width  */
    int bh  = TITLE_H;    /* full button height */
    int bx  = cx2 - bw/2; /* left edge          */

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* Hover background — full-height rectangle, Windows 11 style */
    if(ht > 0.005f || fl > 0.005f){
        C4 bg;
        if(type == TB_CLOSE_BTN){
            /* Red close: hover fades in, flash briefly brightens */
            Uint8 ha = (Uint8)((ht + fl * 0.4f) * 240);
            bg = MK4(196, 43, 28, ha);
        } else {
            Uint8 ha = (Uint8)((ht * 38) + fl * 60);
            bg = MK4(255, 255, 255, ha);
        }
        SDL_Rect r = {bx, 0, bw, bh};
        sc_(bg); SDL_RenderFillRect(ren, &r);
    }

    /* Icon — dim white at rest, full white on hover/flash */
    Uint8 ia = (Uint8)(110 + (ht + fl * 0.5f) * 145);
    if(ia > 255) ia = 255;
    C4 ic = MK4(255, 255, 255, ia);

    /* Use Material Symbols if loaded, else hand-drawn fallback */
    /* close=U+E5CD, minimize=U+E15B, fullscreen=U+E5D0, fullscreen_exit=U+E5D1 */
    if(f_icon){
        const char *cp=NULL;
        switch(type){
        case TB_CLOSE_BTN: cp="\xEE\x97\x8D"; break; /* close          U+E5CD */
        case TB_MIN_BTN:   cp="\xEE\x85\x9B"; break; /* remove         U+E15B */
        case TB_MAX_BTN:   cp=win_maximized?"\xEE\x97\x91":"\xEE\x97\x90"; break;
                           /* fullscreen_exit U+E5D1 : fullscreen U+E5D0 */
        }
        if(cp) ric(cp, cx2, cy, ic);
    } else {
    int s = 5;
    switch(type){
    case TB_CLOSE_BTN:
        rline(cx2-s, cy-s, cx2+s, cy+s, ic);
        rline(cx2+s, cy-s, cx2-s, cy+s, ic);
        break;
    case TB_MAX_BTN:
        if(win_maximized){
            C4 back = ic; back.a = (Uint8)(ic.a * 0.45f);
            frr_aa(cx2-s,   cy-s+2, s*2-2, s*2-2, 1, back);
            frr_aa(cx2-s+2, cy-s,   s*2-2, s*2-2, 1, ic);
        } else {
            frr_aa(cx2-s, cy-s, s*2, s*2, 1, ic);
        }
        break;
    case TB_MIN_BTN:
        rline(cx2-s, cy, cx2+s, cy, ic);
        break;
    }
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void draw_sort_dropdown(void){
    if(sort_dd_anim < 0.01f) return;
    static const char *slbl[5]={"A \xe2\x86\x91 Z","Z \xe2\x86\x93 A","Newest","Oldest","Top Rated"};
    float a = sort_dd_anim;
    int total_h = 5*DD_ITEM_H + 6;
    int visible_h = (int)(total_h * a);
    if(visible_h < 2) return;

    /* clip to animated height — start from TITLE_H so it overlaps nothing above */
    SDL_Rect clip = {TC_X0-2, TITLE_H, DD_W+4, visible_h+6};
    SDL_RenderSetClipRect(ren, &clip);

    /* dropdown background — fully opaque dark panel */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    C4 dbg = C_TBAR; dbg.a = 255;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    C4 dbc = C_ACC;  dbc.a = (Uint8)(180*a);
    bfrr_aa(TC_X0, TITLE_H+1, DD_W, total_h+2, R_MD, BRD_T, dbc, dbg);

    /* items — sliding pill follows sort_ind_f, same as tab/sort pill */
    {
        /* draw the sliding selection pill */
        float pill_y = TITLE_H+4 + sort_ind_f * DD_ITEM_H;
        C4 pill = {C_ACC.r, C_ACC.g, C_ACC.b, (Uint8)(180*a)};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        frr_aa(TC_X0+3, (int)(pill_y+0.5f)+2, DD_W-6, DD_ITEM_H-4, 5, pill);
    }
    for(int i=0;i<5;i++){
        int iy = TITLE_H+4+i*DD_ITEM_H;
        float hv = sort_item_hov[i];
        int act = (sort_mode==(SortMode)i);
        int ty = iy+(DD_ITEM_H-TTF_FontHeight(f14))/2;

        if(!act && hv > 0.01f){
            C4 hi = {255,255,255,(Uint8)(hv*30*a)};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(TC_X0+3, iy+2, DD_W-6, DD_ITEM_H-4, 5, hi);
        }

        C4 lc = act ? MK4(255,255,255,(Uint8)(255*a))
                    : lerpc(C_DIM, C_TXT, hv*0.7f);
        if(!act) lc.a = (Uint8)(lc.a * a);

        /* item icon: sort,sort,calendar,history,grade */
        static const char *dd_icons[5]={
            "\xEE\x85\xA4", /* sort     U+E164 */
            "\xEE\x85\xA4", /* sort     U+E164 */
            "\xEE\xA4\xB5", /* today    U+E935 */
            "\xEE\xA2\x89", /* history  U+E889 */
            "\xEE\xA2\x85", /* grade    U+E885 */
        };
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        int txt_x = TC_X0+8;
        if(f_icon){
            int ih=TTF_FontHeight(f_icon);
            int iw=txw_(f_icon,dd_icons[i]);
            rtx(f_icon,dd_icons[i],TC_X0+6,iy+(DD_ITEM_H-ih)/2,lc);
            txt_x=TC_X0+6+iw+4;
        }
        { int avail_dw = TC_X0+DD_W-4 - txt_x;
          if(avail_dw > 0) rtxclip(f14, slbl[i], txt_x, ty, avail_dw, lc); }
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren, NULL);
}

static void draw_theme_dropdown(void){
    if(theme_dd_anim < 0.01f) return;
    float a = theme_dd_anim;
    int total_h = N_THEMES*TD_ITEM_H + 6;
    int visible_h = (int)(total_h * a);
    if(visible_h < 2) return;

    /* clip to animated height — start from TITLE_H so it overlaps nothing above */
    SDL_Rect clip = {TD_BTN_X_-2, TITLE_H, TD_W+4, visible_h+6};
    SDL_RenderSetClipRect(ren, &clip);

    /* dropdown background — fully opaque dark panel */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    C4 dbg = C_TBAR; dbg.a = 255;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    C4 dbc = C_ACC;  dbc.a = (Uint8)(180*a);
    bfrr_aa(TD_BTN_X_, TITLE_H+1, TD_W, total_h+2, R_MD, BRD_T, dbc, dbg);

    /* sliding selection pill */
    {
        float pill_y = TITLE_H+4 + theme_ind_f * TD_ITEM_H;
        C4 pill = {C_ACC.r, C_ACC.g, C_ACC.b, (Uint8)(180*a)};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        frr_aa(TD_BTN_X_+3, (int)(pill_y+0.5f)+2, TD_W-6, TD_ITEM_H-4, 5, pill);
    }
    for(int i=0;i<N_THEMES;i++){
        int iy = TITLE_H+4+i*TD_ITEM_H;
        float hv = theme_item_hov[i];
        int act = (i==cur_theme);
        int ty = iy+(TD_ITEM_H-TTF_FontHeight(f14))/2;

        if(!act && hv > 0.01f){
            C4 hi = {255,255,255,(Uint8)(hv*30*a)};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(TD_BTN_X_+3, iy+2, TD_W-6, TD_ITEM_H-4, 5, hi);
        }

        C4 lc = act ? MK4(255,255,255,(Uint8)(255*a))
                    : lerpc(C_DIM, C_TXT, hv*0.7f);
        if(!act) lc.a = (Uint8)(lc.a * a);

        /* colour dot swatch */
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        C4 dot = act ? MK4(lc.r,lc.g,lc.b,lc.a) : THEMES[i].acc;
        frr_aa(TD_BTN_X_+8, iy+(TD_ITEM_H-8)/2, 8, 8, 4, dot);
        { int avail_tw = TD_BTN_X_+TD_W-6 - (TD_BTN_X_+20);
          if(avail_tw > 0) rtxclip(f14, THEMES[i].name, TD_BTN_X_+20, ty, avail_tw, lc); }
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren, NULL);
}

/* ── Genre filter dropdown ─────────────────────────────────────────────── */
static void draw_genre_dropdown(void){
    /* only render while the CMD panel itself is open or still animating */
    if(!cmd_open && cmd_anim < 0.05f) return;
    if(genre_dd_anim < 0.01f) return;
    float a = genre_dd_anim;
    /* anchor set by draw_cmd_panel each frame */
    int ax = genre_dd_ax;
    int ay = genre_dd_ay;
    /* max visible items — clamp to remaining window height below anchor */
    int max_vis = (win_h - ay - 12) / DD_ITEM_H;
    if(max_vis < 3) max_vis = 3;
    int n_vis   = n_genres < max_vis ? n_genres : max_vis;
    int panel_h = n_vis   * DD_ITEM_H + 6;
    int visible_h = (int)(panel_h * a);
    if(visible_h < 2) return;

    SDL_Rect clip = {ax-2, ay, GENRE_DD_W+4, visible_h+6};
    SDL_RenderSetClipRect(ren, &clip);

    C4 dbg = C_TBAR; dbg.a = 255;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    C4 dbc = C_ACC;  dbc.a = (Uint8)(180*a);
    bfrr_aa(ax, ay+1, GENRE_DD_W, panel_h+2, R_MD, BRD_T, dbc, dbg);

    /* active-item pills */
    for(int i=0;i<n_vis;i++){
        if(genre_is_active(genre_list[i])){
            float pill_y = ay+4 + (float)i * DD_ITEM_H;
            C4 pill = {C_ACC.r, C_ACC.g, C_ACC.b, (Uint8)(180*a)};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(ax+3, (int)(pill_y+0.5f)+2, GENRE_DD_W-6, DD_ITEM_H-4, 5, pill);
        }
    }

    int fh_lbl = TTF_FontHeight(f14);
    for(int i=0;i<n_vis;i++){
        int iy  = ay+4+i*DD_ITEM_H;
        float hv = genre_item_hov[i];
        int act  = genre_is_active(genre_list[i]);
        int ty2  = iy+(DD_ITEM_H-fh_lbl)/2;

        if(!act && hv>0.01f){
            C4 hi={255,255,255,(Uint8)(hv*30*a)};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(ax+3, iy+2, GENRE_DD_W-6, DD_ITEM_H-4, 5, hi);
        }

        C4 lc = act ? MK4(255,255,255,(Uint8)(255*a))
                    : lerpc(C_DIM, C_TXT, hv*0.7f);
        if(!act) lc.a = (Uint8)(lc.a * a);

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        C4 swatch = gcol(genre_list[i]); swatch.a = (Uint8)(200*a);
        frr_aa(ax+8, iy+(DD_ITEM_H-8)/2, 8, 8, 4, swatch);

        int txt_x = ax+20;
        char cnt[8]; snprintf(cnt,sizeof(cnt),"%d",genre_counts[i]);
        int cnt_w = txw_(f12,cnt)+6;
        int avail_gw = ax+GENRE_DD_W-4 - txt_x - cnt_w;
        if(avail_gw>0) rtxclip(f14, genre_list[i], txt_x, ty2, avail_gw, lc);

        if(genre_counts[i]>0){
            C4 cc = act ? MK4(255,255,255,(Uint8)(180*a)) : lerpc(C_DIM,C_SUB,hv*0.5f);
            cc.a=(Uint8)(cc.a*a);
            rtx(f12, cnt, ax+GENRE_DD_W-cnt_w+2, iy+(DD_ITEM_H-TTF_FontHeight(f12))/2, cc);
        }
    }
    if(n_vis < n_genres){
        int hy = ay + panel_h;
        C4 hint = C_DIM; hint.a = (Uint8)(120*a);
        rtxcen(f12,"▾ more", ax, hy-TTF_FontHeight(f12)-2, GENRE_DD_W, TTF_FontHeight(f12), hint);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren, NULL);
}

static void draw_titlebar_dots(void);
static void draw_cmd_panel(void); /* forward decl */
static void draw_titlebar(void){
    /* Title bar background */
    { C4 c=C_TITLE; c.a=170; fblend(0,0,win_w,TITLE_H,c); }
    {
        C4 tl=C_ACC; tl.a=120;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(tl);
        SDL_RenderDrawLine(ren,0,TITLE_H-1,win_w,TITLE_H-1);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    /* ── Command Center trigger button ── */
    {
        float hv  = cmd_btn_hov;
        float fl0 = tc_fl[0];
        C4 bg  = lerpc(C_BG, C_ACC, 0.18f*hv + 0.12f*fl0); bg.a = 255;
        C4 bc  = lerpc(C_SEP, C_ACC, hv*0.9f + fl0*0.2f);
        bc.a   = (Uint8)(80 + hv*140 + fl0*30);
        /* glow ring when open */
        if(cmd_open || cmd_anim > 0.05f){
            C4 ring = C_ACC; ring.a = (Uint8)(140*cmd_anim);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            bfrr_aa(CMD_BTN_X-1, CMD_BTN_Y-1, CMD_BTN_W+2, CMD_BTN_H+2,
                    R_SM+1, 1, ring, (C4){0,0,0,0});
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }
        bfrr_aa(CMD_BTN_X, CMD_BTN_Y, CMD_BTN_W, CMD_BTN_H, R_SM, BRD_T, bc, bg);
        C4 lc = C_TXT; lc.a = (Uint8)(140 + hv*115);
        /* tune / settings icon U+E429 */
        if(f_icon){
            ric("\xEE\x90\xA9", CMD_BTN_X + CMD_BTN_W/2, CMD_BTN_Y + CMD_BTN_H/2, lc);
        } else {
            /* fallback: three horizontal lines with dots (settings icon) */
            sc_(lc);
            int cx2=CMD_BTN_X+CMD_BTN_W/2, cy2=CMD_BTN_Y+CMD_BTN_H/2;
            int lw2=12;
            SDL_RenderDrawLine(ren,cx2-lw2/2,cy2-4,cx2+lw2/2,cy2-4);
            SDL_RenderDrawLine(ren,cx2-lw2/2,cy2,  cx2+lw2/2,cy2  );
            SDL_RenderDrawLine(ren,cx2-lw2/2,cy2+4,cx2+lw2/2,cy2+4);
        }
    }

    /* Window control buttons (Close / Max / Min) */
    draw_tbbtn(TB_CLOSE_BTN, TB_CX, tb_ch, tb_close_fl);
    draw_tbbtn(TB_MAX_BTN,   TB_MX, tb_mh, tb_max_fl);
    draw_tbbtn(TB_MIN_BTN,   TB_NX, tb_nh, tb_min_fl);
}

/* ── Header ───────────────────────────────────────────────────── */
static void draw_hdr(void){
    { C4 c=C_HDR; c.a=170; fblend(0,HDR_Y,win_w,HDR_H,c); }
    rtx(f18,"GAME CATALOGUE",18,HDR_Y+(HDR_H-TTF_FontHeight(f18))/2,C_TXT);

    /* search bar: matches toolbar button language —
       inactive = C_BTNI bg + C_SEP border,
       active/focused = C_SRCHA bg + C_ACC border               */
    /* center search bar between title text and right window edge */
    {
        int title_right = 18 + txw_(f18,"GAME CATALOGUE") + 24;
        int avail_right = win_w - 16;
        int avail = avail_right - title_right;
        int sw2 = avail > 420 ? 420 : (avail < 80 ? 80 : avail);
        sr_x = title_right + (avail - sw2) / 2;
        sr_w = sw2;
    }
    C4 bc=lerpc(C_SEP, C_ACC, s_on?1.f:0.f);
    C4 ic=s_on?C_SRCHA:C_BTNI;
    bfrr_aa(sr_x,SR_Y,sr_w,SR_H,SR_R,BRD_T,bc,ic);

    draw_srch_spin_fx();

    {
        int tx_off = sr_x+12;
        int tx_mw  = sr_w - 24;
        int ty     = SR_Y+(SR_H-TTF_FontHeight(f14))/2;
        int fh     = TTF_FontHeight(f14);

        if(!*srch&&!s_on){
            /* search icon U+E8B6 before placeholder */
            int ty2=SR_Y+(SR_H-TTF_FontHeight(f14))/2;
            if(f_icon){
                int ih=TTF_FontHeight(f_icon);
                C4 ic2=C_DIM;
                rtx(f_icon,"\xEE\xA2\xB6",tx_off,SR_Y+(SR_H-ih)/2,ic2);
                int iw=txw_(f_icon,"\xEE\xA2\xB6");
                rtxclip(f14,"Search games...",tx_off+iw+4,ty2,tx_mw-iw-4,C_DIM);
            } else {
                rtxclip(f14,"Search games...",tx_off,ty2,tx_mw,C_DIM);
            }
            /* keyboard shortcut hint pill — "Ctrl+F" faintly on the right of the bar */
            { int fh_=TTF_FontHeight(f12);
              const char *hint_ = "Ctrl+F";
              int pw_=txw_(f12,hint_)+10, ph_=fh_+4;
              int px_=sr_x+sr_w-pw_-8, py_=SR_Y+(SR_H-ph_)/2;
              C4 pbg_={C_SEP.r,C_SEP.g,C_SEP.b,45};
              C4 pbd_={C_DIM.r,C_DIM.g,C_DIM.b,65};
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
              bfrr_aa(px_,py_,pw_,ph_,R_SM,1,pbd_,pbg_);
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
              C4 htc_=C_DIM; htc_.a=140;
              rtxcen(f12,hint_,px_,py_,pw_,ph_,htc_);
            }
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
        if(0){ /* dots replaced by theme dropdown button */
            int fh12 = TTF_FontHeight(f12);
            int dot_block = (N_THEMES-1)*TDOT_STEP + TDOT_R*2;
            int bx0 = dots_x0 - TDOT_R;

            {
                int lw2=txw_(f12,"THEME");
                rtx(f12,"THEME", bx0+(dot_block-lw2)/2,
                    dots_cy-TDOT_R-4-fh12, C_TXT);
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
                int ring_r = TDOT_R + (int)(galaxy_dot_f * 3.f);
                frr_aa(rx-ring_r-1,dots_cy+ryo-ring_r-1,(ring_r+1)*2,(ring_r+1)*2,ring_r+1,ring);
            }
            for(int i=0;i<N_THEMES;i++){
                int cx  = dots_x0+i*TDOT_STEP;
                float hf= tdot_hov[i];
                float bn= tdot_bounce[i];
                int yoff= (bn>0.f)?(int)(-sinf(bn*(float)M_PI)*7.f):0;
                C4 ac=THEMES[i].acc;

                /* Galaxy extra effects */
                if(i==8) draw_galaxy_dot_fx(cx, dots_cy+yoff, TDOT_R, galaxy_dot_f, hf, 1.0f, dt_);

                /* Base dot — always drawn at TDOT_R, same as every other dot */
                if(hf>0.02f && i!=cur_theme){
                    C4 hover={ac.r,ac.g,ac.b,(Uint8)(hf*90)};
                    frr_aa(cx-TDOT_R-1,dots_cy+yoff-TDOT_R-1,(TDOT_R+1)*2,(TDOT_R+1)*2,TDOT_R+1,hover);
                }
                float br=0.45f+0.55f*hf;
                C4 fc={(Uint8)(ac.r*br),(Uint8)(ac.g*br),(Uint8)(ac.b*br),255};
                frr_aa(cx-TDOT_R,dots_cy+yoff-TDOT_R,TDOT_R*2,TDOT_R*2,TDOT_R,fc);
                if(i==cur_theme){
                    int wr = (i==8) ? 3+(int)(galaxy_dot_f) : 3;
                    C4 w={255,255,255,(Uint8)(180+(i==8?galaxy_dot_f*40.f:20.f))};
                    frr_aa(cx-wr,dots_cy+yoff-wr,wr*2,wr*2,wr,w);
                }
            }
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

            {
                const char *nm=THEMES[cur_theme].name;
                int nw=txw_(f12,nm);
                rtx(f12,nm, bx0+(dot_block-nw)/2, dots_cy+TDOT_R+4, C_TXT);
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
        /* selection ring stays at r2 — expanding it causes it to overlap neighbours */
        frr_aa(rx-r2-1,dots_cy+ryo-r2-1,(r2+1)*2,(r2+1)*2,r2+1,ring);
    }
    for(int i=0;i<N_THEMES;i++){
        int cx=dots_x0+i*TDOT_STEP;
        float hf=tdot_hov[i];
        float bn=tdot_bounce[i];
        int yoff=(bn>0.f)?(int)(-sinf(bn*(float)M_PI)*5.f):0;
        C4 ac=THEMES[i].acc;

        /* Galaxy extra effects — fluid orbital animation */
        if(i==8) draw_galaxy_dot_fx(cx, dots_cy+yoff, r2, galaxy_dot_f, hf, 1.0f, dt_);

        /* Base dot — always at r2, same as every other dot */
        if(hf>0.02f && i!=cur_theme){
            C4 hover={ac.r,ac.g,ac.b,(Uint8)(hf*90)};
            frr_aa(cx-r2-1,dots_cy+yoff-r2-1,(r2+1)*2,(r2+1)*2,r2+1,hover);
        }
        float br=0.45f+0.55f*hf;
        C4 fc={(Uint8)(ac.r*br),(Uint8)(ac.g*br),(Uint8)(ac.b*br),255};
        frr_aa(cx-r2,dots_cy+yoff-r2,r2*2,r2*2,r2,fc);
        if(i==cur_theme){
            int wr=(i==8)?2+(int)(galaxy_dot_f):2;
            C4 w={255,255,255,(Uint8)(170+(i==8?galaxy_dot_f*40.f:10.f))};
            frr_aa(cx-wr,dots_cy+yoff-wr,wr*2,wr*2,wr,w);
        }
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* ══════════════════════════════════════════════════════════════════════════
   COMMAND CENTER PANEL
   Large modal overlay — replaces all individual title-bar action buttons.
   Opens / closes with a slide-down clip animation identical to the sort
   dropdown.  Content fades in once a > 0.3 so the clip completes first.
   ══════════════════════════════════════════════════════════════════════════ */
static void draw_cmd_panel(void){
    if(cmd_anim < 0.005f) return;

    float a   = cmd_anim;
    int   ox  = CMD_BTN_X;
    int   oy  = CMD_PANEL_OY;
    int   ow  = CMD_OW;
    int   oh  = CMD_OH;
    int   visible_h = (int)(oh * a);
    if(visible_h < 2) return;

    int fh14 = TTF_FontHeight(f14);
    int fh12 = TTF_FontHeight(f12);

    /* ── Dim backdrop — full window, scaled by anim ── */
    { C4 dim={0,0,0,(Uint8)(120*a)};
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(dim); SDL_Rect r={0,0,win_w,win_h}; SDL_RenderFillRect(ren,&r);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    /* ── Clip to animated slide-down height ── */
    SDL_Rect clip={ox-2, oy, ow+4, visible_h+4};
    SDL_RenderSetClipRect(ren, &clip);

    /* ── Panel shell ── */
    { SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      C4 bc=C_ACC; bc.a=(Uint8)(200*a);
      bfrr_aa(ox, oy, ow, oh, R_LG, BRD_T, bc, C_TBAR);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    /* Defer content until panel is sufficiently open */
    if(a < 0.25f){ SDL_RenderSetClipRect(ren,NULL); return; }

    /* ── Layout helpers ── */
    int cx0   = ox + CMD_PAD;           /* content left edge   */
    int px0   = cx0 + CMD_LBL_W;       /* pill/control left   */
    int pw_av = ow - CMD_PAD*2 - CMD_LBL_W; /* available pill width */

    /* Section label helper macro */
    #define SEC_LBL_(lbl, ry_) do { \
        C4 _lc = C_SUB; _lc.a = (Uint8)(200*a); \
        rtxclip(f12, (lbl), cx0, (ry_) + (CMD_SEC_H - fh12)/2, \
                CMD_LBL_W-4, _lc); \
    } while(0)

    /* Separator helper macro */
    #define SEC_SEP_(ry_) do { \
        C4 _sc = C_SEP; _sc.a = (Uint8)(55*a); \
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND); sc_(_sc); \
        SDL_RenderDrawLine(ren, ox+CMD_PAD/2, (ry_), \
                           ox+ow-CMD_PAD/2, (ry_)); \
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); \
    } while(0)

    int ry = oy + CMD_PAD;

    /* ══════════════════════════════════════════════════════════════
       SECTION 0 — Sort
       Five horizontal inline pills. Sliding accent drawn last so
       it cleanly overlaps the outline-only inactive pills.
    ══════════════════════════════════════════════════════════════ */
    SEC_LBL_("SORT", ry);
    {
        static const char *slbls[5]={"A\xe2\x86\x91Z","Z\xe2\x86\x93" "A","Newest","Oldest","Top"};
        int n=5, gap=CMD_PILL_GAP;
        int pw=(pw_av - gap*(n-1)) / n;
        int ph=CMD_PILL_H;
        int py=ry+(CMD_SEC_H-ph)/2;

        /* 1. Inactive pill outlines — no fill, just a faint border */
        for(int i=0;i<n;i++){
            int px2=px0+i*(pw+gap);
            int act=(sort_mode==(SortMode)i);
            if(!act){
                float hv=cmd_sort_hov[i];
                C4 bc2=lerpc(C_SEP,C_ACC,hv*0.4f); bc2.a=(Uint8)((50+hv*70)*a);
                C4 bg2=lerpc(C_BTNI,C_ACC,hv*0.10f); bg2.a=(Uint8)((80+hv*60)*a);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                bfrr_aa(px2,py,pw,ph,R_SM,BRD_T,bc2,bg2);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            }
        }

        /* 2. Sliding accent pill — drawn on top of outlines */
        { float pill_xf = px0 + sort_ind_f*(float)(pw+gap);
          C4 vbg=lerpc(C_BG,C_ACC,0.30f); vbg.a=(Uint8)(255*a);
          C4 vbd=C_ACC; vbd.a=(Uint8)(200*a);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
          bfrr_aa((int)(pill_xf+0.5f),py,pw,ph,R_SM,BRD_T,vbd,vbg);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

        /* 3. Text — all pills, active is bright */
        for(int i=0;i<n;i++){
            int px2=px0+i*(pw+gap);
            int act=(sort_mode==(SortMode)i);
            float hv=cmd_sort_hov[i];
            C4 lc=act?C_TXT:lerpc(C_DIM,C_TXT,hv*0.7f);
            lc.a=(Uint8)(lc.a*a);
            rtxcen(f14,slbls[i],px2,py,pw,ph,lc);
        }
    }

    ry += CMD_SEC_H;
    SEC_SEP_(ry);

    /* ══════════════════════════════════════════════════════════════
       SECTION 1 — Display  (List · Grid · Stats)
       Same three-pass pill rendering as Sort.
       A vertical separator divides view-mode pills from Stats.
    ══════════════════════════════════════════════════════════════ */
    SEC_LBL_("DISPLAY", ry);
    {
        static const char *vlbls[2]={"List","Grid"};
        static const char *vicons[2]={"\xEE\xA3\xAF","\xEE\xA6\xB0"};
        int vgap=CMD_PILL_GAP;
        int vpw=CMD_VIEW_PW, ph=CMD_PILL_H;
        int py=ry+(CMD_SEC_H-ph)/2;
        /* Stats pill sits after a wider gap to create a visual separation */
        int stats_x=px0+2*(vpw+vgap)+vgap+6;
        int spw=CMD_STATS_PW;

        /* 1. Inactive pill outlines for view pills */
        for(int i=0;i<2;i++){
            int px2=px0+i*(vpw+vgap);
            int act=(view_mode==(ViewMode)i);
            if(!act){
                float hv=tc_view_hov[i];
                C4 bc2=lerpc(C_SEP,C_ACC,hv*0.4f); bc2.a=(Uint8)((50+hv*70)*a);
                C4 bg2=lerpc(C_BTNI,C_ACC,hv*0.10f); bg2.a=(Uint8)((80+hv*60)*a);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                bfrr_aa(px2,py,vpw,ph,R_SM,BRD_T,bc2,bg2);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            }
        }

        /* 2. Sliding accent pill for view mode */
        { float pill_xf=px0+view_ind_f*(float)(vpw+vgap);
          C4 vbg=lerpc(C_BG,C_ACC,0.30f); vbg.a=(Uint8)(255*a);
          C4 vbd=C_ACC; vbd.a=(Uint8)(200*a);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
          bfrr_aa((int)(pill_xf+0.5f),py,vpw,ph,R_SM,BRD_T,vbd,vbg);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

        /* 3. View pill text */
        for(int i=0;i<2;i++){
            int px2=px0+i*(vpw+vgap);
            int act=(view_mode==(ViewMode)i);
            float hv=tc_view_hov[i];
            C4 lc=act?C_TXT:lerpc(C_DIM,C_TXT,hv*0.7f);
            lc.a=(Uint8)(lc.a*a);
            if(f_icon){
                TxEntry *te=tc_get(f_icon,vicons[i]);
                if(te){
                    int iw=te->w, tw2=txw_(f14,vlbls[i]);
                    int total=iw+5+tw2, lx=px2+(vpw-total)/2, icy=py+ph/2;
                    SDL_SetTextureColorMod(te->tex,lc.r,lc.g,lc.b);
                    SDL_SetTextureAlphaMod(te->tex,lc.a);
                    SDL_Rect dr={lx,icy-te->h/2,te->w,te->h};
                    SDL_RenderCopy(ren,te->tex,NULL,&dr);
                    rtx(f14,vlbls[i],lx+iw+5,icy-fh14/2,lc);
                } else { rtxcen(f14,vlbls[i],px2,py,vpw,ph,lc); }
            } else { rtxcen(f14,vlbls[i],px2,py,vpw,ph,lc); }
        }

        /* Thin vertical separator between view pills and Stats */
        { C4 sv=C_SEP; sv.a=(Uint8)(60*a);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND); sc_(sv);
          int sx=stats_x-5;
          SDL_RenderDrawLine(ren,sx,py+4,sx,py+ph-4);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

        /* Stats pill — treated as a separate toggle (no sliding pill) */
        { int act=(cur_tab==T_STATS);
          float hv=tc_stats_hov, fl4=tc_fl[4];
          C4 bg2,bc2;
          if(act){
              bg2=lerpc(C_BG,C_ACC,0.30f+fl4*0.1f); bg2.a=(Uint8)(255*a);
              bc2=C_ACC; bc2.a=(Uint8)(200*a);
          } else {
              C4 bc3=lerpc(C_SEP,C_ACC,hv*0.4f); bc3.a=(Uint8)((50+hv*70)*a);
              C4 bg3=lerpc(C_BTNI,C_ACC,hv*0.10f); bg3.a=(Uint8)((80+hv*60)*a);
              bg2=bg3; bc2=bc3;
          }
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
          bfrr_aa(stats_x,py,spw,ph,R_SM,BRD_T,bc2,bg2);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
          C4 lc=act?lerpc(C_TXT,MK4(255,255,255,255),fl4*0.18f):lerpc(C_DIM,C_TXT,hv*0.7f);
          lc.a=(Uint8)(lc.a*a);
          const char *slbl2=act?"Hide Stats":"Show Stats";
          if(f_icon){
              TxEntry *te=tc_get(f_icon,"\xEE\x89\xAB");
              if(te){
                  int iw=te->w, tw2=txw_(f14,slbl2);
                  int total=iw+5+tw2, lx=stats_x+(spw-total)/2, icy=py+ph/2;
                  SDL_SetTextureColorMod(te->tex,lc.r,lc.g,lc.b);
                  SDL_SetTextureAlphaMod(te->tex,lc.a);
                  SDL_Rect dr={lx,icy-te->h/2,te->w,te->h};
                  SDL_RenderCopy(ren,te->tex,NULL,&dr);
                  rtx(f14,slbl2,lx+iw+5,icy-fh14/2,lc);
              } else { rtxcen(f14,slbl2,stats_x,py,spw,ph,lc); }
          } else { rtxcen(f14,slbl2,stats_x,py,spw,ph,lc); }
        }
    }

    ry += CMD_SEC_H;
    SEC_SEP_(ry);

    /* ══════════════════════════════════════════════════════════════
       SECTION 2 — Genre filter
       Button that opens the existing genre dropdown below the panel.
       Active filter chips shown to the right of the button.
    ══════════════════════════════════════════════════════════════ */
    SEC_LBL_("GENRE", ry);
    {
        int bw=CMD_GENRE_BTN_W, ph=CMD_PILL_H;
        int py=ry+(CMD_SEC_H-ph)/2;
        float hv=genre_btn_hov;
        float active_t=n_filt_genres>0?0.35f:0.f;
        C4 bg2=lerpc(C_BTNI,C_ACC,0.14f*hv+0.10f*active_t); bg2.a=(Uint8)(255*a);
        C4 bc2=lerpc(C_SEP,C_ACC,hv*0.8f+active_t*0.5f);
        bc2.a=(Uint8)((80+hv*100)*a);
        bfrr_aa(px0,py,bw,ph,R_SM,BRD_T,bc2,bg2);

        /* store dropdown anchor for draw_genre_dropdown */
        genre_dd_ax = px0;
        genre_dd_ay = py + ph + 2;

        /* button label */
        char glbl[48];
        if(n_filt_genres==0) strncpy(glbl,"Filter genre",47);
        else snprintf(glbl,sizeof(glbl),"Genre (%d)",n_filt_genres);
        C4 gc=lerpc(C_DIM,C_TXT,0.3f+hv*0.7f); gc.a=(Uint8)(gc.a*a);
        rtxclip(f12,glbl,px0+9,py+(ph-fh12)/2,bw-22,gc);
        /* chevron */
        { int chx=px0+bw-12, chy=py+ph/2;
          float rot=genre_dd_anim;
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
          C4 cc=lerpc(C_DIM,C_TXT,hv); cc.a=(Uint8)(cc.a*a);
          int cy2c=chy+(int)((1.f-rot)*2.f-1.f);
          int arm=3; float flip=1.f-2.f*rot;
          rline(chx-arm,cy2c-(int)(flip*arm/2),chx,cy2c+(int)(flip*arm/2),cc);
          rline(chx+arm,cy2c-(int)(flip*arm/2),chx,cy2c+(int)(flip*arm/2),cc);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }
    }

    ry += CMD_SEC_H;
    SEC_SEP_(ry);

    /* ═══════════════════════════════════════════════════════════════
       SECTION 3 — Theme
       Nine coloured dots, selection ring, bounce, galaxy effects.
    ══════════════════════════════════════════════════════════════ */
    SEC_LBL_("THEME", ry);
    {
        int r2=7, dstep=TDOT_STEP;
        int dot_x0_=px0+r2;
        int dot_cy_=ry+CMD_SEC_H/2;

        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);

        /* selection ring */
        { int ri0=(int)sel_ring_if; if(ri0>=N_THEMES-1) ri0=N_THEMES-2;
          float bnr=tdot_bounce[ri0];
          int ryo=(bnr>0.f)?(int)(-sinf(bnr*(float)M_PI)*5.f):0;
          int rx=dot_x0_+(int)(sel_ring_if*(float)dstep+0.5f);
          C4 ac=THEMES[cur_theme].acc;
          C4 ring={ac.r,ac.g,ac.b,(Uint8)(220*a)};
          frr_aa(rx-r2-1,dot_cy_+ryo-r2-1,(r2+1)*2,(r2+1)*2,r2+1,ring); }

        for(int i=0;i<N_THEMES;i++){
            int dcx=dot_x0_+i*dstep;
            float hf=tdot_hov[i];
            float bn=tdot_bounce[i];
            int yoff=(bn>0.f)?(int)(-sinf(bn*(float)M_PI)*5.f):0;
            C4 ac=THEMES[i].acc;

            /* Galaxy extra effects — fluid orbital animation */
            if(i==8) draw_galaxy_dot_fx(dcx, dot_cy_+yoff, r2, galaxy_dot_f, hf, a, dt_);

            /* hover halo */
            if(hf>0.02f&&i!=cur_theme){
                C4 hover={ac.r,ac.g,ac.b,(Uint8)(hf*80*a)};
                frr_aa(dcx-r2-1,dot_cy_+yoff-r2-1,(r2+1)*2,(r2+1)*2,r2+1,hover);
            }
            /* base dot */
            float br=0.45f+0.55f*hf;
            C4 fc={(Uint8)(ac.r*br),(Uint8)(ac.g*br),(Uint8)(ac.b*br),(Uint8)(255*a)};
            frr_aa(dcx-r2,dot_cy_+yoff-r2,r2*2,r2*2,r2,fc);
            /* white pip on selected dot */
            if(i==cur_theme){
                int wr=(i==8)?2+(int)(galaxy_dot_f):2;
                C4 w={255,255,255,(Uint8)(170*a)};
                frr_aa(dcx-wr,dot_cy_+yoff-wr,wr*2,wr*2,wr,w);
            }
        }
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

        /* theme name label */
        { int name_x=dot_x0_+N_THEMES*dstep+6;
          C4 nc=C_SUB; nc.a=(Uint8)(180*a);
          rtx(f12,THEMES[cur_theme].name,name_x,dot_cy_-fh12/2,nc); }
    }

    #undef SEC_LBL_
    #undef SEC_SEP_
    SDL_RenderSetClipRect(ren, NULL);
}

/* ── Click handler for inside the CMD panel ──────────────────────────────
   Called from MOUSEBUTTONDOWN when cmd_open and click is within the panel.
   Returns 1 if the click was consumed.                                     */
static void do_tab(int i); /* forward declaration — defined later */
static int handle_cmd_click(int mx, int my){
    int ox=CMD_BTN_X, oy=CMD_PANEL_OY, ow=CMD_OW;
    /* reject if outside panel rect */
    if(!IN_RECT(mx,my,ox,oy,ow,CMD_OH)) return 0;

    int cx0=ox+CMD_PAD, px0=cx0+CMD_LBL_W;
    int pw_av=ow-CMD_PAD*2-CMD_LBL_W;

    /* ── Sort pills ── */
    { int n=5, gap=CMD_PILL_GAP;
      int pw=(pw_av-gap*(n-1))/n, ph=CMD_PILL_H;
      int py=CMD_SORT_RY+(CMD_SEC_H-ph)/2;
      for(int i=0;i<n;i++){
          int px2=px0+i*(pw+gap);
          if(IN_RECT(mx,my,px2,py,pw,ph)){
              if(sort_mode!=(SortMode)i){ sort_mode=(SortMode)i; sfx_sort(); rebuild(); save_d(); }
              tc_fl[0]=1.f;
              return 1;
          }
      } }

    /* ── View pills + Stats (Display row) ── */
    { int vgap=CMD_PILL_GAP;
      int vpw=CMD_VIEW_PW, ph=CMD_PILL_H;
      int py=CMD_VIEW_RY+(CMD_SEC_H-ph)/2;
      /* view pills */
      for(int i=0;i<2;i++){
          int px2=px0+i*(vpw+vgap);
          if(IN_RECT(mx,my,px2,py,vpw,ph)){
              tc_fl[2+i]=1.f;
              if(view_mode!=(ViewMode)i){
                  view_mode=(ViewMode)i; sfx_tab();
                  BADGE_CLOSE_NOW(); scr_tgt=0; scr_f=0; save_d();
              }
              return 1;
          }
      }
      /* stats pill — offset past view pills + extra gap */
      int stats_x=px0+2*(vpw+vgap)+vgap+6;
      if(IN_RECT(mx,my,stats_x,py,CMD_STATS_PW,ph)){
          tc_fl[4]=1.f;
          if(cur_tab==T_STATS){ do_tab((int)prev_tab); sfx_tab(); }
          else { prev_tab=cur_tab; do_tab((int)T_STATS); sfx_tab(); }
          return 1;
      } }
    { int bw=CMD_GENRE_BTN_W, ph=CMD_PILL_H;
      int py=CMD_GENRE_RY+(CMD_SEC_H-ph)/2;
      if(IN_RECT(mx,my,px0,py,bw,ph)){
          genre_dd_open=!genre_dd_open;
          sfx_click();
          return 1;
      } }

    /* ── Theme dots ── */
    { int r2=7, dstep=TDOT_STEP;
      int dot_x0_=px0+r2;
      int dot_cy_=CMD_THEME_RY+CMD_SEC_H/2;
      for(int i=0;i<N_THEMES;i++){
          int dcx=dot_x0_+i*dstep;
          if(mx>=dcx-r2-2&&mx<dcx+r2+2&&my>=dot_cy_-r2-2&&my<dot_cy_+r2+2){
              tdot_bounce[i]=1.f;
              if(i!=cur_theme){ set_theme(i); save_d(); sfx_tab(); }
              return 1;
          }
      } }

    return 1; /* consumed — inside panel even if no control hit */
}

/* ── Tabs ─────────────────────────────────────────────────────── */
static const char *TAB_ICON[N_TABS]={
    "\xEE\xA3\xAF", /* format_list  — All        (confirmed: view toggle) */
    "\xEE\xA1\xA6", /* bookmark     — Wishlist   (confirmed: SICON)       */
    "\xEE\xA2\x89", /* history      — Played     (confirmed: SICON)       */
    "\xEE\x80\xB7", /* play_arrow   — Playing    (confirmed: SICON)       */
    "\xEE\xA1\xAC", /* check_circle — Finished   (confirmed: SICON)       */
    "\xEE\x97\x8D", /* close/cancel — Dropped    (confirmed: SICON)       */
    "\xEE\xA1\xBD", /* favorite     — Favourites (confirmed: SICON)       */
    "\xEE\xA1\xA3", /* autorenew    — Rotation   (confirmed: SICON)       */
};
/* ── Filter chips ─────────────────────────────────────────────────────── */
static void draw_chips(void){
    if(!chip_band) return;
    int cy  = TITLE_H + HDR_H;
    int ih  = CHIP_H - 8;               /* pill inner height                */
    int iy  = cy + (CHIP_H - ih) / 2;  /* pill Y centred in band            */
    int cx  = 10;
    int fh  = TTF_FontHeight(f12);

    /* same background as the tab row — chip band is part of the nav area   */
    { C4 c=C_TBAR; c.a=170; fblend(0, cy, win_w, CHIP_H, c); }

    /* bottom separator matching tab/header dividers                         */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    { C4 sep = C_SEP; sep.a = 70; sc_(sep);
      SDL_RenderDrawLine(ren, 0, cy+CHIP_H-1, win_w, cy+CHIP_H-1); }

    /* × close glyph from f12 — reliable, no icon-font dependency           */
    const char *X_GLYPH = "\xc3\x97";   /* U+00D7 × — in every Latin font  */
    int xw = txw_(f12, X_GLYPH);

    #define DRAW_CHIP(label, hov) do { \
        int lw_  = txw_(f12,(label)); \
        int cw_  = lw_ + xw + 20;    /* 8 left + text + 4 gap + x + 8 right */ \
        float hv_= (hov); \
        /* pill background: C_BTNI tinted toward accent on hover             */ \
        C4 bg_  = lerpc(C_BTNI, C_ACC, 0.14f + hv_*0.12f); bg_.a = 255; \
        C4 brd_ = lerpc(C_SEP,  C_ACC, 0.50f + hv_*0.35f); brd_.a= 200; \
        bfrr_aa(cx, iy, cw_, ih, R_MD, 1, brd_, bg_); \
        /* label text */\
        C4 tc_  = lerpc(C_SUB, C_TXT, 0.35f + hv_*0.65f); \
        rtxclip(f12,(label), cx+8, iy+(ih-fh)/2, lw_+2, tc_); \
        /* × — red tint grows with hover; always readable                    */ \
        C4 xc_  = lerpc(C_SUB, MK4(255,85,85,255), hv_*hv_); \
        rtx(f12, X_GLYPH, cx+8+lw_+4, iy+(ih-fh)/2, xc_); \
        cx += cw_ + 6; \
    } while(0)

    if(n_filt_genres>0){
        /* build a compact label: "Genre: X, Y, Z" */
        char lbl[96]; int pos=snprintf(lbl,sizeof(lbl),"Genre: ");
        for(int gi=0;gi<n_filt_genres&&pos<(int)sizeof(lbl)-2;gi++){
            if(gi>0) pos+=snprintf(lbl+pos,sizeof(lbl)-pos,", ");
            pos+=snprintf(lbl+pos,sizeof(lbl)-pos,"%s",filt_genres[gi]);
        }
        DRAW_CHIP(lbl, chip_genre_hov);
    }
    if(filt_year){
        char lbl[32]; snprintf(lbl,sizeof(lbl),"Year: %d",filt_year);
        DRAW_CHIP(lbl, chip_year_hov);
    }
    #undef DRAW_CHIP

    /* ── "Clear all" button — only when more than one filter is active ── */
    if((n_filt_genres>0) + (filt_year!=0) > 1){
        static float clear_hov = 0.f;
        {   /* hover update */
            int mx_cl,my_cl; SDL_GetMouseState(&mx_cl,&my_cl);
            float cspd_cl = 1.f-powf(0.001f,dt_);
            const char *clbl = "Clear all";
            int clw = txw_(f12,clbl);
            int cw_cl = clw + 16;
            int tgt_cl = (my_cl>=iy&&my_cl<iy+ih&&mx_cl>=cx&&mx_cl<cx+cw_cl)?1:0;
            float diff = (float)tgt_cl - clear_hov;
            if(fabsf(diff)<0.001f) clear_hov=(float)tgt_cl;
            else clear_hov += diff*cspd_cl;
        }
        const char *clbl = "Clear all";
        int clw  = txw_(f12,clbl);
        int cw_cl = clw + 16;
        C4 cbg  = lerpc(C_BTNI, MK4(180,50,50,255), 0.10f + clear_hov*0.15f); cbg.a=255;
        C4 cbrd = lerpc(C_SEP,  MK4(220,80,80,255), 0.40f + clear_hov*0.45f); cbrd.a=200;
        bfrr_aa(cx, iy, cw_cl, ih, R_MD, 1, cbrd, cbg);
        C4 ctc  = lerpc(C_SUB, MK4(255,130,130,255), 0.30f + clear_hov*0.70f);
        rtxcen(f12, clbl, cx, iy, cw_cl, ih, ctc);
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void draw_tabs(void){
    int tw=TAB_W_;
    { C4 c=C_TBAR; c.a=170; fblend(0,TAB_Y,win_w,TAB_H,c); }

    /* sliding active-tab pill — coloured with the tab's own accent */
    if(cur_tab != T_STATS){
        int ix=(int)tab_ix;
        int ai=(int)cur_tab;
        C4 acc = (ai>0 && ai<=(int)T_ROTATION) ? SCOL[ai-1] : C_ACC;
        C4 tab_bg = lerpc(C_BG, acc, 0.15f); tab_bg.a=255;
        C4 border  = acc; border.a = 180;
        bfrr_aa(ix+2, TAB_Y+3, tw-4, TAB_H-3, R_MD, BRD_T, border, tab_bg);
    }

    for(int i=0;i<N_TABS;i++){
        int ti=TAB_ORDER[i];          /* actual TabId for this display slot */
        int tx=TAB_X_(i);
        int act=(cur_tab==(TabId)ti);
        float hv=tab_hov[i];
        float fl=tab_fl[ti];   /* indexed by TabId, same as the writer in hit_tab */
        C4 base_col = (ti>0) ? SCOL[ti-1] : C_ACC;
        C4 tc = lerpc(
            lerpc(lerpc(C_DIM, C_SUB, hv*0.6f), base_col, act?1.f:hv*0.5f),
            MK4(255,255,255,255), fl*0.20f);
        const char *lbl  = (ti==T_ALL) ? "All" : SNAME[ti];
        /* "All" tab icon follows current view mode */
        const char *icon = (ti==T_ALL)
            ? (view_mode==VIEW_GRID ? "\xEE\xA6\xB0" : "\xEE\xA3\xAF")
            : TAB_ICON[ti];
        if(f_icon){
            int fh_ic  = TTF_FontHeight(f_icon);
            int fh_lbl = TTF_FontHeight(f12);
            int block_h = fh_ic + 2 + fh_lbl;
            int iy = TAB_Y + (TAB_H - block_h) / 2;
            int ly = iy + fh_ic + 2;
            int iw = txw_(f_icon, icon);
            rtx(f_icon, icon, tx+(tw-iw)/2, iy, tc);
            rtxcen(f12, lbl, tx, ly, tw, fh_lbl, tc);
        } else {
            rtxcen(f12, lbl, tx, TAB_Y, tw, TAB_H, tc);
        }


    }
}

/* ── Row ──────────────────────────────────────────────────────── */
/* ── Status icon primitives (fallback if icon font not loaded) ──── */
static void draw_status_icon_prim(int j, int cx, int cy, int isz, C4 col){
    int r=isz/2;
    int d=(int)(r*0.71f);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    sc_(col);
    switch(j){
    case S_WISH:{ int w=(r*5)/4,x0=cx-w/2,x1=cx+w/2,y0=cy-r,y1=cy+r,notch=y1-(y1-y0)/4;
        SDL_RenderDrawLine(ren,x0,y0,x1,y0); SDL_RenderDrawLine(ren,x0,y0,x0,y1);
        SDL_RenderDrawLine(ren,x1,y0,x1,y1); SDL_RenderDrawLine(ren,x0,y1,cx,notch);
        SDL_RenderDrawLine(ren,x1,y1,cx,notch); break;}
    case S_PLAYED:{ int px[8]={cx,cx+d,cx+r,cx+d,cx,cx-d,cx-r,cx-d};
        int py[8]={cy-r,cy-d,cy,cy+d,cy+r,cy+d,cy,cy-d};
        for(int k=0;k<8;k++) SDL_RenderDrawLine(ren,px[k],py[k],px[(k+1)%8],py[(k+1)%8]);
        SDL_RenderDrawLine(ren,cx,cy,cx,cy-r+2); SDL_RenderDrawLine(ren,cx,cy,cx+r-2,cy); break;}
    case S_PLAYING:{ for(int dx=0;dx<isz;dx++){int hh=r-dx*r/isz;if(hh<0)hh=0;
        SDL_RenderDrawLine(ren,cx-r+1+dx,cy-hh,cx-r+1+dx,cy+hh);} break;}
    case S_FINISHED:{ int x0=cx-r,xm=cx-r/4,x1=cx+r,ym=cy+r/4,yb=cy+r;
        SDL_RenderDrawLine(ren,x0,ym,xm,yb); SDL_RenderDrawLine(ren,x0+1,ym,xm+1,yb);
        SDL_RenderDrawLine(ren,xm,yb,x1,cy-r+1); SDL_RenderDrawLine(ren,xm+1,yb,x1+1,cy-r+1); break;}
    case S_DROPPED:{
        SDL_RenderDrawLine(ren,cx-r,cy-r,cx+r,cy+r); SDL_RenderDrawLine(ren,cx-r+1,cy-r,cx+r+1,cy+r);
        SDL_RenderDrawLine(ren,cx+r,cy-r,cx-r,cy+r); SDL_RenderDrawLine(ren,cx+r-1,cy-r,cx-r-1,cy+r); break;}
    case S_FAV:{ for(int dy=-r;dy<=r;dy++){int ww=r-abs(dy);
        SDL_RenderDrawLine(ren,cx-ww,cy+dy,cx+ww,cy+dy);} break;}
    case S_ROTATION:{ int px2[7]={cx+r,cx+d,cx,cx-d,cx-r,cx-d,cx};
        int py2[7]={cy,cy-d,cy-r,cy-d,cy,cy+d,cy+r};
        for(int k=0;k<6;k++) SDL_RenderDrawLine(ren,px2[k],py2[k],px2[k+1],py2[k+1]);
        SDL_RenderDrawLine(ren,cx-3,cy+r-1,cx,cy+r+2);
        SDL_RenderDrawLine(ren,cx+3,cy+r+1,cx,cy+r+2); break;}
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
}

/* Material Symbols codepoints (UTF-8 encoded):
   bookmark=U+E866, history=U+E889, play_arrow=U+E037,
   check_circle=U+E86C, cancel=U+E5CD, favorite=U+E87D, autorenew=U+E863 */
static const char *SICON[N_STATUS]={
    "\xEE\xA1\xA6",  /* S_WISH     — bookmark      */
    "\xEE\xA2\x89",  /* S_PLAYED   — history       */
    "\xEE\x80\xB7",  /* S_PLAYING  — play_arrow    */
    "\xEE\xA1\xAC",  /* S_FINISHED — check_circle  */
    "\xEE\x97\x8D",  /* S_DROPPED  — cancel        */
    "\xEE\xA1\xBD",  /* S_FAV      — favorite      */
    "\xEE\xA1\xA3",  /* S_ROTATION — autorenew     */
};

static void draw_status_icon(int j, int cx, int cy, int isz, C4 col){
    if(f_icon){
        /* render via Material Symbols font, centred on cx,cy */
        int fw=txw_(f_icon,SICON[j]);
        int fh=TTF_FontHeight(f_icon);
        rtx(f_icon, SICON[j], cx-fw/2, cy-fh/2, col);
    } else {
        draw_status_icon_prim(j,cx,cy,isz,col);
    }
}

/* render any icon string centred on (cx,cy) using the icon font */
static void ric(const char *cp, int cx, int cy, C4 col){
    if(!f_icon) return;
    int fw=txw_(f_icon,cp), fh=TTF_FontHeight(f_icon);
    rtx(f_icon,cp,cx-fw/2,cy-fh/2,col);
}


/* Returns the highest-priority active status index, or -1 if none. */
static int primary_status(const Game *g){
    for(int k=0;k<N_STATUS;k++){
        int j=SPRIO[k];
        if(g->st[j]) return j;
    }
    return -1;
}

/* ── Status badge popup ─────────────────────────────────────────────── */
/* Compute popup anchor rect from the live row index each frame.
   Returns 0 if the row is not currently visible (caller should close popup). */
static int badge_popup_pos(int *px_out, int *py_out){
    /* prevent close animation from drawing in the wrong view mode */
    if(view_mode != badge_open_vm) return 0;
    int badge_by2, px;
    if(view_mode==VIEW_GRID){
        /* Recompute card position dynamically so popup follows on window resize */
        if(badge_open_ri < page_first() || badge_open_ri > page_last()) return 0;
        int cols2   = grid_cols();
        int block_w2= cols2*(GRID_W+GRID_GAP)-GRID_GAP;
        int ox2     = (LIST_W_-block_w2)/2;
        int pg_cnt2 = page_last()-page_first()+1;
        int rows_pg2= (pg_cnt2+cols2-1)/cols2; if(rows_pg2<1) rows_pg2=1;
        int used_h2 = rows_pg2*(GRID_H+GRID_GAP)-GRID_GAP;
        int oy2     = LST_Y+(LST_H_-used_h2)/2; if(oy2<LST_Y) oy2=LST_Y;
        int local2  = badge_open_ri - page_first();
        int col2    = local2 % cols2, row2 = local2 / cols2;
        int gcx2    = ox2 + col2*(GRID_W+GRID_GAP);
        int gcy2    = oy2 + row2*(GRID_H+GRID_GAP);
        /* chip_h=24, chip_y = gcy2 + GRID_H - 24 - 6 */
        int badge_y2  = gcy2 + GRID_H - 24 - 6;
        badge_by2     = badge_y2;
        px = gcx2 + GRID_W/2 - BPOP_W/2;
    } else {
        if(badge_open_ri < page_first() || badge_open_ri > page_last()) return 0;
        int ri_local = badge_open_ri - page_first();
        int ay = LST_DATA_Y + LIST_TOP_PAD + ri_local * ROW_H;
        badge_by2 = ay + BADGE_YO;
        px = BADGE_X_ + BADGE_W/2 - BPOP_W/2;
    }
    if(px < 4) px = 4;
    if(px + BPOP_W > win_w - 4) px = win_w - 4 - BPOP_W;
    int badge_bh = (view_mode==VIEW_GRID) ? 24 : BADGE_H;
    int py = badge_by2 + badge_bh + 2;
    if(py + BPOP_H > win_h - SB_H - PG_H)
        py = badge_by2 - BPOP_H - 2;
    *px_out = px; *py_out = py;
    return 1;
}

static void draw_badge_popup(void){
    if(badge_anim < 0.01f) return;
    float a = badge_anim;

    int gi_draw = (badge_open_gi>=0) ? badge_open_gi : badge_last_gi;
    Game *g = (gi_draw>=0 && gi_draw<ndb) ? &db[gi_draw] : NULL;
    if(!g) return;

    int px, py;
    if(!badge_popup_pos(&px, &py)) return;

    int total_h  = BPOP_H;
    int visible_h = (int)(total_h * a);
    if(visible_h < 2) return;

    /* Detect above/below: if popup top is above the badge, animation opens upward */
    int anchor_y = (view_mode==VIEW_GRID) ? badge_grid_by
                 : (badge_open_ri>=0 ? LST_DATA_Y+LIST_TOP_PAD+(badge_open_ri-page_first())*ROW_H+BADGE_YO : py+total_h);
    int opens_above = (py < anchor_y);
    /* Clip from anchor end: below=top-pinned, above=bottom-pinned */
    int clip_y = opens_above ? (py + total_h - visible_h) : py;
    SDL_Rect clip = {px-2, clip_y, BPOP_W+4, visible_h+2};
    SDL_RenderSetClipRect(ren, &clip);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    C4 pbg = C_TBAR; pbg.a = 245;
    C4 pbrd = C_ACC;  pbrd.a = (Uint8)(180*a);
    bfrr_aa(px, py, BPOP_W, total_h, R_MD, BRD_T, pbrd, pbg);

    int fh12 = TTF_FontHeight(f12);
    int fhi  = f_icon ? TTF_FontHeight(f_icon) : fh12;

    /* Draw items in priority order: Favourites, Playing, Finished, Played, Rotation, Wishlist, Dropped */
    for(int pi=0;pi<N_STATUS;pi++){
        int j = SPRIO[pi]; /* actual status index */
        int iy  = py + 4 + pi*BPOP_ITEM_H;
        float hv = badge_item_hov[pi];
        int active = g->st[j];

        /* row highlight */
        if(active || hv > 0.01f){
            C4 hi;
            if(active){ hi=SCOL[j]; hi.a=(Uint8)(55*a); }
            else       { hi=MK4(255,255,255,(Uint8)(hv*25*a)); }
            frr_aa(px+3, iy+2, BPOP_W-6, BPOP_ITEM_H-4, R_SM, hi);
        }

        C4 ic = active ? tintc(SCOL[j],1.4f)
                       : lerpc(C_DIM, C_TXT, hv*0.7f);
        ic.a = (Uint8)(ic.a * a);

        int cy = iy + BPOP_ITEM_H/2;
        int tx = px + 9;

        /* status icon */
        if(f_icon){
            int iw=txw_(f_icon,SICON[j]), ih=TTF_FontHeight(f_icon);
            rtx(f_icon, SICON[j], tx, cy-ih/2, ic);
            tx += iw + 5;
        }
        /* label */
        int avail = px + BPOP_W - 22 - tx; /* leave room for checkmark */
        if(avail > 0) rtxclip(f12, SNAME[j+1], tx, cy-fh12/2, avail, ic);

        /* active checkmark on right */
        if(active){
            C4 ck = tintc(SCOL[j],1.5f); ck.a=(Uint8)(220*a);
            int ck_x = px + BPOP_W - 16;
            /* simple check: two lines */
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            sc_(ck);
            SDL_RenderDrawLine(ren, ck_x,   cy+1, ck_x+3, cy+4);
            SDL_RenderDrawLine(ren, ck_x+3, cy+4, ck_x+7, cy-2);
            SDL_RenderDrawLine(ren, ck_x,   cy+2, ck_x+3, cy+5);
            SDL_RenderDrawLine(ren, ck_x+3, cy+5, ck_x+7, cy-1);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren, NULL);
}

static void draw_row(int ri, int ay){
    int lw=LIST_W_;
    int gi=flt[ri]; Game *g=&db[gi];
    float ht=row_ht[gi];

    /* row bg — alternating, lerps to hover colour; semi-transparent in Galaxy */
    frr_aa(4,ay+3,lw-8,ROW_H-6,R_MD,gal_bg(lerpc((ri%2==0)?C_ROWA:C_ROWB,C_ROWH,ht)));

    /* 3px status accent left-border — glanceable status at-a-row */
    {
        int prim = primary_status(g);
        if(prim >= 0){
            C4 sc2 = SCOL[prim];
            sc2.a = (Uint8)(160 + ht * 60);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(5, ay+5, 3, ROW_H-10, 1, sc2);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }
    }


    int fh18=TTF_FontHeight(f18), fh12=TTF_FontHeight(f12);

    /* Name + genre subtitle stacked in the name column */
    {
        /* vertical block: name (f18) + gap + genre (f12) */
        int block_h = fh18 + 3 + fh12;
        int name_y  = ay + (ROW_H - block_h) / 2;
        int gnr_y   = name_y + fh18 + 3;

        /* ── Name with search-match highlight ── */
        C4 base_col = lerpc(C_TXT,MK4(255,255,255,255),ht*0.4f);
        int qlen = (int)strlen(srch_lc);
        const char *hit = (qlen>0) ? strstr(g->name_lc, srch_lc) : NULL;
        if(hit){
            int off = (int)(hit - g->name_lc);
            int nx  = NM_X;
            if(off > 0){
                char pre[128]; memcpy(pre,g->name,off); pre[off]=0;
                int pw=txw_(f18,pre);
                if(NM_MW_>0) rtxclip(f18,pre,nx,name_y,NM_MW_,base_col);
                nx += pw;
            }
            if(nx < NM_X+NM_MW_){
                char mat[128]; memcpy(mat,g->name+off,qlen); mat[qlen]=0;
                int mw = txw_(f18,mat);
                int avl= NM_X+NM_MW_-nx;
                int vw = mw<avl?mw:avl;
                if(vw>0){
                    C4 hlbg=C_ACC; hlbg.a=55;
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                    SDL_Rect hr={nx,ay+4,vw+2,ROW_H-8};
                    sc_(hlbg); SDL_RenderFillRect(ren,&hr);
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                    C4 hl=tintc(C_ACC,1.8f); hl.a=255;
                    rtxclip(f18,mat,nx,name_y,avl,hl);
                }
                nx += mw;
            }
            if(nx < NM_X+NM_MW_){
                int avl2=NM_X+NM_MW_-nx;
                if(avl2>0){
                    char suf[128]; strncpy(suf,g->name+off+qlen,sizeof(suf)-1); suf[sizeof(suf)-1]=0;
                    rtxclip(f18,suf,nx,name_y,avl2,base_col);
                }
            }
        } else {
            rtxclip(f18,g->name,NM_X,name_y,NM_MW_,base_col);
        }

        /* ── Genre chips below name — each independently clickable ── */
        {
            int cx2 = NM_X;
            /* genre1 */
            int act1 = genre_is_active(g->genre);
            C4 gc1 = act1 ? tintc(C_ACC,1.3f) : lerpc(C_DIM, C_SUB, ht*0.6f);
            int gw1 = txw_(f12, g->genre);
            rtxclip(f12, g->genre, cx2, gnr_y, gw1+1, gc1);
            cx2 += gw1;
            /* genre2 */
            if(g->genre2[0]){
                /* separator dot */
                C4 dotc = C_DIM; dotc.a = 160;
                rtx(f12, " \xC2\xB7 ", cx2, gnr_y, dotc);
                cx2 += txw_(f12, " \xC2\xB7 ");
                int act2 = genre_is_active(g->genre2);
                C4 gc2 = act2 ? tintc(C_ACC,1.3f) : lerpc(C_DIM, C_SUB, ht*0.6f);
                int gw2 = txw_(f12, g->genre2);
                rtxclip(f12, g->genre2, cx2, gnr_y, gw2+1, gc2);
            }
        }
    }

    /* Year — centred alone in its column */
    char yr[8]; sprintf(yr,"%d",g->year);
    C4 yr_col = (filt_year && g->year==filt_year) ? tintc(C_ACC,1.2f)
              : lerpc(C_SUB, C_TXT, ht*0.3f);
    rtxcen(f12,yr, YR_X_, ay+(ROW_H-fh12)/2, YR_W, fh12, yr_col);

    int bby=ay+BTN_YO;

    /* ── Rating ── */
    {
        int ry=ay+(ROW_H-TTF_FontHeight(f12))/2;
        if(g->rating>0){
            char rbuf[8];
            snprintf(rbuf,sizeof(rbuf),"%d",g->rating);
            float t=((float)g->rating-1.f)/9.f;
            C4 rc=lerpc(MK4(160,130,0,255),SCOL[S_FAV],t);
            if(f_icon){
                int iw=txw_(f_icon,"\xEE\xA0\xB8"), nw=txw_(f12,rbuf);
                int ih=TTF_FontHeight(f_icon), fh12r=TTF_FontHeight(f12);
                int block_w=iw+3+nw;
                int lx2=RAT_X_+(RAT_W-block_w)/2;
                int cy2=ry+fh12r/2;
                rtx(f_icon,"\xEE\xA0\xB8",lx2,cy2-ih/2,rc);
                rtx(f12,rbuf,lx2+iw+3,ry,rc);
            } else {
                char rstar[20]; snprintf(rstar,sizeof(rstar),"\xe2\x98\x85 %s",rbuf);
                rtxcen(f12,rstar,RAT_X_,ry,RAT_W,TTF_FontHeight(f12),rc);
            }
        } else {
            rtxcen(f12,"\xe2\x80\x93",RAT_X_,ry,RAT_W,TTF_FontHeight(f12),C_DIM);
        }
    }

    /* ── Notes button ── */
    { int nbx=NOTE_BTN_X_, nby=bby;
      int has_note=(g->notes[0]!=0);
      C4 nb_bg  = lerpc(C_BTNI, has_note?C_ACC:C_SEP, ht*0.5f);
      C4 nb_brd = lerpc(C_SEP,  has_note?C_ACC:C_SUB, ht*0.7f);
      bfrr_aa(nbx,nby,NOTE_BTN_SZ,NOTE_BTN_SZ,R_SM,BRD_T,nb_brd,nb_bg);
      C4 ic2 = lerpc(C_DIM, has_note?C_ACC:C_TXT, ht*0.5f);
      if(f_icon) ric("\xEE\xA1\xB3", nbx+NOTE_BTN_SZ/2, nby+NOTE_BTN_SZ/2, ic2);
    }

    /* ── Status badge ── */
    { int bx=BADGE_X_, by=ay+BADGE_YO, bw=BADGE_W, bh=BADGE_H;
      int gi2=gi; /* alias for clarity */
      float hv=badge_hov[gi2];
      int prim=primary_status(g);

      /* pill background */
      C4 badge_bg, badge_brd;
      if(prim>=0){
          C4 sc=SCOL[prim];
          badge_bg  = tintc(sc, 0.28f + hv*0.12f); badge_bg.a  = 210;
          badge_brd = tintc(sc, 0.70f + hv*0.20f); badge_brd.a = 220;
      } else {
          badge_bg  = lerpc(C_BTNI, C_SEP,  hv*0.5f); badge_bg.a  = 180;
          badge_brd = lerpc(C_SEP,  C_SUB,  hv*0.6f); badge_brd.a = 160;
      }
      /* highlight ring when this badge's popup is open */
      if(badge_open_gi==gi2){
          badge_brd = prim>=0 ? tintc(SCOL[prim],1.1f) : C_ACC;
          badge_brd.a = 255;
      }
      bfrr_aa(bx,by,bw,bh,R_MD,BRD_T,badge_brd,badge_bg);

      /* icon + label inside pill */
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      int cy=by+bh/2;
      int tx=bx+9;
      C4 lc = prim>=0 ? tintc(SCOL[prim],1.4f+hv*0.2f) : lerpc(C_DIM,C_SUB,hv*0.5f);
      lc.a=230;
      if(f_icon && prim>=0){
          int iw=txw_(f_icon,SICON[prim]), ih=TTF_FontHeight(f_icon);
          rtx(f_icon, SICON[prim], tx, cy-ih/2, lc);
          tx += iw+4;
      }
      int fh12b=TTF_FontHeight(f12);
      const char *lbl = prim>=0 ? SNAME[prim+1] : "\xe2\x80\x94"; /* em-dash for none */
      int avail_b = bx+bw-6-tx;
      if(avail_b>0) rtxclip(f12, lbl, tx, cy-fh12b/2, avail_b, lc);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
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
    int prim   = primary_status(g);
    C4 id_col  = (prim>=0) ? SCOL[prim] : C_ACC;

    int fh12 = TTF_FontHeight(f12);
    int fh14 = TTF_FontHeight(f14);
    int tw2  = GRID_W - 24;

    int div_y  = y + 96;
    int meta_y = div_y + 8;
    int chip_h = 24;
    int chip_y = y + GRID_H - chip_h - 6;

    /* ── 1. CARD SHELL ──────────────────────────────────────────── */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    {
        C4 fill, brd;
        if(cur_theme == 8){
            fill   = lerpc(C_BG, id_col, 0.08f);
            fill.a = (Uint8)(55 + (int)(ht * 55));
            brd    = id_col; brd.a = (Uint8)(110 + (int)(ht * 100));
        } else {
            /* Stay very close to BG — just barely lifted above it */
            fill = lerpc(C_BG, C_TBAR, 0.6f + ht * 0.25f);
            brd  = lerpc(C_SEP, id_col, ht * 0.55f);
            brd.a = (Uint8)(70 + (int)(ht * 140));
        }
        bfrr_aa(x, y, GRID_W, GRID_H, R_LG, BRD_T, brd, fill);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    /* ── 2. GAME NAME — centred in upper zone ───────────────────── */
    {
        C4 tc = lerpc(C_TXT, MK4(255,255,255,255), ht * 0.4f);
        int nzone_top = y + 10;
        int nzone_h   = div_y - nzone_top;
        const char *nm = g->name;
        int fw = txw_(f14, nm);

        if(fw <= tw2){
            int ty = nzone_top + (nzone_h - fh14) / 2;
            rtxcen(f14, nm, x, ty, GRID_W, fh14, tc);
        } else {
            int nlen = (int)strlen(nm), split = -1;
            for(int k = nlen-1; k > 0; k--){
                if(nm[k] == ' '){
                    char tmp[128]; strncpy(tmp, nm, k); tmp[k] = 0;
                    if(txw_(f14, tmp) <= tw2){ split = k; break; }
                }
            }
            if(split < 0){
                char buf[128]; ellipsis(f14, nm, tw2, buf, sizeof(buf));
                int ty = nzone_top + (nzone_h - fh14) / 2;
                rtxcen(f14, buf, x, ty, GRID_W, fh14, tc);
            } else {
                char l1[128], l2[128], l2e[128];
                strncpy(l1, nm, split); l1[split] = 0;
                strncpy(l2, nm+split+1, sizeof(l2)-1); l2[sizeof(l2)-1]=0;
                ellipsis(f14, l2, tw2, l2e, sizeof(l2e));
                int bh2 = fh14*2 + 3;
                int ty  = nzone_top + (nzone_h - bh2) / 2;
                if(ty < nzone_top) ty = nzone_top;
                rtxcen(f14, l1,  x, ty,            GRID_W, fh14, tc);
                rtxcen(f14, l2e, x, ty + fh14 + 3, GRID_W, fh14, tc);
            }
        }
    }

    /* ── 3. DIVIDER ─────────────────────────────────────────────── */
    {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        C4 dl = C_SEP; dl.a = 50;
        sc_(dl); SDL_RenderDrawLine(ren, x+8, div_y, x+GRID_W-8, div_y);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    /* ── 4. META — year · genre, centred ───────────────────────── */
    {
        char yr_s[8]; snprintf(yr_s, sizeof(yr_s), "%d", g->year);
        C4 yr_c = (filt_year && g->year==filt_year) ? tintc(C_ACC,1.2f) : C_SUB;
        int act1g = genre_is_active(g->genre);
        C4 gc1g   = act1g ? tintc(C_ACC,1.3f) : C_DIM;
        int sep_w = txw_(f12, " \xC2\xB7 ");
        int yr_w  = txw_(f12, yr_s);
        int gw1   = txw_(f12, g->genre);
        int tot   = yr_w + sep_w + gw1;
        int gx    = x + (GRID_W - tot) / 2;
        if(gx < x+6) gx = x+6;
        C4 dotc = C_DIM; dotc.a = 90;
        rtxclip(f12, yr_s,          gx,           meta_y, yr_w+1, yr_c);
        rtx    (f12, " \xC2\xB7 ", gx+yr_w,      meta_y,         dotc);
        rtxclip(f12, g->genre,      gx+yr_w+sep_w, meta_y, gw1+1, gc1g);
    }

    /* ── 5. RATING — if set, below meta ────────────────────────── */
    if(g->rating > 0){
        int ry = meta_y + fh12 + 5;
        char rbuf[8]; snprintf(rbuf, sizeof(rbuf), "%d", g->rating);
        float rt = ((float)g->rating-1.f)/9.f;
        C4 rc = lerpc(MK4(160,130,0,200), SCOL[S_FAV], rt);
        if(f_icon){
            int iw=txw_(f_icon,"\xEE\xA0\xB8"), ih=TTF_FontHeight(f_icon);
            int bw3=iw+3+txw_(f12,rbuf);
            int lx3=x+(GRID_W-bw3)/2;
            rtx(f_icon,"\xEE\xA0\xB8",lx3,ry+(fh12-ih)/2,rc);
            rtx(f12,rbuf,lx3+iw+3,ry,rc);
        } else {
            char rs[20]; snprintf(rs,sizeof(rs),"\xe2\x98\x85 %s",rbuf);
            rtxcen(f12,rs,x,ry,GRID_W,fh12,rc);
        }
    }

    /* ── 6. NOTES BUTTON — top-right ───────────────────────────── */
    {
        int nb=16, nbx=x+GRID_W-nb-5, nby=y+5;
        int has_note=(g->notes[0]!=0);
        C4 nb_bg  = lerpc(C_BTNI, has_note?C_ACC:C_SEP, ht*0.5f);
        C4 nb_brd = lerpc(C_SEP,  has_note?C_ACC:C_SUB, ht*0.6f);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        bfrr_aa(nbx,nby,nb,nb,R_SM,BRD_T,nb_brd,nb_bg);
        C4 ic2=lerpc(C_DIM,has_note?C_ACC:C_TXT,ht*0.5f);
        if(f_icon) ric("\xEE\xA1\xB3",nbx+nb/2,nby+nb/2,ic2);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    /* ── 7. STATUS BUTTON — centred, proper pill button ─────────── */
    {
        float hv = badge_hov[gi];
        const char *lbl = (prim>=0) ? SNAME[prim+1] : NULL;
        int iw = (f_icon && prim>=0) ? txw_(f_icon, SICON[prim]) : 0;
        int lw = lbl ? txw_(f12, lbl) : 0;

        int chip_w;
        if(iw>0 && lw>0) chip_w = iw + lw + 18;
        else if(iw>0)     chip_w = iw + 14;
        else if(lw>0)     chip_w = lw + 18;
        else              chip_w = 44;
        if(chip_w > GRID_W - 16) chip_w = GRID_W - 16;

        int chip_x = x + (GRID_W - chip_w) / 2;   /* horizontally centred */

        C4 chip_bg, chip_brd;
        if(prim >= 0){
            C4 sc2 = SCOL[prim];
            if(cur_theme == 8){
                /* Galaxy: dark bg lets the border carry the color */
                chip_bg  = lerpc(C_BG, sc2, 0.30f + hv*0.15f); chip_bg.a  = 200;
                chip_brd = sc2; chip_brd.a = (Uint8)(180 + (int)(hv * 60));
            } else {
                chip_bg  = tintc(sc2, 0.25f + hv*0.12f); chip_bg.a  = 210;
                chip_brd = tintc(sc2, 0.65f + hv*0.20f); chip_brd.a = 220;
            }
        } else {
            chip_bg  = lerpc(C_BTNI, C_SEP, hv*0.5f); chip_bg.a  = 180;
            chip_brd = lerpc(C_SEP,  C_SUB, hv*0.6f); chip_brd.a = 160;
        }
        if(badge_open_gi == gi){
            chip_brd = (prim>=0) ? tintc(id_col,1.1f) : C_ACC; chip_brd.a = 255;
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        bfrr_aa(chip_x, chip_y, chip_w, chip_h, chip_h/2, BRD_T, chip_brd, chip_bg);

        int cy2 = chip_y + chip_h/2;
        int tx2 = chip_x + (chip_w - (iw>0&&lw>0 ? iw+lw+4 : iw>0 ? iw : lw)) / 2;
        C4 lc = (prim>=0) ? tintc(id_col, 1.45f + hv*0.2f)
                           : lerpc(C_DIM, C_SUB, hv*0.5f);
        lc.a = 235;
        if(f_icon && prim>=0){
            int ih = TTF_FontHeight(f_icon);
            rtx(f_icon, SICON[prim], tx2, cy2-ih/2, lc);
            tx2 += iw + 4;
        }
        if(lbl){
            int avail = chip_x + chip_w - 4 - tx2;
            if(avail > 0) rtxclip(f12, lbl, tx2, cy2-fh12/2, avail, lc);
        } else if(prim < 0){
            rtxcen(f12, "\xe2\x80\x94", chip_x, chip_y, chip_w, chip_h, lc);
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

        badge_grid_bx = chip_x;
        badge_grid_by = chip_y;
    }
}

/* ── Page bar ────────────────────────────────────────────────────── */
/* Smooth chevron using Wu lines — open V shape, tip centred */
static void draw_arrow(int cx, int cy, int sz, int right, C4 col){
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    /* Slightly open chevron: tip offset 40%, arms extend back */
    int arm = (int)(sz * 1.1f);
    int tip_x = right ? cx + sz*2/3 : cx - sz*2/3;
    int back_x = right ? cx - arm/2  : cx + arm/2;
    rline(back_x, cy - arm, tip_x, cy, col);
    rline(back_x, cy + arm, tip_x, cy, col);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void draw_page_bar(void){
    int tp=total_pages();
    int py=PG_BAR_Y, ph=PG_H;
    if(cur_theme==8){ C4 c=C_SBAR; c.a=120; fblend(0,py,win_w,ph,c); }
    else fr_(0,py,win_w,ph,C_SBAR);

    /* separator line */
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    C4 sep2={C_ACC.r,C_ACC.g,C_ACC.b,40}; sc_(sep2);
    SDL_RenderDrawLine(ren,0,py,win_w,py);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

    if(tp<=1) return;

    int cy=py+ph/2;

    /* ── Prev / Next buttons ── */
    int bw=40, bh=34, br=8;
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

    /* prev / next — icon-only square buttons */
    prev_x=14; next_x=win_w-14-bw;

    #define DRAW_NAV_BTN(bx2,active2,hov2,dir2) do { \
        float _hv=(hov2); \
        C4 _bc=(active2)?lerpc(C_SEP,C_ACC,_hv*0.9f):C_SEP; \
        C4 _ic=(active2)?lerpc(C_SRCH,C_SRCHA,_hv*0.4f):C_BTNI; _ic.a=255; \
        bfrr_aa(bx2,cy-bh/2,bw,bh,R_SM,BRD_T,_bc,_ic); \
        C4 _ac=(active2)?lerpc(C_SUB,C_ACC,_hv):C_DIM; \
        /* chevron_left U+E5CB, chevron_right U+E5CC */ \
        if(f_icon){ ric((dir2)?"\xEE\x97\x8C":"\xEE\x97\x8B",(bx2)+bw/2,cy,_ac); } \
        else { draw_arrow((bx2)+bw/2,cy,4,(dir2),_ac); } \
    } while(0)

    DRAW_NAV_BTN(prev_x, prev_active, pg_prev_hov, 0);
    DRAW_NAV_BTN(next_x, next_active, pg_next_hov, 1);
    #undef DRAW_NAV_BTN
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
    { C4 c=C_BG; c.a=(cur_theme==8)?45:148; fblend(0,LST_Y,LIST_W_,lh,c); }

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

/* ── Stats screen ─────────────────────────────────────────────────── */
static void draw_stats(void){
    /* fully transparent — background texture shows through */

    /* dark card behind all content so text reads against any background */
    { C4 card=C_TBAR; card.a=170;
      fblend(0,LST_Y,LIST_W_,LST_H_+PG_H,card); }

    /* ── Compute stats ── */
    int by_status[N_STATUS]={0};
    int total_tracked=0, untracked=0, n_rated=0;
    long rating_sum=0;
    int rating_hist[11]={0};
    #define MAX_GENRES 64
    char gnames[MAX_GENRES][32]; int gcounts[MAX_GENRES]={0}; int ngnames=0;
    memset(gnames,0,sizeof(gnames));
    static const char *DEC_LBL[]={"<80s","80s","90s","00s","10s","20s+"};
    int decade_counts[6]={0};

    for(int i=0;i<ndb;i++){
        Game *g=&db[i];
        int has=0;
        for(int j=0;j<N_STATUS;j++) if(g->st[j]){ by_status[j]++; has=1; }
        if(has) total_tracked++; else untracked++;
        if(g->rating>0){ n_rated++; rating_sum+=g->rating; rating_hist[g->rating]++; }
        /* genres: only games with at least one status set */
        if(has){
            /* count both primary and secondary genre */
            const char *genres2[2] = {g->genre, g->genre2[0] ? g->genre2 : NULL};
            for(int gi2=0;gi2<2;gi2++){
                if(!genres2[gi2]) continue;
                int gf=-1;
                for(int k=0;k<ngnames;k++) if(strcmp(gnames[k],genres2[gi2])==0){ gf=k; break; }
                if(gf<0&&ngnames<MAX_GENRES){ strncpy(gnames[ngnames],genres2[gi2],31); gf=ngnames++; }
                if(gf>=0) gcounts[gf]++;
            }
        }
        int dec=(g->year<1980)?0:(g->year-1980)/10+1; if(dec>5)dec=5;
        decade_counts[dec]++;
    }
    for(int a=0;a<ngnames-1;a++)
        for(int b=a+1;b<ngnames;b++)
            if(gcounts[b]>gcounts[a]){
                int tc2=gcounts[a]; gcounts[a]=gcounts[b]; gcounts[b]=tc2;
                char tmp[32]; memcpy(tmp,gnames[a],32); memcpy(gnames[a],gnames[b],32); memcpy(gnames[b],tmp,32);
            }
    float avg_rating=(n_rated>0)?(float)rating_sum/(float)n_rated:0.f;
    int hist_max=1;
    for(int k=1;k<=10;k++) if(rating_hist[k]>hist_max) hist_max=rating_hist[k];
    #undef MAX_GENRES

    int fh12=TTF_FontHeight(f12), fh18=TTF_FontHeight(f18);
    /* Layout constants */
    int PAD  = 32;   /* outer left margin */
    int RPAD = 32;   /* outer right margin */
    int GAP  = 12;   /* gap between sections */
    int CGAP = 36;   /* gap between two-column halves */
    int y    = LST_Y + 18;
    int avail_w = LIST_W_ - PAD - RPAD;

    /* subtle card behind all content so text reads against the transparent bg */
    /* no card overlay needed — solid background already set */

    /* ════════════════════════════════════════════
       ROW 1 — three big stat numbers, full width
       ════════════════════════════════════════════ */
    {
        struct { const char *lbl; int val; C4 col; const char *icon; } cs[3]={
            {"In Library", ndb,           C_TXT, "\xEE\xA1\xAC"}, /* check_circle */
            {"Tracked",    total_tracked,  C_ACC, "\xEE\x80\xB7"}, /* play_arrow   */
            {"Untracked",  untracked,      C_DIM, "\xEE\xA1\xA6"}, /* bookmark     */
        };
        int cw = avail_w / 3;
        for(int i=0;i<3;i++){
            int cx = PAD + i*cw;
            char num[16]; snprintf(num,sizeof(num),"%d",cs[i].val);
            int nw=txw_(f22,num);
            rtx(f22,num, cx+(cw-nw)/2, y, cs[i].col);
            /* label + icon underneath */
            int lw2=txw_(f12,cs[i].lbl);
            int iw2=f_icon?txw_(f_icon,cs[i].icon):0;
            int total_lbl=lw2+(iw2>0?iw2+4:0);
            int lx2=cx+(cw-total_lbl)/2;
            C4 lc=C_DIM; if(i==1)lc=C_SUB;
            if(f_icon && iw2>0){
                int ih=TTF_FontHeight(f_icon);
                rtx(f_icon,cs[i].icon,lx2,y+fh18+3+(fh12-ih)/2,lc);
                lx2+=iw2+4;
            }
            rtx(f12,cs[i].lbl, lx2, y+fh18+3, lc);
            /* subtle vertical divider */
            if(i>0){
                C4 dv={C_SEP.r,C_SEP.g,C_SEP.b,55};
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                sc_(dv); SDL_RenderDrawLine(ren,cx,y+2,cx,y+fh18+fh12+4);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            }
        }
        y += fh18 + fh12 + GAP + 10;
    }

    /* ── separator ── */
    { C4 s2={C_SEP.r,C_SEP.g,C_SEP.b,100};
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(s2); SDL_RenderDrawLine(ren,PAD,y,LIST_W_-RPAD,y);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); y+=GAP+2; }

    /* ════════════════════════════════════════════
       ROW 2 — status pills, evenly distributed
       ════════════════════════════════════════════ */
    {
        int pill_h=28;
        /* measure pill widths: icon + space + count */
        int pw[N_STATUS], total_pw=0;
        for(int j=0;j<N_STATUS;j++){
            char num[8]; snprintf(num,sizeof(num),"%d",by_status[j]);
            int iw=f_icon?txw_(f_icon,SICON[j]):txw_(f12,SLBL[j]);
            pw[j]=iw+txw_(f12,num)+22; total_pw+=pw[j];
        }
        int gap2=(avail_w-total_pw)/(N_STATUS-1); if(gap2<4)gap2=4;
        int px=PAD;
        for(int j=0;j<N_STATUS;j++){
            char num[8]; snprintf(num,sizeof(num),"%d",by_status[j]);
            C4 bg2=SCOL[j]; bg2.r/=4; bg2.g/=4; bg2.b/=4; bg2.a=220;
            C4 bc2=SCOL[j]; bc2.a=180;
            C4 pc=SCOL[j]; pc.r=SDL_min(255,(int)pc.r*2); pc.g=SDL_min(255,(int)pc.g*2); pc.b=SDL_min(255,(int)pc.b*2);
            bfrr_aa(px,y,pw[j],pill_h,R_MD,BRD_T,bc2,bg2);
            /* draw icon then count, horizontally centred together */
            int iw=f_icon?txw_(f_icon,SICON[j]):txw_(f12,SLBL[j]);
            int nw=txw_(f12,num);
            int total_inner=iw+6+nw;
            int ix=px+(pw[j]-total_inner)/2;
            int cy2=y+pill_h/2;
            if(f_icon){
                int ih=TTF_FontHeight(f_icon);
                rtx(f_icon,SICON[j],ix,cy2-ih/2,pc);
            } else {
                rtxcen(f12,SLBL[j],px,y,iw+4,pill_h,pc);
            }
            rtx(f12,num,ix+iw+6,cy2-fh12/2,pc);
            px+=pw[j]+gap2;
        }
        y+=pill_h+GAP+6;
    }

    /* ── separator ── */
    { C4 s2={C_SEP.r,C_SEP.g,C_SEP.b,100};
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(s2); SDL_RenderDrawLine(ren,PAD,y,LIST_W_-RPAD,y);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); y+=GAP+2; }

    /* ════════════════════════════════════════════════════════
       ROW 3 — two columns
         LEFT  (55%): Top Genres horizontal bars
         RIGHT (45%): Rating histogram OR "no ratings yet"
       ════════════════════════════════════════════════════════ */
    {
        int lw3 = (int)(avail_w * 0.55f) - CGAP/2;
        int rw3 = avail_w - lw3 - CGAP;
        int lx3 = PAD, rx3 = PAD + lw3 + CGAP;
        int row_top = y;

        /* ── LEFT: Top Genres ── */
        {
            /* category U+E574 */
            { int hx=lx3, hcy=y+fh12/2;
              if(f_icon){ ric("\xEE\x95\xB4",hx+TTF_FontHeight(f_icon)/2,hcy,C_TXT); hx+=TTF_FontHeight(f_icon)+5; }
              rtx(f12,"TOP GENRES", hx, y, C_TXT); } y+=fh12+6;
            int show_g = ngnames<9?ngnames:9;
            if(show_g==0){
                rtx(f12,"No tracked games yet.", lx3, y, C_DIM);
            } else {
            int lbl_w  = 90, cnt_w=28;
            int bar_max_w = lw3 - lbl_w - cnt_w - 6;
            int top_cnt = (gcounts[0]>0)?gcounts[0]:1;
            int bar_h=14, bar_row=bar_h+5;
            for(int k=0;k<show_g;k++){
                int bw3=(int)((float)gcounts[k]/(float)top_cnt*(float)bar_max_w);
                if(bw3<2) bw3=2;
                float fade=1.f-(float)k*0.075f; if(fade<0.3f)fade=0.3f;
                C4 bc3={C_ACC.r,C_ACC.g,C_ACC.b,(Uint8)(210*fade)};
                C4 trk3={C_SEP.r,C_SEP.g,C_SEP.b,35};
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                frr_aa(lx3+lbl_w, y, bar_max_w, bar_h, 3, trk3);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                frr_aa(lx3+lbl_w, y, bw3, bar_h, 3, bc3);
                char ge[32]; strncpy(ge,gnames[k],31); ge[31]=0;
                int ty2=y+(bar_h-fh12)/2;
                rtxclip(f12,ge, lx3, ty2, lbl_w-6, C_TXT);
                char cb3[16]; snprintf(cb3,sizeof(cb3),"%d",gcounts[k]);
                rtx(f12,cb3, lx3+lbl_w+bw3+5, ty2, C_SUB);
                y+=bar_row;
            }
            } /* end show_g>0 */
        }

        /* ── RIGHT: Rating histogram ── */
        {
            int ry=row_top;
            if(n_rated>0){
                /* star U+E838 */
                { int hx=rx3, hcy=ry+fh12/2;
                  if(f_icon){ ric("\xEE\xA0\xB8",hx+TTF_FontHeight(f_icon)/2,hcy,C_TXT); hx+=TTF_FontHeight(f_icon)+5; }
                  char hdr2[72];
                  snprintf(hdr2,sizeof(hdr2),"RATINGS   avg %.1f / 10   (%d rated)",avg_rating,n_rated);
                  rtx(f12,hdr2, hx, ry, C_TXT); } ry+=fh12+6;

                int bar_w2=(rw3 - 9*3)/10; if(bar_w2<12)bar_w2=12;
                int bh_max=LST_H_ - (ry-LST_Y) - fh12 - 20;
                if(bh_max<50) bh_max=50;
                int hx2=rx3;
                for(int k=1;k<=10;k++){
                    int bh2=(int)((float)rating_hist[k]/(float)hist_max*(float)bh_max);
                    if(bh2<2&&rating_hist[k]>0) bh2=2;
                    float t2=((float)k-1.f)/9.f;
                    C4 bc2=lerpc(MK4(150,120,0,200),SCOL[S_FAV],t2);
                    C4 trk2={C_SEP.r,C_SEP.g,C_SEP.b,35};
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                    frr_aa(hx2,ry,bar_w2,bh_max,3,trk2);
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                    if(bh2>0) frr_aa(hx2,ry+bh_max-bh2,bar_w2,bh2,3,bc2);
                    /* count label — only draw if it fits above the bar without overlapping the header */
                    if(rating_hist[k]>0 && bh2 < bh_max - fh12 - 2){
                        char lb2[8]; snprintf(lb2,sizeof(lb2),"%d",rating_hist[k]);
                        rtxcen(f12,lb2, hx2, ry+bh_max-bh2-fh12-1, bar_w2, fh12, bc2);
                    }
                    char lb[4]; snprintf(lb,sizeof(lb),"%d",k);
                    rtxcen(f12,lb, hx2, ry+bh_max+3, bar_w2, fh12, C_DIM);
                    hx2+=bar_w2+3;
                }
            } else {
                { int hx=rx3, hcy=ry+fh12/2;
                  if(f_icon){ ric("\xEE\xA0\xB8",hx+TTF_FontHeight(f_icon)/2,hcy,C_TXT); hx+=TTF_FontHeight(f_icon)+5; }
                  rtx(f12,"RATINGS", hx, ry, C_TXT); } ry+=fh12+10;
                rtx(f14,"No games rated yet.", rx3, ry, C_DIM); ry+=TTF_FontHeight(f14)+4;
                rtx(f12,"Click \xe2\x98\x85 in list view to rate.", rx3, ry, C_DIM);
            }
        }

        /* advance y past both columns */
        int right_bottom = row_top + fh12+6 + LST_H_ - (row_top - LST_Y) - fh12 - 20;
        if(y < right_bottom) y = right_bottom;
        y += GAP + 4;
    }

    /* ── separator ── */
    { C4 s2={C_SEP.r,C_SEP.g,C_SEP.b,45};
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(s2); SDL_RenderDrawLine(ren,PAD,y,LIST_W_-RPAD,y);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); y+=GAP+2; }

    /* ════════════════════════════════════════════
       ROW 4 — Decade distribution, full width
       ════════════════════════════════════════════ */
    if(y + fh12 + 8 + 52 + fh12 + 4 < LST_Y + LST_H_){
        /* calendar_today U+E935 */
        { int hx=PAD, hcy=y+fh12/2;
          if(f_icon){ ric("\xEE\xA4\xB5",hx+TTF_FontHeight(f_icon)/2,hcy,C_TXT); hx+=TTF_FontHeight(f_icon)+5; }
          rtx(f12,"BY DECADE", hx, y, C_TXT); } y+=fh12+6;
        int dec_max=1;
        for(int k=0;k<6;k++) if(decade_counts[k]>dec_max) dec_max=decade_counts[k];
        int bw5=(avail_w - 5*12)/6; if(bw5<40)bw5=40;
        int bh5_max=52;
        int dx2=PAD;
        for(int k=0;k<6;k++){
            int bh5=(int)((float)decade_counts[k]/(float)dec_max*(float)bh5_max);
            if(bh5<2&&decade_counts[k]>0) bh5=2;
            float tf=(float)k/5.f;
            C4 bc5=lerpc(C_SUB,C_ACC,tf); bc5.a=200;
            C4 trk5={C_SEP.r,C_SEP.g,C_SEP.b,35};
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            frr_aa(dx2,y,bw5,bh5_max,4,trk5);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            if(bh5>0) frr_aa(dx2,y+bh5_max-bh5,bw5,bh5,4,bc5);
            if(decade_counts[k]>0){
                char cb5[16]; snprintf(cb5,sizeof(cb5),"%d",decade_counts[k]);
                rtxcen(f12,cb5, dx2, y+bh5_max-bh5-fh12-2, bw5, fh12, C_TXT);
            }
            rtxcen(f12,DEC_LBL[k], dx2, y+bh5_max+4, bw5, fh12, C_SUB);
            dx2+=bw5+12;
        }
    }
}

/* ── Notes overlay ───────────────────────────────────────────────── */
#define NOTE_OW  660
#define NOTE_OH  400
#define NOTE_PAD  20
#define NOTE_LH   22

static void note_area(int *ax,int *ay,int *aw,int *ah){
    int ow=NOTE_OW, oh=NOTE_OH;
    int ox=(win_w-ow)/2, oy=(win_h-oh)/2;
    int fh12=TTF_FontHeight(f12), fh18=TTF_FontHeight(f18);
    int ty=oy+NOTE_PAD+fh18+4+fh12+10+1+8; /* after title+meta+sep */
    *ax=ox+NOTE_PAD; *ay=ty;
    *aw=ow-NOTE_PAD*2; *ah=oh-(ty-oy)-NOTE_PAD;
}
static void note_ensure_visible(void){
    if(note_open<0||note_open>=ndb) return;
    char *ns=db[note_open].notes;
    int len=(int)strlen(ns);
    int ax,ay,aw,ah; note_area(&ax,&ay,&aw,&ah);
    int tp=12, tw=aw-tp*2;
    int cy=0; /* relative to text content top */
    int i=0;
    while(i<len){
        int j=i;
        while(j<len&&ns[j]!='\n'){
            char tmp[512]={0}; int tc=j-i; if(tc>511)tc=511;
            memcpy(tmp,ns+i,tc); tmp[tc]=0;
            if(txw_(f14,tmp)>tw&&j>i) break;
            j++;
        }
        if(j<len&&ns[j]!='\n'){
            int jj=j; while(jj>i&&ns[jj]!=' ') jj--;
            if(jj>i) j=jj;
        }
        int line_end=j;
        int is_nl=(j<len&&ns[j]=='\n');
        int owns=(note_cur>=i&&(is_nl?note_cur<line_end:note_cur<=line_end));
        if(owns){
            if(cy < note_scroll) note_scroll=cy;
            if(cy+NOTE_LH > note_scroll+ah-tp*2) note_scroll=cy+NOTE_LH-(ah-tp*2);
            if(note_scroll<0) note_scroll=0;
            return;
        }
        cy+=NOTE_LH;
        if(j<len&&(ns[j]=='\n'||ns[j]==' ')) j++;
        i=j;
    }
    /* cursor after final '\n' */
    if(cy < note_scroll) note_scroll=cy;
    if(cy+NOTE_LH > note_scroll+ah-tp*2) note_scroll=cy+NOTE_LH-(ah-tp*2);
    if(note_scroll<0) note_scroll=0;
}

/* map a pixel x,y inside the note area to a char offset in ns */
static int note_px_to_pos(const char *ns, int len, int ax, int tw, int ay, int mx2, int my2){
    int tp=12;
    int tx2=ax+tp;
    int cy=ay+tp-note_scroll;
    int fh14=TTF_FontHeight(f14);
    int best_pos=0;
    int i=0;
    while(i<len){
        int j=i;
        while(j<len&&ns[j]!='\n'){
            char tmp[512]={0}; int tc2=j-i; if(tc2>511)tc2=511;
            memcpy(tmp,ns+i,tc2); tmp[tc2]=0;
            if(txw_(f14,tmp)>tw&&j>i) break;
            j++;
        }
        int is_hard_wrap=0;
        if(j<len&&ns[j]!='\n'){
            int jj=j; while(jj>i&&ns[jj]!=' ') jj--;
            if(jj>i) j=jj; else is_hard_wrap=1;
        }
        int is_last=(j>=len||(j<len&&ns[j]=='\n')||is_hard_wrap);
        char linebuf[512]={0};
        int ll=j-i; if(ll>511)ll=511;
        memcpy(linebuf,ns+i,ll);
        if(my2>=cy&&my2<cy+fh14){
            /* for justified lines, compute per-char x positions */
            int nw=0; /* number of inter-word gaps */
            int word_w[64]={0}; int word_pos[64]={0}; int nwords=0;
            int ws=0;
            for(int k=0;k<=(int)strlen(linebuf);k++){
                if(linebuf[k]==' '||linebuf[k]==0){
                    if(k>ws&&nwords<64){
                        char w[512]={0}; memcpy(w,linebuf+ws,k-ws);
                        word_pos[nwords]=ws; word_w[nwords]=txw_(f14,w); nwords++; nw++;
                    }
                    ws=k+1;
                }
            }
            int total_ww=0; for(int k=0;k<nwords;k++) total_ww+=word_w[k];
            float gap_w=(nwords>1&&!is_last)?(float)(tw-total_ww)/(float)(nwords-1):
                        (float)txw_(f14," ");
            int best=i; int best_dist=99999;
            float xf=(float)tx2;
            for(int wi=0;wi<nwords;wi++){
                char w[512]={0}; memcpy(w,linebuf+word_pos[wi],
                    (wi+1<nwords?word_pos[wi+1]-1:strlen(linebuf))-word_pos[wi]);
                for(int k=0;k<=(int)strlen(w);k++){
                    char pre[512]={0}; memcpy(pre,w,k);
                    int px2=(int)(xf+txw_(f14,pre));
                    int dist=abs(mx2-px2);
                    if(dist<best_dist){best_dist=dist;best=i+word_pos[wi]+k;}
                }
                xf+=(float)word_w[wi]+gap_w;
            }
            /* if no words or click past end */
            if(nwords==0){
                int dist=abs(mx2-tx2); if(dist<best_dist){best=i;}
            }
            return best;
        }
        best_pos=(j<len&&ns[j]=='\n')?j+1:j;
        cy+=NOTE_LH;
        /* skip the separator char (newline or wrap-space) */
        if(j<len&&(ns[j]=='\n'||ns[j]==' ')) j++;
        i=j;
    }
    if(my2>=cy) return len;
    return best_pos;
}

/* draw one line of text justified; if is_last, left-align.
   Returns nothing; handles selection highlights and cursor. */
static void note_draw_line(const char *linebuf, int line_offset, int i,
                            int tx2, int cy, int tw,
                            int is_last, int fh14,
                            int has_sel, int sel_a, int sel_b, int line_end,
                            int cur_vis, int owns, int note_cur_arg,
                            int *cursor_drawn_out){
    int lb_len=(int)strlen(linebuf);
    /* split into words */
    int word_pos[64]={0}, word_len_arr[64]={0}; int nwords=0;
    { int ws=0;
      for(int k=0;k<=lb_len;k++){
          if(linebuf[k]==' '||linebuf[k]==0){
              if(k>ws&&nwords<64){
                  word_pos[nwords]=ws; word_len_arr[nwords]=k-ws; nwords++;
              }
              ws=k+1;
          }
      }
    }
    /* compute word pixel widths */
    int word_w[64]={0}; int total_ww=0;
    for(int w=0;w<nwords;w++){
        char wb[512]={0}; memcpy(wb,linebuf+word_pos[w],word_len_arr[w]);
        word_w[w]=txw_(f14,wb); total_ww+=word_w[w];
    }
    /* inter-word gap */
    float gap_w;
    if(nwords>1&&!is_last) gap_w=(float)(tw-total_ww)/(float)(nwords-1);
    else gap_w=(float)txw_(f14," ");
    /* draw each word */
    float xf=(float)tx2;
    for(int w=0;w<nwords;w++){
        char wb[512]={0}; memcpy(wb,linebuf+word_pos[w],word_len_arr[w]);
        int wx=(int)xf;
        /* selection highlight for this word */
        if(has_sel){
            int wa=i+word_pos[w], wb2=wa+word_len_arr[w];
            if(sel_a<wb2 && sel_b>wa){
                int hs2=sel_a>wa?sel_a:wa, he2=sel_b<wb2?sel_b:wb2;
                int ks=hs2-wa, ke=he2-wa;
                if(ks<0)ks=0; if(ke>word_len_arr[w])ke=word_len_arr[w];
                char ps2[512]={0},pe2[512]={0};
                memcpy(ps2,wb,ks); memcpy(pe2,wb,ke);
                int sx2=wx+txw_(f14,ps2), ex2=wx+txw_(f14,pe2);
                if(ex2>sx2){
                    C4 sc3=C_ACC; sc3.a=80;
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                    SDL_Rect sr2={sx2,cy,ex2-sx2,fh14}; sc_(sc3); SDL_RenderFillRect(ren,&sr2);
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                }
            }
        }
        rtx(f14,wb,wx,cy,C_TXT);
        /* cursor in this word */
        if(cur_vis && owns && !(*cursor_drawn_out)){
            int wa=i+word_pos[w], wb2=wa+word_len_arr[w];
            if(note_cur_arg>=wa && note_cur_arg<=wb2){
                int cp=note_cur_arg-wa; if(cp>word_len_arr[w])cp=word_len_arr[w];
                char pre[512]={0}; memcpy(pre,wb,cp);
                int cxp=wx+txw_(f14,pre);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                sc_(C_ACC);
                SDL_RenderDrawLine(ren,cxp,cy,cxp,cy+fh14-1);
                SDL_RenderDrawLine(ren,cxp+1,cy,cxp+1,cy+fh14-1);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                *cursor_drawn_out=1;
            }
        }
        xf+=(float)word_w[w]+gap_w;
    }
    /* cursor at very end of line (after last word) */
    if(cur_vis && owns && !(*cursor_drawn_out) && note_cur_arg==i+lb_len){
        int cxp=(int)xf-(nwords>0?(int)gap_w:0);
        /* for empty line or end, just use tx2 */
        if(nwords==0) cxp=tx2;
        else { /* recompute end of last word */
            float xf2=(float)tx2;
            for(int w2=0;w2<nwords;w2++) xf2+=(float)word_w[w2]+(w2<nwords-1?gap_w:0.f);
            cxp=(int)xf2;
        }
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(C_ACC);
        SDL_RenderDrawLine(ren,cxp,cy,cxp,cy+fh14-1);
        SDL_RenderDrawLine(ren,cxp+1,cy,cxp+1,cy+fh14-1);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        *cursor_drawn_out=1;
    }
    /* selection spanning the inter-word gaps */
    if(has_sel){
        /* fill gaps between selected words */
        float xf2=(float)tx2;
        for(int w=0;w<nwords-1;w++){
            int wa=i+word_pos[w]; int wb2=wa+word_len_arr[w];
            int wa_next=i+word_pos[w+1];
            if(sel_a<=wb2 && sel_b>=(int)(wa_next)){
                int gx=(int)(xf2+(float)word_w[w]);
                int ge=(int)(xf2+(float)word_w[w]+gap_w);
                if(ge>gx){
                    C4 sc3=C_ACC; sc3.a=80;
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                    SDL_Rect sr2={gx,cy,ge-gx,fh14}; sc_(sc3); SDL_RenderFillRect(ren,&sr2);
                    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                }
            }
            xf2+=(float)word_w[w]+gap_w;
        }
    }
    (void)line_offset; (void)line_end;
}

static void draw_note_overlay(void){
    if(note_anim<0.005f) return;
    Game *g=(note_open>=0&&note_open<ndb)?&db[note_open]:NULL;
    if(!g&&note_open<0) return;

    int ow=NOTE_OW, oh=NOTE_OH;
    int ox=(win_w-ow)/2, oy=(win_h-oh)/2;
    float a=note_anim;

    /* dim backdrop */
    { C4 dim={0,0,0,(Uint8)(155*a)};
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(dim); SDL_Rect r={0,0,win_w,win_h}; SDL_RenderFillRect(ren,&r);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    /* panel — rounded fill + rounded border */
    { SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      C4 bc=C_ACC; bc.a=(Uint8)(200*a);
      bfrr_aa(ox,oy,ow,oh,R_LG,BRD_T,bc,C_TBAR);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    if(!g) return;

    int fh12=TTF_FontHeight(f12), fh14=TTF_FontHeight(f14), fh18=TTF_FontHeight(f18);
    int ty=oy+NOTE_PAD;

    /* title */
    rtxclip(f18,g->name, ox+NOTE_PAD, ty, ow-NOTE_PAD*2, C_TXT);
    ty+=fh18+4;

    /* meta */
    { char meta[80];
      if(g->rating>0)
          snprintf(meta,sizeof(meta),"%s  \xc2\xb7  %d  \xc2\xb7  \xe2\x98\x85 %d/10",g->genre,g->year,g->rating);
      else
          snprintf(meta,sizeof(meta),"%s  \xc2\xb7  %d",g->genre,g->year);
      rtx(f12,meta, ox+NOTE_PAD, ty, C_SUB); }
    ty+=fh12+10;

    /* separator */
    { C4 sep=C_SEP; sep.a=80;
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      sc_(sep); SDL_RenderDrawLine(ren,ox+NOTE_PAD,ty,ox+ow-NOTE_PAD,ty);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }
    ty+=8; /* area_y starts here, matches note_area() */

    /* text area — fills all remaining space */
    int area_x=ox+NOTE_PAD, area_y=ty;
    int area_w=ow-NOTE_PAD*2, area_h=oh-(ty-oy)-NOTE_PAD;
    int box_r=7, tp=12;
    int tx2=area_x+tp, tw=area_w-tp*2;

    /* document-style background: slightly lighter than panel, like a page */
    C4 doc_bg={C_ROWB.r+14,C_ROWB.g+14,C_ROWB.b+18,255};
    { SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      C4 brd=C_SEP; brd.a=120;
      bfrr_aa(area_x,area_y,area_w,area_h,box_r,BRD_T,brd,doc_bg);
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    /* clip strictly inside the rounded box */
    SDL_Rect clip2={area_x+2,area_y+2,area_w-4,area_h-4};
    SDL_RenderSetClipRect(ren,&clip2);

    { char *ns=g->notes;
      int len=(int)strlen(ns);
      int cy=area_y+tp-note_scroll;  /* subtract scroll offset */
      Uint32 ticks=SDL_GetTicks();
      int cur_vis=((ticks/530)%2==0);
      int sel_a=note_sel0<note_cur?note_sel0:note_cur;
      int sel_b=note_sel0<note_cur?note_cur:note_sel0;
      int has_sel=(sel_a!=sel_b);

      if(len==0){
          /* empty: placeholder + single cursor at start */
          int py=area_y+tp;
          rtx(f14,"Write your notes here...",tx2,py,C_DIM);
          if(cur_vis){
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
              sc_(C_ACC);
              SDL_RenderDrawLine(ren,tx2,py,tx2,py+fh14-1);
              SDL_RenderDrawLine(ren,tx2+1,py,tx2+1,py+fh14-1);
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
          }
      } else {
          int cursor_drawn=0;
          int i=0;
          while(i<len){
              /* ── find wrap point j ── */
              int j=i;
              while(j<len&&ns[j]!='\n'){
                  char tmp2[512]={0}; int tc2=j-i; if(tc2>511)tc2=511;
                  memcpy(tmp2,ns+i,tc2); tmp2[tc2]=0;
                  if(txw_(f14,tmp2)>tw&&j>i) break;
                  j++;
              }
              int is_hard_wrap=0;
              if(j<len&&ns[j]!='\n'){
                  int jj=j; while(jj>i&&ns[jj]!=' ') jj--;
                  if(jj>i) j=jj; else is_hard_wrap=1;
              }
              /* last line = ends paragraph or hard-wraps (no space found) */
              int is_last=(j>=len||(j<len&&ns[j]=='\n')||is_hard_wrap);

              char linebuf[512]={0};
              int ll=j-i; if(ll>511)ll=511;
              memcpy(linebuf,ns+i,ll);
              int line_end=j;

              if(cy+NOTE_LH>area_y&&cy<area_y+area_h){
                  int is_nl=(j<len&&ns[j]=='\n');
                  int owns=(note_cur>=i&&(is_nl?note_cur<line_end:note_cur<=line_end));
                  note_draw_line(linebuf,i,i,tx2,cy,tw,is_last,fh14,
                                 has_sel,sel_a,sel_b,line_end,
                                 cur_vis,owns,note_cur,&cursor_drawn);
              }
              cy+=NOTE_LH;
              /* skip separator: newline or wrap-space */
              if(j<len&&(ns[j]=='\n'||ns[j]==' ')) j++;
              i=j;
              if(cy>area_y+area_h) break;
          }
          /* cursor on empty trailing line after final '\n' */
          if(cur_vis && !cursor_drawn && note_cur==len && len>0 && ns[len-1]=='\n'){
              if(cy+NOTE_LH>area_y&&cy<area_y+area_h){
                  SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                  sc_(C_ACC);
                  SDL_RenderDrawLine(ren,tx2,cy,tx2,cy+fh14-1);
                  SDL_RenderDrawLine(ren,tx2+1,cy,tx2+1,cy+fh14-1);
                  SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
              }
          }
      }
    }
    SDL_RenderSetClipRect(ren,NULL);
    (void)fh14;
}

/* ── Column header row ─────────────────────────────────────────── */
static void draw_col_header(void){
    int hy = LST_Y, hh = COL_HDR_H;
    { C4 hbg = C_BG; hbg.a = (cur_theme==8)?85:185; fblend(0, hy, LIST_W_, hh, hbg); }
    /* two-pixel separator: bright line + subtle shadow — makes it read as a real table header */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    { C4 sep = C_SEP; sep.a = 40; sc_(sep);
      SDL_RenderDrawLine(ren, 0, hy+hh-2, LIST_W_, hy+hh-2); }
    { C4 sep = C_SEP; sep.a = 120; sc_(sep);
      SDL_RenderDrawLine(ren, 0, hy+hh-1, LIST_W_, hy+hh-1); }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    int fh = TTF_FontHeight(f12), ty = hy + (hh - fh) / 2;
    /* uppercase labels read clearly as column identifiers */
    rtx(f12, "GAME",   NM_X, ty, C_DIM);
    rtxcen(f12, "YEAR",   YR_X_,       ty, YR_W,        fh, C_DIM);
    rtxcen(f12, "RATING", RAT_X_,      ty, RAT_W,       fh, C_DIM);
    rtxcen(f12, "NOTE",   NOTE_BTN_X_, ty, NOTE_BTN_SZ, fh, C_DIM);
    rtxcen(f12, "STATUS", BADGE_X_,    ty, BADGE_W,     fh, C_DIM);
}

/* ── List ─────────────────────────────────────────────────────── */
static void draw_list(void){
    if(cur_tab==T_STATS){ draw_stats(); return; }
    if(view_mode==VIEW_GRID){ draw_grid(); return; }
    int lh = LST_H_;
    /* header sits above the clip region — always visible */
    draw_col_header();
    SDL_Rect clip={0, LST_DATA_Y, win_w, lh - COL_HDR_H};
    SDL_RenderSetClipRect(ren, &clip);
    { C4 c=C_BG; c.a=(cur_theme==8)?45:148; fblend(0, LST_DATA_Y, LIST_W_, lh-COL_HDR_H, c); }
    int first=page_first(), last=page_last();
    for(int r=first;r<=last;r++)
        draw_row(r, LST_DATA_Y+LIST_TOP_PAD+(r-first)*ROW_H);
    if(nflt==0){
        rtxcen(f18,"No games found.",   0, LST_DATA_Y,                   LIST_W_, (lh-COL_HDR_H)*2/3, C_SUB);
        rtxcen(f14,"Try a different search or tab.", 0, LST_DATA_Y+(lh-COL_HDR_H)*2/3, LIST_W_, (lh-COL_HDR_H)/3, C_DIM);
    }
    SDL_RenderSetClipRect(ren, NULL);
    draw_page_bar();
}

/* ── Status bar ───────────────────────────────────────────────── */
static void draw_sbar(void){
    { C4 c=C_SBAR; c.a=170; fblend(0,win_h-SB_H,win_w,SB_H,c); }
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
        if(f_icon){
            int iw_=txw_(f_icon,SICON[i]), ih_=TTF_FontHeight(f_icon);
            C4 sc2=SCOL[i]; sc2.a=200;
            rx-=iw_;
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            rtx(f_icon,SICON[i],rx,y+(fh-ih_)/2,sc2);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            rx-=4;
        } else {
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            frr_aa(rx-7,y+(fh-6)/2,6,6,3,SCOL[i]);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            rx-=12;
        }
    }
}

/* ═══════════════════════ Helpers ═══════════════════════════════ */
static void do_tab(int i){
    cur_tab=(TabId)i; scr_tgt=0; scr_f=0; cur_page=0;
    BADGE_CLOSE_NOW(); /* close popup on tab change */
    if(i<N_TABS) tab_itx=(float)TAB_X_(tab_disp(i)); /* don't move pill for T_STATS */
    rebuild();
}
static int hit_tab(int mx,int my){
    if(my<TAB_Y||my>=TAB_Y+TAB_H) return -1;
    int tw=TAB_W_;
    for(int i=0;i<N_TABS;i++){ int tx=TAB_X_(i); if(mx>=tx&&mx<tx+tw) return TAB_ORDER[i]; }
    return -1;
}
static int row_at(int mx,int my){
    if(mx>=LIST_W_||my<LST_DATA_Y||my>=LST_Y+LST_H_) return -1;
    int r=(my-LST_DATA_Y-LIST_TOP_PAD)/ROW_H+page_first();
    return (r>=page_first()&&r<=page_last())?r:-1;
}
/* ── Titlebar button tooltips ────────────────────────────────── */
static void draw_tb_tooltips(void){
    /* Only show the trigger button tooltip — all other controls are inside the panel */
    if(cmd_open || cmd_anim > 0.05f) return; /* suppress when panel is open */
    if(cmd_btn_hov < 0.02f) return;

    int fh = TTF_FontHeight(f12);
    int ph = fh + 6;
    int ty = TITLE_H + 4;
    float a = cmd_btn_hov;

    const char *lbl = "Settings";
    int tw = txw_(f12, lbl);
    int pw = tw + 12;
    int px = CMD_BTN_X + CMD_BTN_W/2 - pw/2;
    if(px < 2) px = 2;
    if(px+pw > win_w-2) px = win_w-2-pw;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    C4 bg  = {(Uint8)C_TITLE.r,(Uint8)C_TITLE.g,(Uint8)C_TITLE.b,(Uint8)(230*a)};
    C4 brd = {(Uint8)C_ACC.r,  (Uint8)C_ACC.g,  (Uint8)C_ACC.b,  (Uint8)(140*a)};
    C4 tc2 = {(Uint8)C_TXT.r,  (Uint8)C_TXT.g,  (Uint8)C_TXT.b,  (Uint8)(230*a)};
    bfrr_aa(px, ty, pw, ph, R_SM, BRD_T, brd, bg);
    rtxcen(f12, lbl, px, ty, pw, ph, tc2);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
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
        if(GDB[i].g2){ strncpy(db[i].genre2,GDB[i].g2,31); db[i].genre2[31]=0; }
        else db[i].genre2[0]=0;
        db[i].year=GDB[i].y; memset(db[i].st,0,N_STATUS);
        db[i].rating=0; db[i].notes[0]=0;
        strlower(db[i].name,   db[i].name_lc,   128);
        strlower(db[i].genre,  db[i].genre_lc,  32);
        strlower(db[i].genre2, db[i].genre2_lc, 32);
    }
    ndb=lim;
    qsort(db,ndb,sizeof(Game),cmp_game);
    memset(row_ht,0,sizeof(row_ht));
    memset(btn_fl,0,sizeof(btn_fl));
    memset(tab_fl,0,sizeof(tab_fl));
    memset(tc_fl, 0,sizeof(tc_fl));
    build_genre_list();
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

    /* Bake all SFX tones once so clicks never stall the render thread */
    sfx_init();

    f22=load_font(22); f18=load_font(18);
    f14=load_font(14); f12=load_font(12);
    /* Material Symbols icon font — user places as icons.ttf in app folder */
    { const char *ip[]={
        "icons.ttf",
        "MaterialSymbolsRounded-VariableFont_FILL_GRAD_opsz_wght.ttf",
        NULL };
      for(int i=0;ip[i];i++){
          f_icon=TTF_OpenFont(ip[i],14);
          if(f_icon) break;
      }
    }
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
    bg_init();

    /* Snap pill indicators to loaded state */
    sort_ind_f=(float)sort_mode;
    sort_dd_open=0; sort_dd_anim=0.f; sort_btn_hov=0.f;
    memset(sort_item_hov,0,sizeof(sort_item_hov));
    view_ind_f=(float)view_mode;

    memset(tdot_hov,0,sizeof(tdot_hov));
    memset(tdot_bounce,0,sizeof(tdot_bounce));
    memset(btn_hov,0,sizeof(btn_hov));
    memset(badge_hov,0,sizeof(badge_hov));
    memset(badge_item_hov,0,sizeof(badge_item_hov));
    n_filt_genres=0; filt_year=0; chip_band=0;
    genre_dd_open=0; genre_dd_anim=0.f; genre_btn_hov=0.f;
    memset(genre_item_hov,0,sizeof(genre_item_hov));
    
    BADGE_CLOSE();
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
            if(SDL_WaitEventTimeout(&ev, 16)){
                SDL_PushEvent(&ev); /* put it back — PollEvent handles it below */
            }
        }

        needs_redraw = anim_tick();

        /* Flush deferred save once the debounce window has passed */
        if(save_pending_at && SDL_GetTicks()-save_pending_at >= SAVE_DEFER_MS)
            save_flush();

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
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));}
                    apply_rgn(win_w,win_h,!win_maximized);
                    rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_MAXIMIZED){
                    win_maximized=1; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,0);
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));} rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_RESTORED){
                    win_maximized=0; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,1);
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_(tab_disp((int)cur_tab));} rebuild();
                    gal_paused=0;  /* resume galaxy animation on restore */
                }
                /* Pause / resume the galaxy background animation so it doesn't
                   run while the window is hidden or the user has switched away. */
                if(ev.window.event==SDL_WINDOWEVENT_FOCUS_LOST ||
                   ev.window.event==SDL_WINDOWEVENT_MINIMIZED)
                    gal_paused=1;
                if(ev.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)
                    gal_paused=0;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if(ev.button.button==SDL_BUTTON_LEFT){
                    int mx=ev.button.x, my=ev.button.y;

                        /* ── Note overlay: click inside places cursor, outside closes ── */
                        if(note_open>=0){
                            int ow2=NOTE_OW,oh2=NOTE_OH;
                            int ox2=(win_w-ow2)/2,oy2=(win_h-oh2)/2;
                            if(mx>=ox2&&mx<ox2+ow2&&my>=oy2&&my<oy2+oh2){
                                /* click inside text area → place cursor */
                                int ax2,ay2,aw2,ah2; note_area(&ax2,&ay2,&aw2,&ah2);
                                if(mx>=ax2&&mx<ax2+aw2&&my>=ay2&&my<ay2+ah2){
                                    int tp2=8, tw2=aw2-tp2*2;
                                    char *ns2=db[note_open].notes;
                                    int len2=(int)strlen(ns2);
                                    SDL_Keymod km2=SDL_GetModState();
                                    int shift2=(km2&KMOD_SHIFT)!=0;
                                    int pos=note_px_to_pos(ns2,len2,ax2,tw2,ay2,mx,my);
                                    note_cur=pos;
                                    if(!shift2) note_sel0=pos;
                                    note_drag=1;
                                }
                            } else {
                                note_open=-1; note_drag=0; note_scroll=0; save_defer();
                            }
                            goto done_click;
                        }

                        /* ── CMD panel: genre dropdown item click ── */
                        if(genre_dd_open&&genre_dd_anim>0.1f){
                            int ax=genre_dd_ax, ay=genre_dd_ay;
                            if(IN_RECT(mx,my,ax,ay,GENRE_DD_W,800)){
                                int gi=(my-ay-4)/DD_ITEM_H;
                                if(gi>=0&&gi<n_genres){
                                    toggle_genre_filter(genre_list[gi]);
                                    update_chip_band(); sfx_click(); rebuild();
                                    cur_page=0;
                                }
                                goto done_click;
                            }
                            /* dropdown stays open — only genre button or CMD trigger closes it */
                        }

                        /* ── CMD panel: all clicks inside the panel ── */
                        if((cmd_open||cmd_anim>0.1f)&&
                           IN_RECT(mx,my,CMD_BTN_X,CMD_PANEL_OY,CMD_OW,CMD_OH)){
                            handle_cmd_click(mx,my);
                            goto done_click;
                        }

                        if(my<TITLE_H){
                            /* Window controls */
                            if(IN_BTN(mx,TB_CX)){ tb_close_fl=1.f; sfx_click(); run=0; break; }
                            if(IN_BTN(mx,TB_MX)){ tb_max_fl=1.f;   sfx_click(); toggle_maximize(); break; }
                            if(IN_BTN(mx,TB_NX)){ tb_min_fl=1.f;   sfx_click(); SDL_MinimizeWindow(win); break; }

                            /* CMD trigger button */
                            if(IN_RECT(mx,my,CMD_BTN_X,CMD_BTN_Y,CMD_BTN_W,CMD_BTN_H)){
                                cmd_open=!cmd_open;
                                if(!cmd_open){ genre_dd_open=0; genre_dd_anim=0.f; }
                                tc_fl[0]=1.f; sfx_click(); break;
                            }

                            /* Close CMD panel + genre dropdown if clicking anywhere else in title bar */
                            if(cmd_open){ cmd_open=0; genre_dd_open=0; genre_dd_anim=0.f; sfx_click(); break; }

                            RzDir rd=get_rz(mx,my);
                            if(rd!=RZ_NONE){
                                rz_drag=1; rz_active=rd;
                                SDL_GetGlobalMouseState(&rz_gx0,&rz_gy0);
                                SDL_GetWindowPosition(win,&rz_wx0,&rz_wy0);
                                rz_ww0=win_w; rz_wh0=win_h; break;
                            }
                            if(!win_maximized){ win_drag=1; win_drag_ox=mx; win_drag_oy=my; }
                            break;
                        }

                        /* Close CMD panel on click anywhere outside title bar + panel */
                        if(cmd_open){ cmd_open=0; genre_dd_open=0; genre_dd_anim=0.f; }

                    {
                        RzDir rd=get_rz(mx,my);
                        if(rd!=RZ_NONE){
                            rz_drag=1; rz_active=rd;
                            SDL_GetGlobalMouseState(&rz_gx0,&rz_gy0);
                            SDL_GetWindowPosition(win,&rz_wx0,&rz_wy0);
                            rz_ww0=win_w; rz_wh0=win_h; break;
                        }
                    }

                    /* ── Filter chip clicks ── */
                    if(chip_band && my>=TITLE_H+HDR_H && my<TITLE_H+HDR_H+CHIP_H){
                        int ih = CHIP_H - 8;
                        int iy = (TITLE_H+HDR_H) + (CHIP_H-ih)/2;
                        const char *X_GLYPH = "\xc3\x97";
                        int xw = txw_(f12, X_GLYPH);
                        int ccx = 10;
                        if(n_filt_genres>0){
                            char lbl[96]; int pos2=snprintf(lbl,sizeof(lbl),"Genre: ");
                            for(int gi=0;gi<n_filt_genres&&pos2<(int)sizeof(lbl)-2;gi++){
                                if(gi>0) pos2+=snprintf(lbl+pos2,sizeof(lbl)-pos2,", ");
                                pos2+=snprintf(lbl+pos2,sizeof(lbl)-pos2,"%s",filt_genres[gi]);
                            }
                            int cw = txw_(f12,lbl) + xw + 20;
                            if(my>=iy&&my<iy+ih&&mx>=ccx&&mx<ccx+cw){
                                n_filt_genres=0; chip_genre_hov=0.f;
                                update_chip_band(); sfx_click(); rebuild(); goto done_click;
                            }
                            ccx += cw + 6;
                        }
                        if(filt_year){
                            char lbl[32]; snprintf(lbl,sizeof(lbl),"Year: %d",filt_year);
                            int cw = txw_(f12,lbl) + xw + 20;
                            if(my>=iy&&my<iy+ih&&mx>=ccx&&mx<ccx+cw){
                                filt_year=0; chip_year_hov=0.f;
                                update_chip_band(); sfx_click(); rebuild(); goto done_click;
                            }
                            ccx += cw + 6;
                        }
                        /* "Clear all" button — only visible when 2+ filters active */
                        if((n_filt_genres>0)+(filt_year!=0) > 1){
                            /* button is drawn at ccx */
                            const char *clbl = "Clear all";
                            int cw_cl = txw_(f12,clbl) + 16;
                            if(my>=iy&&my<iy+ih&&mx>=ccx&&mx<ccx+cw_cl){
                                n_filt_genres=0; filt_year=0;
                                chip_genre_hov=0.f; chip_year_hov=0.f;
                                update_chip_band(); sfx_click(); rebuild(); goto done_click;
                            }
                        }
                    }

                    { int t=hit_tab(mx,my); if(t>=0){ tab_fl[t]=1.f; if(t!=(int)cur_tab) sfx_tab(); do_tab(t); break; } }

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
                        int py=PG_BAR_Y,ph=PG_H,bw=40,bh=34;
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
                        /* ── Popup item click (always handled first) ── */
                        if(badge_open_gi>=0 && badge_anim>0.1f){
                            int px_p, py_p;
                            if(badge_popup_pos(&px_p,&py_p)){
                                if(mx>=px_p&&mx<px_p+BPOP_W&&my>=py_p&&my<py_p+BPOP_H){
                                    int pi_hit=(my-py_p-4)/BPOP_ITEM_H;
                                    if(pi_hit>=0&&pi_hit<N_STATUS){
                                        int j_hit=SPRIO[pi_hit];
                                        int gi_p=badge_open_gi;
                                        db[gi_p].st[j_hit]^=1;
                                        btn_fl[gi_p][j_hit]=1.f;
                                        sfx_toggle(); rebuild(); save_defer();
                                    }
                                    goto done_click;
                                }
                            }
                        }
                        int r=row_at(mx,my);
                        if(r>=0){
                            int ay_r=LST_DATA_Y+LIST_TOP_PAD+(r-page_first())*ROW_H;
                            /* ── Year column: click to filter by year ── */
                            if(mx>=YR_X_&&mx<YR_X_+YR_W&&my>=ay_r&&my<ay_r+ROW_H){
                                int gi=flt[r];
                                int ny = db[gi].year;
                                filt_year = (filt_year==ny) ? 0 : ny;
                                update_chip_band(); sfx_click(); rebuild(); goto done_click;
                            }
                            /* ── Genre chips in name column: click to filter ── */
                            {
                                int gi=flt[r];
                                int fh18c=TTF_FontHeight(f18), fh12c=TTF_FontHeight(f12);
                                int block_h = fh18c + 3 + fh12c;
                                int name_y2 = ay_r + (ROW_H - block_h) / 2;
                                int gnr_y2  = name_y2 + fh18c + 3;
                                if(my>=gnr_y2 && my<gnr_y2+fh12c &&
                                   mx>=NM_X   && mx<NM_X+NM_MW_){
                                    /* determine which chip was clicked */
                                    int cx2 = NM_X;
                                    int gw1 = txw_(f12, db[gi].genre);
                                    if(mx < cx2 + gw1){
                                        toggle_genre_filter(db[gi].genre);
                                    } else if(db[gi].genre2[0]){
                                        int sep_w = txw_(f12, " \xC2\xB7 ");
                                        cx2 += gw1 + sep_w;
                                        int gw2 = txw_(f12, db[gi].genre2);
                                        if(mx >= cx2 && mx < cx2+gw2)
                                            toggle_genre_filter(db[gi].genre2);
                                        else if(mx < cx2) /* clicked on the dot */
                                            toggle_genre_filter(db[gi].genre);
                                    }
                                    update_chip_band(); sfx_click(); rebuild(); goto done_click;
                                }
                            }
                            /* ── Rating column ── */
                            if(mx>=RAT_X_&&mx<RAT_X_+RAT_W&&my>=ay_r&&my<ay_r+ROW_H){
                                int gi=flt[r];
                                db[gi].rating=(db[gi].rating%10)+1;
                                sfx_toggle(); save_defer(); goto done_click;
                            }
                            /* ── Badge click: toggle popup open/close ── */
                            int bby_c=ay_r+BADGE_YO;
                            int gi=flt[r];
                            if(mx>=BADGE_X_&&mx<BADGE_X_+BADGE_W&&my>=bby_c&&my<bby_c+BADGE_H){
                                if(badge_open_gi==gi){
                                    BADGE_CLOSE();
                                } else {
                                    badge_open_gi=gi; badge_open_ri=r;
                                    badge_last_gi=gi; badge_open_vm=VIEW_LIST;
                                    memset(badge_item_hov,0,sizeof(badge_item_hov));
                                }
                                sfx_click(); goto done_click;
                            }
                            /* ── Note button — closes badge popup ── */
                            int nby2=ay_r+BTN_YO;
                            if(mx>=NOTE_BTN_X_&&mx<NOTE_BTN_X_+NOTE_BTN_SZ
                               &&my>=nby2&&my<nby2+NOTE_BTN_SZ){
                                BADGE_CLOSE();
                                note_open=gi;
                                note_cur=(int)strlen(db[gi].notes);
                                note_sel0=note_cur;
                                sfx_click(); srch_blur();
                            }
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

                            /* Popup item click (grid) — handled before card clicks */
                            if(badge_open_gi>=0 && badge_anim>0.1f){
                                int px_p2, py_p2;
                                if(badge_popup_pos(&px_p2,&py_p2)){
                                    if(mx>=px_p2&&mx<px_p2+BPOP_W&&my>=py_p2&&my<py_p2+BPOP_H){
                                        int pi_hit=(my-py_p2-4)/BPOP_ITEM_H;
                                        if(pi_hit>=0&&pi_hit<N_STATUS){
                                            int j_hit=SPRIO[pi_hit];
                                            int gi_p=badge_open_gi;
                                            db[gi_p].st[j_hit]^=1;
                                            btn_fl[gi_p][j_hit]=1.f;
                                            sfx_toggle(); rebuild(); save_defer();
                                        }
                                        goto done_click;
                                    }
                                }
                            }

                            for(int ri2=page_first();ri2<=page_last();ri2++){
                                if(flt[ri2]!=hov_db) continue;
                                int local2=ri2-page_first();
                                int row2=local2/cols, col2=local2%cols;
                                int cx2=ox+col2*(GRID_W+GRID_GAP);
                                int cy2=oy3+row2*(GRID_H+GRID_GAP);
                                int gbx=cx2+(GRID_W-BADGE_W)/2;
                                int gby=cy2+GRID_H-GBSZ-4;

                                /* ── Badge pill click: open/close popup ── */
                                if(mx>=gbx&&mx<gbx+BADGE_W&&my>=gby&&my<gby+BADGE_H){
                                    if(badge_open_gi==hov_db){
                                        BADGE_CLOSE();
                                    } else {
                                        badge_open_gi=hov_db; badge_open_ri=ri2;
                                        badge_last_gi=hov_db; badge_open_vm=VIEW_GRID;
                                        badge_grid_bx=gbx; badge_grid_by=gby;
                                        memset(badge_item_hov,0,sizeof(badge_item_hov));
                                    }
                                    sfx_click(); goto done_click;
                                }

                                /* ── Note button — top-right corner of card ── */
                                int nb=18, nbx2=cx2+GRID_W-nb-4, nby3=cy2+14;
                                if(mx>=nbx2&&mx<nbx2+nb&&my>=nby3&&my<nby3+nb){
                                    BADGE_CLOSE();
                                    note_open=hov_db;
                                    note_cur=(int)strlen(db[hov_db].notes);
                                    note_sel0=note_cur;
                                    sfx_click(); srch_blur(); break;
                                }

                                /* ── Genre / year click — separate rows ── */
                                {
                                    Game *gcard = &db[hov_db];
                                    int fh12c = TTF_FontHeight(f12);
                                    int gnr_by2 = gby - fh12c - 4;         /* genre row */
                                    int yr_by2  = gnr_by2 - fh12c - 2;     /* year row */
                                    /* year — full-width centered hit area */
                                    if(my>=yr_by2 && my<yr_by2+fh12c){
                                        filt_year = (filt_year==gcard->year) ? 0 : gcard->year;
                                        update_chip_band(); sfx_click(); rebuild(); break;
                                    }
                                    /* genre chips — centered */
                                    if(my>=gnr_by2 && my<gnr_by2+fh12c){
                                        int gw1c = txw_(f12,gcard->genre);
                                        int dot_wc = gcard->genre2[0] ? txw_(f12," \xC2\xB7 ") : 0;
                                        int gw2c  = gcard->genre2[0] ? txw_(f12,gcard->genre2) : 0;
                                        int total_gc = gw1c + dot_wc + gw2c;
                                        int gxc = cx2 + (GRID_W - total_gc)/2;
                                        if(gxc < cx2+4) gxc = cx2+4;
                                        if(mx>=gxc && mx<gxc+gw1c){
                                            toggle_genre_filter(gcard->genre);
                                            update_chip_band(); sfx_click(); rebuild(); break;
                                        }
                                        if(gcard->genre2[0] && mx>=gxc+gw1c+dot_wc && mx<gxc+total_gc){
                                            toggle_genre_filter(gcard->genre2);
                                            update_chip_band(); sfx_click(); rebuild(); break;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                done_click:
                if(ev.button.button==SDL_BUTTON_LEFT&&ev.button.clicks==2
                   &&ev.button.y<TITLE_H&&ev.button.x<win_w-3*TB_BTN_W){
                    toggle_maximize();
                }
                /* ── Right-click: clear rating (both list and grid) ── */
                if(ev.button.button==SDL_BUTTON_RIGHT&&note_open<0){
                    if(view_mode==VIEW_LIST){
                        int mx2r=ev.button.x, my2r=ev.button.y;
                        int r2r=row_at(mx2r,my2r);
                        if(r2r>=0){
                            int ay_r2=LST_DATA_Y+LIST_TOP_PAD+(r2r-page_first())*ROW_H;
                            if(mx2r>=RAT_X_&&mx2r<RAT_X_+RAT_W&&my2r>=ay_r2&&my2r<ay_r2+ROW_H){
                                db[flt[r2r]].rating=0;
                                sfx_toggle(); save_defer();
                            }
                        }
                    } else if(view_mode==VIEW_GRID && hov_db>=0){
                        /* right-click anywhere on a grid card cycles rating down */
                        int gi_rc=hov_db;
                        if(db[gi_rc].rating>0){ db[gi_rc].rating--; sfx_toggle(); save_defer(); }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if(ev.button.button==SDL_BUTTON_LEFT){
                    if(rz_drag){ SDL_GetWindowSize(win,&win_w,&win_h); apply_rgn(win_w,win_h,!win_maximized); }
                    win_drag=0; rz_drag=0; rz_active=RZ_NONE; drag_sb=0;
                    srch_drag=0; note_drag=0;
                    SDL_SetCursor(cur_arr);
                }
                break;

            case SDL_MOUSEMOTION:{
                int mx=ev.motion.x, my=ev.motion.y;
                /* If left button was released outside the window, cancel drag/resize */
                if((win_drag||rz_drag||drag_sb||srch_drag||note_drag) &&
                   !(SDL_GetGlobalMouseState(NULL,NULL) & SDL_BUTTON_LMASK)){
                    if(rz_drag){ SDL_GetWindowSize(win,&win_w,&win_h); apply_rgn(win_w,win_h,!win_maximized); }
                    win_drag=0; rz_drag=0; rz_active=RZ_NONE; drag_sb=0; srch_drag=0; note_drag=0;
                    SDL_SetCursor(cur_arr);
                }
                /* note drag-select */
                if(note_drag && note_open>=0){
                    int ax2,ay2,aw2,ah2; note_area(&ax2,&ay2,&aw2,&ah2);
                    int tp2=8, tw2=aw2-tp2*2;
                    /* clamp mouse to text area so selection stays sane */
                    int cmx=mx<ax2?ax2:(mx>=ax2+aw2?ax2+aw2-1:mx);
                    int cmy=my<ay2?ay2:(my>=ay2+ah2?ay2+ah2-1:my);
                    char *ns2=db[note_open].notes;
                    int len2=(int)strlen(ns2);
                    note_cur=note_px_to_pos(ns2,len2,ax2,tw2,ay2,cmx,cmy);
                    break;
                }
                /* when overlay is open, don't update row/grid hover */
                if(note_open>=0) break;
                hov_db=-1;
                if(view_mode==VIEW_GRID){
                    if(my>=LST_Y&&my<LST_Y+LST_H_&&mx>=0&&mx<LIST_W_){
                        int cols=grid_cols();
                        int block_w=cols*(GRID_W+GRID_GAP)-GRID_GAP;
                        int ox=(LIST_W_-block_w)/2;
                        int pg_cnt2=page_last()-page_first()+1;
                        int rows_pg=(pg_cnt2+cols-1)/cols; if(rows_pg<1) rows_pg=1;
                        int used_h2=rows_pg*(GRID_H+GRID_GAP)-GRID_GAP;
                        int oy2=LST_Y+(LST_H_-used_h2)/2; if(oy2<LST_Y) oy2=LST_Y;
                        int row2=(my-oy2)/(GRID_H+GRID_GAP);
                        int col2=(mx-ox)/(GRID_W+GRID_GAP);
                        if(col2>=0&&col2<cols&&row2>=0){
                            int local=row2*cols+col2;
                            int ri=page_first()+local;
                            int cy2=oy2+row2*(GRID_H+GRID_GAP);
                            int cx2=ox+col2*(GRID_W+GRID_GAP);
                            if(mx>=cx2&&mx<cx2+GRID_W&&my>=cy2&&my<cy2+GRID_H&&ri<=page_last()&&ri<nflt)
                                hov_db=flt[ri];
                        }
                    }
                } else {
                    int r=row_at(mx,my); if(r>=0) hov_db=flt[r];
                }
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

                /* ── Notes overlay keyboard ── */
                if(note_open>=0){
                    if(sym==SDLK_ESCAPE){ sfx_click(); note_open=-1; note_scroll=0; save_defer(); break; }
                    char *ns=db[note_open].notes;
                    int  len=(int)strlen(ns);
                    int  has_sel=(note_sel0!=note_cur);
                    /* helper: delete selection */
                    #define NOTE_DEL_SEL() do{ \
                        int a_=note_sel0<note_cur?note_sel0:note_cur; \
                        int b_=note_sel0<note_cur?note_cur:note_sel0; \
                        memmove(ns+a_,ns+b_,len-b_+1); \
                        note_cur=note_sel0=a_; len=(int)strlen(ns); \
                        has_sel=0; save_defer(); \
                    }while(0)
                    if(sym==SDLK_BACKSPACE){
                        if(has_sel){ NOTE_DEL_SEL(); }
                        else if(note_cur>0){
                            if(ctrl){ /* Ctrl+Backspace: delete word left */
                                int np=note_cur;
                                while(np>0&&ns[np-1]==' ') np--;
                                while(np>0&&ns[np-1]!=' '&&ns[np-1]!='\n') np--;
                                memmove(ns+np,ns+note_cur,len-note_cur+1);
                                note_cur=note_sel0=np;
                            } else {
                                memmove(ns+note_cur-1,ns+note_cur,len-note_cur+1);
                                note_cur--; note_sel0=note_cur;
                            }
                            save_defer();
                        }
                        note_ensure_visible();
                    } else if(sym==SDLK_DELETE){
                        if(has_sel){ NOTE_DEL_SEL(); }
                        else if(note_cur<len){
                            memmove(ns+note_cur,ns+note_cur+1,len-note_cur);
                            save_defer();
                        }
                        note_ensure_visible();
                    } else if(sym==SDLK_LEFT){
                        if(!shift&&has_sel){
                            int a=note_sel0<note_cur?note_sel0:note_cur;
                            note_cur=note_sel0=a;
                        } else {
                            int np=ctrl?(note_cur>0?(note_cur-1):0):note_cur>0?note_cur-1:0;
                            if(ctrl){ while(np>0&&ns[np-1]!=' '&&ns[np-1]!='\n') np--; }
                            note_cur=np; if(!shift) note_sel0=np;
                        }
                        note_ensure_visible();
                    } else if(sym==SDLK_RIGHT){
                        if(!shift&&has_sel){
                            int b=note_sel0>note_cur?note_sel0:note_cur;
                            note_cur=note_sel0=b;
                        } else {
                            int np=note_cur<len?note_cur+1:len;
                            if(ctrl){ np=note_cur; while(np<len&&ns[np]!=' '&&ns[np]!='\n') np++; while(np<len&&ns[np]==' ') np++; }
                            note_cur=np; if(!shift) note_sel0=np;
                        }
                        note_ensure_visible();
                    } else if(sym==SDLK_HOME){
                        /* go to start of line */
                        int np=note_cur;
                        while(np>0&&ns[np-1]!='\n') np--;
                        note_cur=np; if(!shift) note_sel0=np;
                        note_ensure_visible();
                    } else if(sym==SDLK_END){
                        int np=note_cur;
                        while(np<len&&ns[np]!='\n') np++;
                        note_cur=np; if(!shift) note_sel0=np;
                        note_ensure_visible();
                    } else if(sym==SDLK_RETURN||sym==SDLK_KP_ENTER){
                        if(has_sel) NOTE_DEL_SEL();
                        len=(int)strlen(ns);
                        if(len<510){
                            memmove(ns+note_cur+1,ns+note_cur,len-note_cur+1);
                            ns[note_cur]='\n'; note_cur++; note_sel0=note_cur; save_defer();
                        }
                        note_ensure_visible();
                    } else if(ctrl&&sym==SDLK_a){
                        note_sel0=0; note_cur=len; /* select all */
                    } else if(ctrl&&sym==SDLK_c){
                        if(has_sel){
                            int a=note_sel0<note_cur?note_sel0:note_cur;
                            int b=note_sel0<note_cur?note_cur:note_sel0;
                            char tmp[512]={0}; int cl=b-a; if(cl>511)cl=511;
                            memcpy(tmp,ns+a,cl); SDL_SetClipboardText(tmp);
                        }
                    } else if(ctrl&&sym==SDLK_x){
                        if(has_sel){
                            int a=note_sel0<note_cur?note_sel0:note_cur;
                            int b=note_sel0<note_cur?note_cur:note_sel0;
                            char tmp[512]={0}; int cl=b-a; if(cl>511)cl=511;
                            memcpy(tmp,ns+a,cl); SDL_SetClipboardText(tmp);
                            NOTE_DEL_SEL();
                        }
                    } else if(ctrl&&sym==SDLK_v){
                        if(has_sel) NOTE_DEL_SEL();
                        char *clip=SDL_GetClipboardText();
                        if(clip&&*clip){
                            len=(int)strlen(ns);
                            int ins=(int)strlen(clip);
                            if(len+ins<511){
                                memmove(ns+note_cur+ins,ns+note_cur,len-note_cur+1);
                                memcpy(ns+note_cur,clip,ins);
                                note_cur+=ins; note_sel0=note_cur;
                            }
                            save_defer();
                        }
                        if(clip) SDL_free(clip);
                        note_ensure_visible();
                    }
                    #undef NOTE_DEL_SEL
                    break;
                }

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
                    if(sym==SDLK_ESCAPE){
                        if(cmd_open||genre_dd_open){
                            cmd_open=0; genre_dd_open=0; genre_dd_anim=0.f; sfx_click();
                        } else {
                            if(*srch||cur_tab!=T_ALL||n_filt_genres>0||filt_year) sfx_click();
                            srch[0]=0; n_filt_genres=0; filt_year=0; update_chip_band(); do_tab(0);
                        }
                    }
                    if(sym==SDLK_LEFT||sym==SDLK_PAGEUP)  { int p=cur_page; go_page(cur_page-1); if(cur_page!=p) sfx_tab(); }
                    if(sym==SDLK_RIGHT||sym==SDLK_PAGEDOWN){ int p=cur_page; go_page(cur_page+1); if(cur_page!=p) sfx_tab(); }
                    if(sym==SDLK_f){ sfx_click(); srch_focus();
                        /* Ctrl+F: select all existing text */
                        if(ctrl){ srch_sel0=0; srch_cur=(int)strlen(srch); }
                        /* Flush any pending TEXTINPUT so 'f' isn't inserted */
                        SDL_FlushEvent(SDL_TEXTINPUT);
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
                if(note_open>=0){
                    char *ns=db[note_open].notes;
                    /* delete selection first */
                    if(note_sel0!=note_cur){
                        int a=note_sel0<note_cur?note_sel0:note_cur;
                        int b=note_sel0<note_cur?note_cur:note_sel0;
                        int len2=(int)strlen(ns);
                        memmove(ns+a,ns+b,len2-b+1);
                        note_cur=note_sel0=a;
                    }
                    int len=(int)strlen(ns);
                    int ins=(int)strlen(ev.text.text);
                    if(len+ins<511){
                        memmove(ns+note_cur+ins,ns+note_cur,len-note_cur+1);
                        memcpy(ns+note_cur,ev.text.text,ins);
                        note_cur+=ins; note_sel0=note_cur;
                    }
                    sfx_type(); save_defer();
                    note_ensure_visible();
                } else if(s_on){
                    srch_insert(ev.text.text);
                    sfx_type(); scr_tgt=0; scr_f=0; rebuild();
                }
                break;
            }
        }

        if(needs_redraw || rz_drag || win_drag || drag_sb || srch_drag || note_open>=0 || note_anim>0.005f || badge_anim>0.005f || cmd_open || cmd_anim>0.005f){
            draw_background();
            draw_titlebar();
            draw_hdr();
            draw_chips();
            draw_tabs();
            draw_list();
            draw_sbar();
            draw_badge_popup();        /* status badge popup — below note overlay */
            draw_cmd_panel();          /* Command Center panel */
            draw_genre_dropdown();     /* genre list — above panel, below note overlay */
            draw_note_overlay();       /* overlay — above everything */
            draw_tb_tooltips();        /* titlebar hover labels     */
            /* ── Status tooltip ── */
            if(tip_bj>=0 && tip_bj<N_STATUS && note_open<0){
                int mx_t,my_t; SDL_GetMouseState(&mx_t,&my_t);
                const char *tname=SNAME[tip_bj+1]; /* SNAME[0]=All, statuses start at 1 */
                int tw2=txw_(f12,tname)+14;
                int th2=TTF_FontHeight(f12)+8;
                int tx3=mx_t-tw2/2, ty3=my_t-th2-8;
                if(tx3<4) tx3=4;
                if(tx3+tw2>win_w-4) tx3=win_w-4-tw2;
                if(ty3<4) ty3=4;
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                C4 tbg={20,20,30,220}; C4 tbrd=C_SEP;
                bfrr_aa(tx3,ty3,tw2,th2,R_SM,BRD_T,tbrd,tbg);
                rtxcen(f12,tname,tx3,ty3,tw2,th2,C_TXT);
                SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            }
            SDL_RenderPresent(ren);
        }
    }

    save_flush(); /* write any pending deferred save */
    save_d();
    tc_free_all();
    if(aud_dev) SDL_CloseAudioDevice(aud_dev);
    sfx_free();
    TTF_CloseFont(f22); TTF_CloseFont(f18);
    TTF_CloseFont(f14); TTF_CloseFont(f12);
    if(f_icon) TTF_CloseFont(f_icon);
    TTF_Quit();
    SDL_FreeCursor(cur_arr); SDL_FreeCursor(cur_ns); SDL_FreeCursor(cur_ew);
    SDL_FreeCursor(cur_nwse); SDL_FreeCursor(cur_nesw);
    bg_free();
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}