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
#ifdef _WIN32
#include <windows.h>
#endif
#include "games.h"
#include "themes.h"

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
#define NOTE_BTN_SZ  BTN_SZ                              /* same size as status buttons      */
#define NOTE_BTN_GAP 14                                  /* gap between note btn and status  */
#define NOTE_BTN_X_  (BTN_LX_ - NOTE_BTN_SZ - NOTE_BTN_GAP) /* left of status buttons      */
#define RAT_W    40                       /* star rating column              */
#define RAT_X_  (NOTE_BTN_X_ - RAT_W - 10)  /* left of note button          */
#define YR_X_   (RAT_X_ - 12 - YR_W)
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
#define SAVE_VER 0xCA7A2029u

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
typedef struct {
    char name[128];
    char genre[32];
    int  year;
    char st[N_STATUS];
    char name_lc[128]; /* pre-computed lowercase for fast search */
    char genre_lc[32]; /* pre-computed lowercase for fast search */
    int  rating;       /* 0 = unrated, 1–10                      */
    char notes[512];   /* personal notes / review                 */
} Game;

typedef enum {
    T_ALL=0,T_WISH,T_PLAYED,T_PLAYING,
    T_FINISHED,T_DROPPED,T_FAV,T_ROTATION,T_STATS
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
static TabId prev_tab=T_ALL; /* tab to return to when closing stats */
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
typedef enum { SORT_AZ=0, SORT_ZA, SORT_NEW, SORT_OLD, SORT_RATING } SortMode;
typedef enum { VIEW_LIST=0, VIEW_GRID } ViewMode;
static SortMode sort_mode = SORT_AZ;
static ViewMode view_mode = VIEW_LIST;
static float tc_sort_hov[4];
static float tc_view_hov[2];
static float tc_stats_hov = 0.f;  /* stats button hover */
static float sort_ind_f = 0.f;   /* sliding pill – index space */
static float view_ind_f = 0.f;   /* sliding pill – view mode   */
/* ── Notes overlay ──────────────────────────────────────────────── */
static int   note_open = -1;   /* db[] index of open note, -1=closed */
static float note_anim = 0.f;  /* 0..1 fade in/out                   */
static int   note_cur  = 0;    /* cursor position in notes string     */
static int   note_sel0 = 0;    /* selection anchor (like srch_sel0)   */
static int   note_drag = 0;    /* 1 while drag-selecting inside notes */
/* sort dropdown */
static int   sort_dd_open  = 0;   /* 1 = dropdown visible              */
static float sort_dd_anim  = 0.f; /* 0..1 open/close animation         */
static float sort_btn_hov  = 0.f; /* button hover                      */
static float sort_item_hov[5];    /* per-item hover                     */
#define DD_ITEM_H  26
#define DD_W       100
#define DD_BTN_H   TC_H
#define DD_BTN_W   (DD_W)
static float tab_ix=8.f, tab_itx=8.f;
static float tb_ch=0.f, tb_mh=0.f, tb_nh=0.f;
static float srch_spin=0.f;
static float srch_glow=0.f;
static Uint64 plast=0;
static float  dt_=0.016f;



/* ═══════════════════════ SDL globals ═══════════════════════════ */
static SDL_Window   *win=NULL;
SDL_Renderer *ren=NULL;
static TTF_Font     *f22=NULL,*f18=NULL,*f14=NULL,*f12=NULL;
static SDL_Cursor   *cur_arr=NULL,*cur_ns=NULL,*cur_ew=NULL,
                    *cur_nwse=NULL,*cur_nesw=NULL;



/* ═══════════════════════ Audio / SFX ═══════════════════════════ */
static SDL_AudioDeviceID aud_dev=0;

/* Deferred save — avoids a file write on every rapid button click */
static void save_d(void); /* forward declaration */
static Uint32 save_pending_at = 0;  /* ticks when last toggle dirtied the save */
#define SAVE_DEFER_MS 500           /* write 500 ms after the last change */
static void save_defer(void){ save_pending_at = SDL_GetTicks(); }
static void save_flush(void){ if(save_pending_at){ save_d(); save_pending_at=0; } }

#define SFX_RATE 44100

/* Pre-baked SFX buffers — generated once at startup, reused on every play */
typedef struct { Sint16 *buf; int n; } SfxBuf;
static SfxBuf sfx_click_buf, sfx_toggle_buf, sfx_tab_buf,
              sfx_type_buf,  sfx_sort_buf;

static SfxBuf sfx_bake(float freq, float dur, float vol, float decay){
    SfxBuf b; b.n = (int)(SFX_RATE * dur);
    b.buf = (Sint16*)malloc(b.n * sizeof(Sint16));
    if(!b.buf){ b.n=0; return b; }
    for(int i = 0; i < b.n; i++){
        float t = (float)i / SFX_RATE;
        float s = sinf(2.f*(float)M_PI*freq*t) * expf(-t*decay) * vol * 32767.f;
        if(s >  32767.f) s =  32767.f;
        if(s < -32767.f) s = -32767.f;
        b.buf[i] = (Sint16)s;
    }
    return b;
}
static void sfx_queue(SfxBuf b){
    if(!aud_dev || !b.buf) return;
    SDL_QueueAudio(aud_dev, b.buf, b.n * sizeof(Sint16));
}

/* Cooldown timestamps — one per sfx category */
static Uint32 sfx_last_click=0, sfx_last_toggle=0, sfx_last_tab=0, sfx_last_type=0, sfx_last_sort=0;
static void sfx_click (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_click <60)  return; sfx_last_click =now; sfx_queue(sfx_click_buf);  }
static void sfx_toggle(void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_toggle<30)  return; sfx_last_toggle=now; sfx_queue(sfx_toggle_buf); }
static void sfx_tab   (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_tab  <50)  return; sfx_last_tab   =now; sfx_queue(sfx_tab_buf);    }
static void sfx_type  (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_type <30)  return; sfx_last_type  =now; sfx_queue(sfx_type_buf);   }
static void sfx_sort  (void){ Uint32 now=SDL_GetTicks(); if(now-sfx_last_sort <200) return; sfx_last_sort  =now; sfx_queue(sfx_sort_buf);   }

/* ── Forward declarations ─────────────────────────────────────── */
static void rebuild(void);
static int  grid_cols(void);
static int  row_at(int mx,int my);
static int  btn_at(int mx,int my,int ri);
static void draw_note_overlay(void);
static void draw_stats(void);

/* ═══════════════════════ Draw helpers ══════════════════════════ */
void sc_(C4 c){ SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a); }
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
    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);}
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
    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);}
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
    { float sp=1.f-powf(0.00004f,dt_); SETTLE(scr_f,scr_tgt,sp); }

    /* ── Grid/List hover detection (polled every tick) ── */
    if(note_open>=0){
        hov_db=-1;
    } else {
        int mx4,my4; SDL_GetMouseState(&mx4,&my4);
        int dd_over=(sort_dd_open||sort_dd_anim>0.05f)&&mx4>=TC_X0&&mx4<TC_X0+DD_W&&my4>=TITLE_H&&my4<TITLE_H+5*DD_ITEM_H+8;
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
                if(r4>=0) hov_db=flt[r4];
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

    /* ── Button flash fade (only non-zero entries) ── */
    {
        float bd=dt_*5.0f;
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
        if(cur_tab<N_TABS) tab_itx=(float)TAB_X_((int)cur_tab);
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

        /* ── Sort/view button hover ── */
        float ths2=1.f-powf(0.001f,dt_);
        int mx3,my3; SDL_GetMouseState(&mx3,&my3);
        int hf2=(SDL_GetWindowFlags(win)&SDL_WINDOW_MOUSE_FOCUS)!=0;
        /* sort button hover */
        {
            int in_btn=(hf2&&my3>=TC_Y&&my3<TC_Y+DD_BTN_H&&mx3>=TC_X0&&mx3<TC_X0+DD_BTN_W);
            SETTLE(sort_btn_hov,(float)in_btn,ths2);
        }
        /* sort dropdown item hovers — fast dedicated speed */
        float dd_item_spd = 1.f-powf(0.000005f, dt_);
        for(int i=0;i<5;i++){
            int item_y=TITLE_H+4+i*DD_ITEM_H;
            int in_item=(sort_dd_open&&sort_dd_anim>0.05f&&hf2&&
                         mx3>=TC_X0&&mx3<TC_X0+DD_W&&
                         my3>=item_y&&my3<item_y+DD_ITEM_H);
            SETTLE(sort_item_hov[i],(float)in_item,dd_item_spd);
        }
        /* dropdown animation — snappy open, quick close */
        {
            float dd_tgt = sort_dd_open ? 1.f : 0.f;
            float dd_spd = 1.f-powf(sort_dd_open ? 0.000002f : 0.00002f, dt_);
            SETTLE(sort_dd_anim, dd_tgt, dd_spd);
        }
        for(int i=0;i<2;i++){
            int bx=TC_X0+DD_BTN_W+20+i*(VC_W+TC_GAP);
            float tg=(hf2&&my3>=0&&my3<TITLE_H&&mx3>=bx&&mx3<bx+VC_W)?1.f:0.f;
            SETTLE(tc_view_hov[i],tg,ths2);
        }
        {
            int sbx_s=TC_X0+DD_BTN_W+20+2*(VC_W+TC_GAP)-TC_GAP+TC_GAP+4;
            float tg=(hf2&&my3>=0&&my3<TITLE_H&&mx3>=sbx_s&&mx3<sbx_s+VC_W)?1.f:0.f;
            SETTLE(tc_stats_hov,tg,ths2);
        }

        /* ── Button hover (skip entries already at target) ── */
        float bspd=1.f-powf(0.00015f,dt_);
        int hov_gi=-1,hov_bj=-1;
        /* suppress row/button hover when sort dropdown covers this area */
        int dd_blocks_hover = (sort_dd_open||sort_dd_anim>0.05f)
                              && hf2 && mx2>=TC_X0 && mx2<TC_X0+DD_W
                              && my2>=TITLE_H && my2<TITLE_H+5*DD_ITEM_H+8;
        if(!dd_blocks_hover){
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
        } /* end !dd_blocks_hover */
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

/* qsort comparators for rebuild() */
static int cmp_flt_az (const void *a,const void *b){ return strcasecmp(db[*(int*)a].name,db[*(int*)b].name); }
static int cmp_flt_za (const void *a,const void *b){ return strcasecmp(db[*(int*)b].name,db[*(int*)a].name); }
static int cmp_flt_new(const void *a,const void *b){ return db[*(int*)b].year - db[*(int*)a].year; }
static int cmp_flt_old(const void *a,const void *b){ return db[*(int*)a].year - db[*(int*)b].year; }
static int cmp_flt_rating(const void *a,const void *b){
    /* highest rating first; unrated sink to bottom; ties A-Z */
    int ra=db[*(int*)a].rating, rb=db[*(int*)b].rating;
    /* treat 0 (unrated) as -1 so it sinks below rated entries */
    int sa=ra?ra:-1, sb=rb?rb:-1;
    if(sb!=sa) return sb-sa;
    return strcasecmp(db[*(int*)a].name,db[*(int*)b].name);
}

static void rebuild(void){
    if(cur_tab==T_STATS){ nflt=0; return; }
    nflt=0;
    char ql[128]; int qlen=(int)strlen(srch);
    strlower(srch,ql,128);

    for(int i=0;i<ndb;i++){
        if(cur_tab!=T_ALL&&!db[i].st[(int)cur_tab-1]) continue;
        if(qlen>0){
            int s=search_score(db[i].name_lc,db[i].genre_lc,ql,qlen);
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
        switch(sort_mode){
        case SORT_AZ:     qsort(flt,nflt,sizeof(int),cmp_flt_az);     break;
        case SORT_ZA:     qsort(flt,nflt,sizeof(int),cmp_flt_za);     break;
        case SORT_NEW:    qsort(flt,nflt,sizeof(int),cmp_flt_new);    break;
        case SORT_OLD:    qsort(flt,nflt,sizeof(int),cmp_flt_old);    break;
        case SORT_RATING: qsort(flt,nflt,sizeof(int),cmp_flt_rating); break;
        }
    }
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
    /* v2: rating and notes */
    for(int i=0;i<ndb;i++) fwrite(&db[i].rating,sizeof(int),1,fp);
    for(int i=0;i<ndb;i++) fwrite(db[i].notes,512,1,fp);
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
    { int sm=0; if(fread(&sm,sizeof(int),1,fp)==1&&sm>=0&&sm<5) sort_mode=(SortMode)sm; }
    { int vm=0; if(fread(&vm,sizeof(int),1,fp)==1&&vm>=0&&vm<2) view_mode=(ViewMode)vm; }
    /* v2: rating and notes — guarded so old saves load cleanly */
    for(int i=0;i<lim;i++){ int r=0; if(fread(&r,sizeof(int),1,fp)==1&&r>=0&&r<=10) db[i].rating=r; }
    for(int i=0;i<lim;i++) fread(db[i].notes,512,1,fp);
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

static void draw_tbbtn(TBBtnType type, int cx2, float ht){
    int cy  = TITLE_H / 2;
    int bw  = TB_BTN_W;   /* full button width  */
    int bh  = TITLE_H;    /* full button height */
    int bx  = cx2 - bw/2; /* left edge          */

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* Hover background — full-height rectangle, Windows 11 style */
    if(ht > 0.005f){
        C4 bg = (type == TB_CLOSE_BTN)
            ? MK4(196, 43, 28,  (Uint8)(ht * 240))   /* Win11 red   */
            : MK4(255, 255, 255,(Uint8)(ht * 38));    /* subtle grey */
        SDL_Rect r = {bx, 0, bw, bh};
        sc_(bg); SDL_RenderFillRect(ren, &r);
    }

    /* Icon — dim white at rest, full white on hover */
    Uint8 ia = (Uint8)(110 + ht * 145);
    C4 ic = MK4(255, 255, 255, ia);

    int s = 5;
    switch(type){
    case TB_CLOSE_BTN:
        rline(cx2-s, cy-s, cx2+s, cy+s, ic);
        rline(cx2+s, cy-s, cx2-s, cy+s, ic);
        break;
    case TB_MAX_BTN:
        if(win_maximized){
            /* Restore icon: two offset squares */
            C4 back = ic; back.a = (Uint8)(ic.a * 0.45f);
            frr_aa(cx2-s,   cy-s+2, s*2-2, s*2-2, 1, back);
            frr_aa(cx2-s+2, cy-s,   s*2-2, s*2-2, 1, ic);
        } else {
            /* Maximize icon: single square outline */
            frr_aa(cx2-s, cy-s, s*2, s*2, 1, ic);
        }
        break;
    case TB_MIN_BTN:
        rline(cx2-s, cy, cx2+s, cy, ic);
        break;
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
    bfrr_aa(TC_X0, TITLE_H+1, DD_W, total_h+2, 7, 1, dbc, dbg);

    /* items — sliding pill follows sort_ind_f, same as tab/sort pill */
    {
        /* draw the sliding selection pill */
        float pill_y = TITLE_H+4 + sort_ind_f * DD_ITEM_H;
        C4 pill = {C_ACC.r, C_ACC.g, C_ACC.b, (Uint8)(180*a)};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        frr_aa(TC_X0+3, (int)(pill_y+0.5f)+2, DD_W-6, DD_ITEM_H-4, 5, pill);
    }
    for(int i=0;i<5;i++){
        int iy = TITLE_H+4+i*DD_ITEM_H;  /* shifted +2 for equal top/bottom gap */
        float hv = sort_item_hov[i];
        int act = (sort_mode==(SortMode)i);
        int ty = iy+(DD_ITEM_H-TTF_FontHeight(f14))/2;

        /* hover highlight on non-active items */
        if(!act && hv > 0.01f){
            C4 hi = {255,255,255,(Uint8)(hv*30*a)};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            frr_aa(TC_X0+3, iy+2, DD_W-6, DD_ITEM_H-4, 5, hi);
        }

        C4 lc = act ? MK4(255,255,255,(Uint8)(255*a))
                    : lerpc(C_DIM, C_TXT, hv*0.7f);
        if(!act) lc.a = (Uint8)(lc.a * a);

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        rtx(f14, slbl[i], TC_X0+8, ty, lc);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(ren, NULL);
}

static void draw_titlebar_dots(void);
static void draw_titlebar(void){
    { C4 c=C_TITLE; c.a=175; fblend(0,0,win_w,TITLE_H,c); }
    {
        C4 tl=C_ACC; tl.a=120;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(tl);
        SDL_RenderDrawLine(ren,0,TITLE_H-1,win_w,TITLE_H-1);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    static const char *slbl[5]={"A \xe2\x86\x91 Z","Z \xe2\x86\x93 A","Newest","Oldest","Top Rated"};

    /* ── Sort dropdown button ── */
    {
        /* When dropdown is open, keep button fully "active" regardless of hover */
        float hv = sort_dd_open ? 1.f : sort_btn_hov;
        C4 bg  = lerpc(C_BG, C_ACC, 0.18f * hv); bg.a = 255;
        C4 bc  = lerpc(C_SEP, C_ACC, hv * 0.9f); bc.a = (Uint8)(80 + hv*120);
        bfrr_aa(TC_X0, TC_Y, DD_BTN_W, DD_BTN_H, 5, 2, bc, bg);
        /* label = current sort mode */
        C4 lc = C_TXT; lc.a = (Uint8)(150 + hv*105);
        int lw2 = txw_(f14, slbl[sort_mode]);
        /* chevron down — 5px wide */
        int chev_x = TC_X0+DD_BTN_W-14;
        int chev_y = TC_Y+DD_BTN_H/2;
        float rot  = sort_dd_anim; /* 0=down, 1=up */
        rtx(f14, slbl[sort_mode], TC_X0+8, TC_Y+(DD_BTN_H-TTF_FontHeight(f14))/2, lc);
        /* animated chevron: rotates 180° when open */
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        C4 chev_c = lerpc(C_DIM, C_TXT, hv);
        int cy2_chev = chev_y + (int)((1.f-rot)*2.f - 1.f); /* slight vertical shift */
        int arm = 3;
        /* top-half of chevron (pointing down when closed, up when open) */
        float flip = 1.f - 2.f*rot; /* +1 down, -1 up */
        rline(chev_x-arm, cy2_chev-(int)(flip*arm/2),
              chev_x,      cy2_chev+(int)(flip*arm/2), chev_c);
        rline(chev_x+arm, cy2_chev-(int)(flip*arm/2),
              chev_x,      cy2_chev+(int)(flip*arm/2), chev_c);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    /* separator between sort button and view toggle */
    {
        int sx = TC_X0 + DD_BTN_W + 8;
        C4 sc2=C_DIM; sc2.a=80;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        sc_(sc2);
        SDL_RenderDrawLine(ren,sx,TC_Y+2,sx,TC_Y+TC_H-2);
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    }

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    {
        int vx0=TC_X0+DD_BTN_W+20;
        float px2 = vx0 + view_ind_f*(float)(VC_W+TC_GAP);
        C4 vbg  = lerpc(C_BG, C_ACC, 0.18f); vbg.a = 255;
        C4 vbdr = C_ACC; vbdr.a = 200;
        bfrr_aa((int)(px2+0.5f), TC_Y, VC_W, TC_H, 5, 2, vbdr, vbg);
    }
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
    {
        int vx0=TC_X0+DD_BTN_W+20;
        for(int i=0;i<2;i++){
            int bx=vx0+i*(VC_W+TC_GAP);
            int act=(view_mode==(ViewMode)i);
            float hv=tc_view_hov[i];
            C4 ic = act ? C_TXT : lerpc(C_DIM, C_SUB, hv);
            sc_(ic);
            int cx=bx+VC_W/2, cy=TC_Y+TC_H/2;
            if(i==0){
                /* List icon: 3×2px lines, offsets -4,-1,+2 → spans cy-4..cy+3, centered on cy */
                int lw2=14;
                int offs[3]={-4,-1,2};
                for(int k=0;k<3;k++){
                    SDL_RenderDrawLine(ren,cx-lw2/2,cy+offs[k],  cx+lw2/2-1,cy+offs[k]);
                    SDL_RenderDrawLine(ren,cx-lw2/2,cy+offs[k]+1,cx+lw2/2-1,cy+offs[k]+1);
                }
            } else {
                /* Grid icon: 2×2 squares, sz=5, gap=4 → 14px total */
                int sz=5, gap=4, half=(sz*2+gap)/2;
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

    /* ── Stats toggle button — directly after view buttons ── */
    {
        int vx0   = TC_X0+DD_BTN_W+20;
        int v_end = vx0 + 2*(VC_W+TC_GAP) - TC_GAP; /* right edge of last view btn */
        /* separator sits midway between grid button and stats button */
        int sbx   = v_end + TC_GAP + 4;   /* stats button left edge */
        int sep_x = v_end + (sbx - v_end)/2; /* centred in the gap */
        int act   = (cur_tab==T_STATS);
        float hv  = tc_stats_hov;

        /* separator */
        { C4 sc2=C_DIM; sc2.a=80;
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND); sc_(sc2);
          SDL_RenderDrawLine(ren,sep_x,TC_Y+2,sep_x,TC_Y+TC_H-2);
          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

        /* active bg pill */
        if(act){
            C4 vbg=lerpc(C_BG,C_ACC,0.18f); vbg.a=255;
            C4 vbd=C_ACC; vbd.a=200;
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            bfrr_aa(sbx,TC_Y,VC_W,TC_H,5,2,vbd,vbg);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
        }

        /* 3-bar chart icon — pixel-perfect centred in VC_W×TC_H */
        { C4 ic = act ? C_ACC : lerpc(C_DIM,C_SUB,hv);
          sc_(ic);
          /* bars: w=3, gap=2 → total icon width=13, heights 4/9/6 */
          int bw2=3, bg2=2, total_iw=bw2*3+bg2*2; /* 13 */
          int total_ih=9; /* tallest bar */
          int ix0 = sbx + (VC_W - total_iw)/2;
          int iy0 = TC_Y + (TC_H - total_ih)/2 + total_ih; /* bottom baseline */
          int hs[3]={4,9,6};
          for(int k=0;k<3;k++){
              SDL_Rect b={ix0+k*(bw2+bg2), iy0-hs[k], bw2, hs[k]};
              SDL_RenderFillRect(ren,&b);
          }
        }
    }

    draw_tbbtn(TB_CLOSE_BTN, TB_CX, tb_ch);
    draw_tbbtn(TB_MAX_BTN,   TB_MX, tb_mh);
    draw_tbbtn(TB_MIN_BTN,   TB_NX, tb_nh);
}

/* ── Header ───────────────────────────────────────────────────── */
static void draw_hdr(void){
    { C4 c=C_HDR; c.a=160; fblend(0,HDR_Y,win_w,HDR_H,c); }
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

                /* Galaxy extra effects — layered behind the base dot, scaled by gf */
                if(i==8 && galaxy_dot_f>0.15f){
                    float gf=galaxy_dot_f;
                    int extra=(int)(gf*3.f);  /* 0..3 extra px, floor so it drops cleanly */
                    /* outer nebula haze */
                    int hr=TDOT_R+extra+(int)(gf*5.f);
                    C4 haze={180,60,255,(Uint8)(gf*(35+hf*30))};
                    frr_aa(cx-hr,dots_cy+yoff-hr,hr*2,hr*2,hr,haze);
                    /* inner glow ring */
                    C4 glow={210,100,255,(Uint8)(gf*(80+hf*50))};
                    int gr1=TDOT_R+extra;
                    frr_aa(cx-gr1-1,dots_cy+yoff-gr1-1,(gr1+1)*2,(gr1+1)*2,gr1+1,glow);
                    /* diffraction spikes */
                    int sd=TDOT_R+extra+(int)(gf*4.f);
                    C4 sp={255,200,255,(Uint8)(gf*200)}; sc_(sp);
                    SDL_RenderDrawPoint(ren,cx,    dots_cy+yoff-sd);
                    SDL_RenderDrawPoint(ren,cx,    dots_cy+yoff+sd);
                    SDL_RenderDrawPoint(ren,cx-sd, dots_cy+yoff);
                    SDL_RenderDrawPoint(ren,cx+sd, dots_cy+yoff);
                    C4 sp2={255,200,255,(Uint8)(gf*100)}; sc_(sp2);
                    SDL_RenderDrawPoint(ren,cx,      dots_cy+yoff-sd-1);
                    SDL_RenderDrawPoint(ren,cx,      dots_cy+yoff+sd+1);
                    SDL_RenderDrawPoint(ren,cx-sd-1, dots_cy+yoff);
                    SDL_RenderDrawPoint(ren,cx+sd+1, dots_cy+yoff);
                    C4 sp3={200,150,255,(Uint8)(gf*80)}; sc_(sp3);
                    SDL_RenderDrawPoint(ren,cx-sd+2, dots_cy+yoff-sd+2);
                    SDL_RenderDrawPoint(ren,cx+sd-2, dots_cy+yoff-sd+2);
                    SDL_RenderDrawPoint(ren,cx-sd+2, dots_cy+yoff+sd-2);
                    SDL_RenderDrawPoint(ren,cx+sd-2, dots_cy+yoff+sd-2);
                }

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

        /* Galaxy extra effects — drawn behind base dot, fully scaled by gf */
        if(i==8 && galaxy_dot_f>0.15f){
            float gf=galaxy_dot_f;
            /* steady outer haze ring */
            int hr=r2+2+(int)(gf*4.f);
            C4 haze={180,60,255,(Uint8)(gf*(45+hf*35))};
            frr_aa(cx-hr,dots_cy+yoff-hr,hr*2,hr*2,hr,haze);
            /* tight glow ring */
            C4 glow={230,120,255,(Uint8)(gf*(110+hf*60))};
            frr_aa(cx-r2-1,dots_cy+yoff-r2-1,(r2+1)*2,(r2+1)*2,r2+1,glow);
            /* spikes — reach r2+3 at full gf, all alpha scaled */
            int sd=r2+1+(int)(gf*3.f);
            C4 sp={255,210,255,(Uint8)(gf*230)}; sc_(sp);
            for(int d=0;d<=1;d++){
                SDL_RenderDrawPoint(ren,cx+d,  dots_cy+yoff-sd);
                SDL_RenderDrawPoint(ren,cx+d,  dots_cy+yoff+sd);
                SDL_RenderDrawPoint(ren,cx-sd, dots_cy+yoff+d);
                SDL_RenderDrawPoint(ren,cx+sd, dots_cy+yoff+d);
            }
            C4 sp2={255,200,255,(Uint8)(gf*110)}; sc_(sp2);
            SDL_RenderDrawPoint(ren,cx,      dots_cy+yoff-sd-1);
            SDL_RenderDrawPoint(ren,cx,      dots_cy+yoff+sd+1);
            SDL_RenderDrawPoint(ren,cx-sd-1, dots_cy+yoff);
            SDL_RenderDrawPoint(ren,cx+sd+1, dots_cy+yoff);
        }

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

/* ── Tabs ─────────────────────────────────────────────────────── */
static void draw_tabs(void){
    int tw=TAB_W_;
    { C4 c=C_TBAR; c.a=165; fblend(0,TAB_Y,win_w,TAB_H,c); }

    {
        int ix=(int)tab_ix;
        C4 tab_bg = lerpc(C_BG, C_ACC, 0.18f); tab_bg.a=255;
        C4 border  = C_ACC; border.a = 200;
        bfrr_aa(ix+2, TAB_Y+3, tw-4, TAB_H-3, 6, 2, border, tab_bg);
    }

    for(int i=0;i<N_TABS;i++){
        int tx=TAB_X_(i);
        int act=(cur_tab==(TabId)i);
        char lbl[48];
        if(i==0) sprintf(lbl,"All");
        else      sprintf(lbl,"%s",SNAME[i]);
        C4 tc;
        if(i==0)               tc=(act?C_ACC:C_SUB);
        else if(i<N_TABS-1)    tc=(act?SCOL[i-1]:C_SUB);
        else                   tc=(act?C_ACC:C_SUB); /* Stats tab */
        rtxcen(f12,lbl,tx,TAB_Y,tw,TAB_H,tc);
    }
}

/* ── Row ──────────────────────────────────────────────────────── */
static void draw_row(int ri, int ay){
    int lw=LIST_W_;
    int gi=flt[ri]; Game *g=&db[gi];
    float ht=row_ht[gi];

    frr_aa(4,ay+3,lw-8,ROW_H-6,7,lerpc((ri%2==0)?C_ROWA:C_ROWB,C_ROWH,ht));

    int fh18=TTF_FontHeight(f18), fh12=TTF_FontHeight(f12);

    /* Name — single line, vertically centred */
    rtxclip(f18,g->name,NM_X,ay+(ROW_H-fh18)/2,NM_MW_,
            lerpc(C_TXT,MK4(255,255,255,255),ht*0.4f));

    /* Year + genre stacked, centred as a block in the year column */
    char yr[8]; sprintf(yr,"%d",g->year);
    int yr_block_h = fh12 + 2 + fh12;
    int yr_y    = ay + (ROW_H - yr_block_h) / 2;
    int genre_y = yr_y + fh12 + 2;
    rtxcen(f12,yr,       YR_X_,yr_y,    YR_W,fh12,C_SUB);
    rtxcen(f12,g->genre, YR_X_,genre_y, YR_W,fh12,lerpc(C_DIM,C_SUB,ht*0.5f));

    int bby=ay+BTN_YO;

    /* ── Rating ── */
    {
        int ry=ay+(ROW_H-TTF_FontHeight(f12))/2;
        if(g->rating>0){
            char rbuf[8];
            snprintf(rbuf,sizeof(rbuf),"\xe2\x98\x85 %d",g->rating);
            float t=((float)g->rating-1.f)/9.f;
            C4 rc=lerpc(MK4(150,130,50,255),SCOL[S_FAV],t);
            rtxcen(f12,rbuf,RAT_X_,ry,RAT_W,TTF_FontHeight(f12),rc);
        } else {
            rtxcen(f12,"\xe2\x80\x93",RAT_X_,ry,RAT_W,TTF_FontHeight(f12),C_DIM);
        }
    }

    /* ── Notes button ── */
    { int nbx=NOTE_BTN_X_, nby=bby;
      int has_note=(g->notes[0]!=0);
      C4 nb_bg  = lerpc(C_BTNI, has_note?C_ACC:C_SEP, ht*0.5f);
      C4 nb_brd = lerpc(C_SEP,  has_note?C_ACC:C_SUB, ht*0.7f);
      bfrr(nbx,nby,NOTE_BTN_SZ,NOTE_BTN_SZ,5,1,nb_brd,nb_bg);
      C4 ic2 = lerpc(C_DIM, has_note?C_ACC:C_TXT, ht*0.5f); sc_(ic2);
      int ix=nbx+NOTE_BTN_SZ/2-5, iy=nby+NOTE_BTN_SZ/2-5;
      for(int li=0;li<3;li++)
          SDL_RenderDrawLine(ren,ix,iy+li*4,ix+10,iy+li*4);
    }

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
        bfrr(bx,bby,BTN_SZ,BTN_SZ,5,1,brc,bg2);
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

    /* (rating now shown between name and genre/year — see below) */

    /* ── Notes button — below accent strip, top-right of card ── */
    { int nb=18, nbx=x+GRID_W-nb-4, nby=y+14;
      int has_note=(g->notes[0]!=0);
      C4 nb_bg  = lerpc(C_BTNI, has_note?C_ACC:C_SEP, ht*0.5f);
      C4 nb_brd = lerpc(C_SEP,  has_note?C_ACC:C_SUB, ht*0.7f);
      bfrr(nbx,nby,nb,nb,4,1,nb_brd,nb_bg);
      C4 ic2 = lerpc(C_DIM, has_note?C_ACC:C_TXT, ht*0.5f); sc_(ic2);
      int ix=nbx+nb/2-4, iy=nby+nb/2-4;
      for(int li=0;li<3;li++)
          SDL_RenderDrawLine(ren,ix,iy+li*3,ix+8,iy+li*3);
    }

    int tx  = x+8;
    int tw2 = GRID_W-16;
    int fh12= TTF_FontHeight(f12);
    int btn_by  = y + GRID_H - GBSZ - 4;
    int info_by = btn_by - fh12 - 5;
    /* rating line sits between name block and info line */
    int rat_by  = info_by - fh12 - 3;
    int name_y  = y + 11;
    int name_h  = rat_by - name_y - 4;

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
        (void)tc2;
    }

    /* Rating — centred between name block and genre/year */
    if(g->rating>0){
        char rbuf[10]; snprintf(rbuf,sizeof(rbuf),"\xe2\x98\x85 %d",g->rating);
        float t=((float)g->rating-1.f)/9.f;
        C4 rc=lerpc(MK4(150,130,50,220),SCOL[S_FAV],t);
        rtxcen(f12,rbuf,x,rat_by,GRID_W,fh12,rc);
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
    fr_(0,py,win_w,ph,C_SBAR);

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

    #define DRAW_NAV_BTN(bx2,active2,hov2,dir2) do {         float _hv=(hov2);         C4 _bc=(active2)?lerpc(C_SEP,C_ACC,_hv*0.9f):C_SEP;         C4 _ic=(active2)?lerpc(C_SRCH,C_SRCHA,_hv*0.4f):C_BTNI; _ic.a=255;         bfrr_aa(bx2,cy-bh/2,bw,bh,br,1,_bc,_ic);         C4 _ac=(active2)?lerpc(C_SUB,C_ACC,_hv):C_DIM;         draw_arrow((bx2)+bw/2,cy,4,(dir2),_ac);     } while(0)

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
    { C4 c=C_BG; c.a=145; fblend(0,LST_Y,LIST_W_,lh,c); }

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
    { C4 card=C_TBAR; card.a=180;
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
            int gf=-1;
            for(int k=0;k<ngnames;k++) if(strcmp(gnames[k],g->genre)==0){ gf=k; break; }
            if(gf<0&&ngnames<MAX_GENRES){ strncpy(gnames[ngnames],g->genre,31); gf=ngnames++; }
            if(gf>=0) gcounts[gf]++;
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
        struct { const char *lbl; int val; C4 col; } cs[3]={
            {"In Library", ndb,           C_TXT},
            {"Tracked",    total_tracked,  C_ACC},
            {"Untracked",  untracked,      C_DIM},
        };
        int cw = avail_w / 3;
        for(int i=0;i<3;i++){
            int cx = PAD + i*cw;
            char num[16]; snprintf(num,sizeof(num),"%d",cs[i].val);
            /* big number */
            int nw=txw_(f22,num);
            rtx(f22,num, cx+(cw-nw)/2, y, cs[i].col);
            /* label underneath */
            int lw2=txw_(f12,cs[i].lbl);
            C4 lc=C_DIM; if(i==1)lc=C_SUB;
            rtx(f12,cs[i].lbl, cx+(cw-lw2)/2, y+fh18+3, lc);
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
        int pw[N_STATUS], total_pw=0;
        for(int j=0;j<N_STATUS;j++){
            char buf[32]; snprintf(buf,sizeof(buf),"%s  %d",SLBL[j],by_status[j]);
            pw[j]=txw_(f12,buf)+18; total_pw+=pw[j];
        }
        int pill_h=24;
        int gap2=(avail_w-total_pw)/(N_STATUS-1); if(gap2<6)gap2=6;
        int px=PAD;
        for(int j=0;j<N_STATUS;j++){
            char buf[32]; snprintf(buf,sizeof(buf),"%s  %d",SLBL[j],by_status[j]);
            C4 bg2=SCOL[j]; bg2.r/=4; bg2.g/=4; bg2.b/=4; bg2.a=220;
            frr_aa(px,y,pw[j],pill_h,7,bg2);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            C4 bc2=SCOL[j]; bc2.a=180; sc_(bc2);
            SDL_Rect rr2={px,y,pw[j],pill_h}; SDL_RenderDrawRect(ren,&rr2);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
            C4 pc=SCOL[j]; pc.r=SDL_min(255,(int)pc.r*2); pc.g=SDL_min(255,(int)pc.g*2); pc.b=SDL_min(255,(int)pc.b*2);
            rtxcen(f12,buf,px,y,pw[j],pill_h,pc);
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
            rtx(f12,"TOP GENRES (tracked)", lx3, y, C_TXT); y+=fh12+6;
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
                char hdr2[72];
                snprintf(hdr2,sizeof(hdr2),"\xe2\x98\x85 RATINGS   avg %.1f / 10   (%d rated)",avg_rating,n_rated);
                rtx(f12,hdr2, rx3, ry, C_TXT); ry+=fh12+6;

                int bar_w2=(rw3 - 9*3)/10; if(bar_w2<12)bar_w2=12;
                int bh_max=LST_H_ - (ry-LST_Y) - fh12 - 20;
                if(bh_max<50) bh_max=50;
                int hx2=rx3;
                for(int k=1;k<=10;k++){
                    int bh2=(int)((float)rating_hist[k]/(float)hist_max*(float)bh_max);
                    if(bh2<2&&rating_hist[k]>0) bh2=2;
                    float t2=((float)k-1.f)/9.f;
                    C4 bc2=lerpc(MK4(140,120,40,200),SCOL[S_FAV],t2);
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
                rtx(f12,"\xe2\x98\x85 RATINGS", rx3, ry, C_SUB); ry+=fh12+10;
                rtx(f12,"No games rated yet.", rx3, ry, C_DIM); ry+=fh12+4;
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
        rtx(f12,"BY DECADE", PAD, y, C_TXT); y+=fh12+6;
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

/* note text-area layout — used also in click handlers */
static void note_area(int *ax,int *ay,int *aw,int *ah){
    int ow=NOTE_OW, oh=NOTE_OH;
    int ox=(win_w-ow)/2, oy=(win_h-oh)/2;
    int fh12=TTF_FontHeight(f12), fh18=TTF_FontHeight(f18);
    int ty=oy+NOTE_PAD+fh18+4+fh12+10+1+8; /* after title+meta+sep */
    int hint_h=fh12+18;
    *ax=ox+NOTE_PAD; *ay=ty;
    *aw=ow-NOTE_PAD*2; *ah=oh-(ty-oy)-NOTE_PAD-hint_h;
}

/* map a pixel x,y inside the note area to a char offset in ns */
static int note_px_to_pos(const char *ns, int len, int ax, int tw, int ay, int mx2, int my2){
    int tp=8;
    int tx2=ax+tp;
    int cy=ay+tp;
    int fh14=TTF_FontHeight(f14);
    int best_pos=0;
    int i=0;
    while(i<=len){
        int j=i;
        while(j<len&&ns[j]!='\n'){
            char tmp[512]={0}; int tc2=j-i; if(tc2>511)tc2=511;
            memcpy(tmp,ns+i,tc2); tmp[tc2]=0;
            if(txw_(f14,tmp)>tw&&j>i) break;
            j++;
        }
        if(j<len&&ns[j]!='\n'){
            int jj=j; while(jj>i&&ns[jj]!=' ') jj--;
            if(jj>i) j=jj;
        }
        char linebuf[512]={0};
        int ll=j-i; if(ll>511)ll=511;
        memcpy(linebuf,ns+i,ll);
        /* is the click on this line? */
        if(my2>=cy&&my2<cy+fh14){
            /* find closest character */
            int best=i; int best_dist=99999;
            for(int k=0;k<=(int)strlen(linebuf);k++){
                char pre[512]={0}; memcpy(pre,linebuf,k);
                int px2=tx2+txw_(f14,pre);
                int dist=abs(mx2-px2);
                if(dist<best_dist){ best_dist=dist; best=i+k; }
            }
            return best;
        }
        best_pos=(j<len&&ns[j]=='\n')?j+1:j;
        cy+=NOTE_LH;
        if(ns[j]=='\n') j++;
        i=j;
        if(i>=len) break;
    }
    /* click below all text → end */
    if(my2>=cy) return len;
    return best_pos;
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

    /* panel */
    frr_aa(ox,oy,ow,oh,10,C_TBAR);
    { SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      C4 bc=C_ACC; bc.a=(Uint8)(200*a);
      bfrr_aa(ox,oy,ow,oh,10,1,bc,(C4){0,0,0,0});
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

    /* text area */
    int hint_h=fh12+18;
    int area_x=ox+NOTE_PAD, area_y=ty;
    int area_w=ow-NOTE_PAD*2, area_h=oh-(ty-oy)-NOTE_PAD-hint_h;
    int box_r=7, tp=8;
    int tx2=area_x+tp, tw=area_w-tp*2;

    /* rounded fill + uniform 1px border via 4 separate lines */
    frr_aa(area_x,area_y,area_w,area_h,box_r,C_ROWB);
    { SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
      C4 brd=C_ACC; brd.a=140; sc_(brd);
      SDL_RenderDrawLine(ren,area_x,area_y,area_x+area_w-1,area_y);           /* top    */
      SDL_RenderDrawLine(ren,area_x,area_y+area_h-1,area_x+area_w-1,area_y+area_h-1); /* bottom */
      SDL_RenderDrawLine(ren,area_x,area_y,area_x,area_y+area_h-1);           /* left   */
      SDL_RenderDrawLine(ren,area_x+area_w-1,area_y,area_x+area_w-1,area_y+area_h-1); /* right  */
      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE); }

    /* clip strictly inside the rounded box */
    SDL_Rect clip2={area_x+2,area_y+2,area_w-4,area_h-4};
    SDL_RenderSetClipRect(ren,&clip2);

    { char *ns=g->notes;
      int len=(int)strlen(ns);
      int cy=area_y+tp;
      Uint32 ticks=SDL_GetTicks();
      int cur_vis=((ticks/530)%2==0);
      int sel_a=note_sel0<note_cur?note_sel0:note_cur;
      int sel_b=note_sel0<note_cur?note_cur:note_sel0;
      int has_sel=(sel_a!=sel_b);

      if(len==0){
          /* empty: placeholder + single cursor at start */
          rtx(f14,"Write your notes here...",tx2,area_y+tp,C_DIM);
          if(cur_vis){
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
              sc_(C_ACC);
              SDL_RenderDrawLine(ren,tx2,area_y+tp,tx2,area_y+tp+fh14-1);
              SDL_RenderDrawLine(ren,tx2+1,area_y+tp,tx2+1,area_y+tp+fh14-1);
              SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
          }
      } else {
          /* track whether we've already drawn the cursor this frame */
          int cursor_drawn=0;
          int i=0;
          while(i<len){
              int j=i;
              while(j<len&&ns[j]!='\n'){
                  char tmp2[512]={0}; int tc2=j-i; if(tc2>511)tc2=511;
                  memcpy(tmp2,ns+i,tc2); tmp2[tc2]=0;
                  if(txw_(f14,tmp2)>tw&&j>i) break;
                  j++;
              }
              if(j<len&&ns[j]!='\n'){
                  int jj=j; while(jj>i&&ns[jj]!=' ') jj--;
                  if(jj>i) j=jj;
              }
              char linebuf[512]={0};
              int ll=j-i; if(ll>511)ll=511;
              memcpy(linebuf,ns+i,ll);
              /* line owns positions i..j (j is the newline or wrap point) */
              int line_end=j;

              if(cy+NOTE_LH>area_y&&cy<area_y+area_h){
                  /* selection highlight */
                  if(has_sel&&sel_a<=line_end&&sel_b>i){
                      int hs=sel_a>i?sel_a:i, he=sel_b<line_end?sel_b:line_end;
                      char ps[512]={0},pe[512]={0};
                      int ls2=hs-i; if(ls2>(int)strlen(linebuf))ls2=(int)strlen(linebuf);
                      int le2=he-i; if(le2>(int)strlen(linebuf))le2=(int)strlen(linebuf);
                      if(ls2<0)ls2=0; if(le2<0)le2=0;
                      memcpy(ps,linebuf,ls2); memcpy(pe,linebuf,le2);
                      int sx2=tx2+txw_(f14,ps), ex2=tx2+txw_(f14,pe);
                      if(ex2>sx2){
                          C4 sc3=C_ACC; sc3.a=80;
                          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                          SDL_Rect sr2={sx2,cy,ex2-sx2,fh14}; sc_(sc3); SDL_RenderFillRect(ren,&sr2);
                          SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                      }
                  }
                  rtx(f14,linebuf,tx2,cy,C_TXT);
                  /* cursor — strictly on the one line where note_cur lives,
                     and only if cursor at end-of-line (j) is not a newline,
                     i.e. this is the last visual segment of a paragraph */
                  int is_nl = (j<len && ns[j]=='\n');
                  int owns = (note_cur>=i && (is_nl ? note_cur<line_end : note_cur<=line_end));
                  if(cur_vis && owns && !cursor_drawn){
                      int cp=note_cur-i;
                      int lb_len=(int)strlen(linebuf);
                      if(cp>lb_len) cp=lb_len;
                      char pre[512]={0}; memcpy(pre,linebuf,cp);
                      int cxp=tx2+txw_(f14,pre);
                      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
                      sc_(C_ACC);
                      SDL_RenderDrawLine(ren,cxp,cy,cxp,cy+fh14-1);
                      SDL_RenderDrawLine(ren,cxp+1,cy,cxp+1,cy+fh14-1);
                      SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);
                      cursor_drawn=1;
                  }
              }
              cy+=NOTE_LH;
              if(ns[j]=='\n') j++;
              i=j;
              if(cy>area_y+area_h) break;
          }
          /* cursor on empty trailing line after a final '\n' */
          if(cur_vis && !cursor_drawn && note_cur==len && ns[len-1]=='\n'){
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

    /* hint — vertically centred in bottom strip */
    { int hy = oy+oh-NOTE_PAD-hint_h/2-fh12/2;
      rtxcen(f12,"[Esc] close  \xc2\xb7  Ctrl+A select all  \xc2\xb7  Ctrl+C/V copy/paste",
             ox,hy,ow,fh12,C_DIM); }
    (void)fh14;
}

/* ── List ─────────────────────────────────────────────────────── */
static void draw_list(void){
    if(cur_tab==T_STATS){ draw_stats(); return; }
    if(view_mode==VIEW_GRID){ draw_grid(); return; }
    int lh=LST_H_;
    SDL_Rect clip={0,LST_Y,win_w,lh};
    SDL_RenderSetClipRect(ren,&clip);
    { C4 c=C_BG; c.a=145; fblend(0,LST_Y,LIST_W_,lh,c); }
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
    if(i<N_TABS) tab_itx=(float)TAB_X_(i); /* don't move pill for T_STATS */
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
        db[i].rating=0; db[i].notes[0]=0;
        strlower(db[i].name, db[i].name_lc, 128);
        strlower(db[i].genre,db[i].genre_lc, 32);
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

    /* Bake all SFX tones once so clicks never stall the render thread */
    sfx_click_buf  = sfx_bake( 900.f,0.055f,0.10f,45.f);
    sfx_toggle_buf = sfx_bake( 660.f,0.075f,0.14f,35.f);
    sfx_tab_buf    = sfx_bake(1100.f,0.045f,0.08f,55.f);
    sfx_type_buf   = sfx_bake(1400.f,0.025f,0.04f,80.f);
    sfx_sort_buf   = sfx_bake(1100.f,0.045f,0.08f,55.f);

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
    bg_init();

    /* Snap pill indicators to loaded state */
    sort_ind_f=(float)sort_mode;
    sort_dd_open=0; sort_dd_anim=0.f; sort_btn_hov=0.f;
    memset(sort_item_hov,0,sizeof(sort_item_hov));
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
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);}
                    apply_rgn(win_w,win_h,!win_maximized);
                    rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_MAXIMIZED){
                    win_maximized=1; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,0);
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);} rebuild();
                }
                if(ev.window.event==SDL_WINDOWEVENT_RESTORED){
                    win_maximized=0; SDL_GetWindowSize(win,&win_w,&win_h);
                    apply_rgn(win_w,win_h,1);
                    if(cur_tab<N_TABS){tab_ix=tab_itx=(float)TAB_X_((int)cur_tab);} rebuild();
                }
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
                                note_open=-1; note_drag=0; save_defer();
                            }
                            goto done_click;
                        }

                        /* sort dropdown item click — items live BELOW the title bar */
                        if(sort_dd_open&&sort_dd_anim>0.1f){
                            for(int i=0;i<5;i++){
                                int item_y=TITLE_H+4+i*DD_ITEM_H;
                                if(mx>=TC_X0&&mx<TC_X0+DD_W&&my>=item_y&&my<item_y+DD_ITEM_H){
                                    if(sort_mode!=(SortMode)i){ sort_mode=(SortMode)i; sfx_sort(); rebuild(); save_d(); }
                                    goto done_click; /* consumed — don't fall through to rows/tabs */
                                }
                            }
                        }

                        if(my<TITLE_H){
                            if(IN_BTN(mx,TB_CX)){ sfx_click(); run=0; break; }
                            if(IN_BTN(mx,TB_MX)){ sfx_click(); toggle_maximize(); break; }
                            if(IN_BTN(mx,TB_NX)){ sfx_click(); SDL_MinimizeWindow(win); break; }
                            int hit_tb_btn=0;
                            /* sort dropdown button toggle */
                            if(my>=TC_Y&&my<TC_Y+DD_BTN_H&&mx>=TC_X0&&mx<TC_X0+DD_BTN_W){
                                hit_tb_btn=1;
                                sort_dd_open=!sort_dd_open;
                                sfx_click();
                            }
                            if(my>=TC_Y&&my<TC_Y+TC_H){
                                for(int i=0;i<2;i++){
                                    int bx=TC_X0+DD_BTN_W+20+i*(VC_W+TC_GAP);
                                    if(mx>=bx&&mx<bx+VC_W){
                                        hit_tb_btn=1;
                                        if(view_mode!=(ViewMode)i){
                                            view_mode=(ViewMode)i; sfx_tab();
                                            scr_tgt=0; scr_f=0; save_d();
                                        }
                                        break;
                                    }
                                }
                                /* stats toggle button */
                                int sbx_s=TC_X0+DD_BTN_W+20+2*(VC_W+TC_GAP)-TC_GAP+TC_GAP+4;
                                if(mx>=sbx_s&&mx<sbx_s+VC_W){
                                    hit_tb_btn=1;
                                    if(cur_tab==T_STATS){
                                        do_tab((int)prev_tab); sfx_tab();
                                    } else {
                                        prev_tab=cur_tab;
                                        do_tab((int)T_STATS); sfx_tab();
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
                        int r=row_at(mx,my);
                        if(r>=0){
                            /* ── Rating column: left-click cycles 1-10, right-click clears ── */
                            int ay_r=LST_Y+LIST_TOP_PAD+(r-page_first())*ROW_H;
                            if(mx>=RAT_X_&&mx<RAT_X_+RAT_W&&my>=ay_r&&my<ay_r+ROW_H){
                                int gi=flt[r];
                                db[gi].rating=(db[gi].rating%10)+1;
                                sfx_toggle(); save_defer(); goto done_click;
                            }
                            /* ── Status buttons ── */
                            int b=btn_at(mx,my,r);
                            if(b>=0){ int gi=flt[r]; db[gi].st[b]^=1; sfx_toggle(); btn_fl[gi][b]=1; rebuild(); save_defer(); }
                            else {
                                /* ── Note button opens notes ── */
                                int ay_r=LST_Y+LIST_TOP_PAD+(r-page_first())*ROW_H;
                                int nby2=ay_r+BTN_YO;
                                int gi=flt[r];
                                if(mx>=NOTE_BTN_X_&&mx<NOTE_BTN_X_+NOTE_BTN_SZ
                                   &&my>=nby2&&my<nby2+NOTE_BTN_SZ){
                                    note_open=gi;
                                    note_cur=(int)strlen(db[gi].notes);
                                    note_sel0=note_cur;
                                    sfx_click(); srch_blur();
                                }
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
                                            btn_fl[hov_db][j]=1; rebuild(); save_defer();
                                            break;
                                        }
                                    }
                                } else {
                                    /* note button — top-right corner of card */
                                    int nb=18, nbx2=cx2+GRID_W-nb-4, nby3=cy2+14;
                                    if(mx>=nbx2&&mx<nbx2+nb&&my>=nby3&&my<nby3+nb){
                                        note_open=hov_db;
                                        note_cur=(int)strlen(db[hov_db].notes);
                                        note_sel0=note_cur;
                                        sfx_click(); srch_blur();
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
                /* ── Right-click: clear rating ── */
                if(ev.button.button==SDL_BUTTON_RIGHT&&note_open<0&&view_mode==VIEW_LIST){
                    int mx2r=ev.button.x, my2r=ev.button.y;
                    int r2r=row_at(mx2r,my2r);
                    if(r2r>=0){
                        int ay_r2=LST_Y+LIST_TOP_PAD+(r2r-page_first())*ROW_H;
                        if(mx2r>=RAT_X_&&mx2r<RAT_X_+RAT_W&&my2r>=ay_r2&&my2r<ay_r2+ROW_H){
                            db[flt[r2r]].rating=0;
                            sfx_toggle(); save_defer();
                        }
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
                    if(sym==SDLK_ESCAPE){ sfx_click(); note_open=-1; save_defer(); break; }
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
                    } else if(sym==SDLK_DELETE){
                        if(has_sel){ NOTE_DEL_SEL(); }
                        else if(note_cur<len){
                            memmove(ns+note_cur,ns+note_cur+1,len-note_cur);
                            save_defer();
                        }
                    } else if(sym==SDLK_LEFT){
                        if(!shift&&has_sel){
                            int a=note_sel0<note_cur?note_sel0:note_cur;
                            note_cur=note_sel0=a;
                        } else {
                            int np=ctrl?(note_cur>0?(note_cur-1):0):note_cur>0?note_cur-1:0;
                            if(ctrl){ while(np>0&&ns[np-1]!=' '&&ns[np-1]!='\n') np--; }
                            note_cur=np; if(!shift) note_sel0=np;
                        }
                    } else if(sym==SDLK_RIGHT){
                        if(!shift&&has_sel){
                            int b=note_sel0>note_cur?note_sel0:note_cur;
                            note_cur=note_sel0=b;
                        } else {
                            int np=note_cur<len?note_cur+1:len;
                            if(ctrl){ np=note_cur; while(np<len&&ns[np]!=' '&&ns[np]!='\n') np++; while(np<len&&ns[np]==' ') np++; }
                            note_cur=np; if(!shift) note_sel0=np;
                        }
                    } else if(sym==SDLK_HOME){
                        /* go to start of line */
                        int np=note_cur;
                        while(np>0&&ns[np-1]!='\n') np--;
                        note_cur=np; if(!shift) note_sel0=np;
                    } else if(sym==SDLK_END){
                        int np=note_cur;
                        while(np<len&&ns[np]!='\n') np++;
                        note_cur=np; if(!shift) note_sel0=np;
                    } else if(sym==SDLK_RETURN||sym==SDLK_KP_ENTER){
                        if(has_sel) NOTE_DEL_SEL();
                        len=(int)strlen(ns);
                        if(len<510){
                            memmove(ns+note_cur+1,ns+note_cur,len-note_cur+1);
                            ns[note_cur]='\n'; note_cur++; note_sel0=note_cur; save_defer();
                        }
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
                    if(sym==SDLK_ESCAPE){ if(*srch||cur_tab!=T_ALL) sfx_click(); srch[0]=0; do_tab(0); }
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
                } else if(s_on){
                    srch_insert(ev.text.text);
                    sfx_type(); scr_tgt=0; scr_f=0; rebuild();
                }
                break;
            }
        }

        if(needs_redraw || rz_drag || win_drag || drag_sb || srch_drag || note_open>=0 || note_anim>0.005f){
            draw_background();
            draw_titlebar();
            draw_hdr();
            draw_tabs();
            draw_list();
            draw_sbar();
            draw_sort_dropdown();  /* drawn last — always on top of tabs/content */
            draw_note_overlay();   /* overlay — above everything */
            SDL_RenderPresent(ren);
        }
    }

    save_flush(); /* write any pending deferred save */
    save_d();
    tc_free_all();
    if(aud_dev) SDL_CloseAudioDevice(aud_dev);
    free(sfx_click_buf.buf); free(sfx_toggle_buf.buf); free(sfx_tab_buf.buf);
    free(sfx_type_buf.buf);  free(sfx_sort_buf.buf);
    TTF_CloseFont(f22); TTF_CloseFont(f18);
    TTF_CloseFont(f14); TTF_CloseFont(f12);
    TTF_Quit();
    SDL_FreeCursor(cur_arr); SDL_FreeCursor(cur_ns); SDL_FreeCursor(cur_ew);
    SDL_FreeCursor(cur_nwse); SDL_FreeCursor(cur_nesw);
    bg_free();
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}