/* ══════════════════════════════════════════════════════════════════
 *  state.h  —  Shared types, constants and extern global declarations
 *
 *  Include this header in any module that needs to access the game
 *  database, filter state, search state, or save/audio state.
 *  All definitions live in main.c.
 * ══════════════════════════════════════════════════════════════════ */
#ifndef STATE_H
#define STATE_H

#include <SDL2/SDL.h>
#include "themes.h"   /* for C4, N_THEMES */
#include "games.h"    /* for GE, GDB, N_GDB */

/* ── Status constants ─────────────────────────────────────────── */
#define N_STATUS   7
#define S_WISH     0
#define S_PLAYED   1
#define S_PLAYING  2
#define S_FINISHED 3
#define S_DROPPED  4
#define S_FAV      5
#define S_ROTATION 6

/* ── Layout constants (shared with search.c) ─────────────────── */
#define CHIP_H       26   /* filter-chip row height                  */

/* ── Database constants ───────────────────────────────────────── */
#define MAX_G        2500
#define SAVEFILE_NAME "catalogue_save.bin"
#define SAVE_VER     0xCA7A2029u
#define SAVE_DEFER_MS 500

/* ── Audio constant ───────────────────────────────────────────── */
#define SFX_RATE 44100

/* ── Core game record ─────────────────────────────────────────── */
typedef struct {
    char name[128];
    char genre[32];
    char genre2[32];    /* secondary genre (empty = none)          */
    int  year;
    char st[N_STATUS];
    char name_lc[128];  /* pre-computed lowercase for fast search  */
    char genre_lc[32];  /* pre-computed lowercase for fast search  */
    char genre2_lc[32]; /* secondary genre lowercase               */
    int  rating;        /* 0 = unrated, 1–10                       */
    char notes[512];    /* personal notes / review                 */
} Game;

/* ── Tab / view / sort enumerations ──────────────────────────── */
typedef enum {
    T_ALL=0, T_WISH, T_PLAYED, T_PLAYING,
    T_FINISHED, T_DROPPED, T_FAV, T_ROTATION, T_STATS
} TabId;

typedef enum { SORT_AZ=0, SORT_ZA, SORT_NEW, SORT_OLD, SORT_RATING } SortMode;
typedef enum { VIEW_LIST=0, VIEW_GRID } ViewMode;

/* ── Game database — defined in main.c ───────────────────────── */
extern Game     db[MAX_G];
extern int      ndb;

/* ── Filter / view state — defined in main.c ─────────────────── */
extern int      flt[MAX_G];
extern int      nflt;
extern int      flt_score[MAX_G];
extern TabId    cur_tab;
extern TabId    prev_tab;
extern SortMode sort_mode;
extern ViewMode view_mode;

/* ── Search / filter state — defined in main.c ───────────────── */
extern char     srch[128];
extern char     srch_lc[128];
extern int      s_on;
extern int      srch_cur;
extern int      srch_sel0;
extern float    srch_blink;
extern int      srch_scroll;
extern int      srch_drag;
extern int      n_filt_genres;
extern char     filt_genres[8][32];
extern int      filt_year;
extern int      chip_band;
extern float    scr_f;
extern float    scr_tgt;
extern int      cur_page;

/* ── Genre list — defined in main.c ──────────────────────────── */
extern char     genre_list[64][32];
extern int      genre_counts[64];
extern int      n_genres;

/* ── Save state — defined in save.c ──────────────────────────── */
extern char     save_path[512];
extern Uint32   save_pending_at;

/* ── Audio device — defined in audio.c ───────────────────────── */
extern SDL_AudioDeviceID aud_dev;

/* ── Pagination helpers — defined in main.c ──────────────────── */
extern void clamp_page(void);

#endif /* STATE_H */