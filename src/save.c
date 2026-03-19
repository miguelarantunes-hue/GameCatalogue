/* ══════════════════════════════════════════════════════════════════
 *  save.c  —  Catalogue persistence (save / load / deferred flush)
 * ══════════════════════════════════════════════════════════════════ */
#include "save.h"
#include "themes.h"   /* set_theme_instant */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#endif

/* ── Global save state (declared extern in state.h) ────────────── */
char   save_path[512]   = {0};
Uint32 save_pending_at  = 0;

/* ── Path initialisation ──────────────────────────────────────── */
void init_save_path(void){
    const char *appdata = getenv("APPDATA");
    if(!appdata) appdata = ".";
    snprintf(save_path, sizeof(save_path), "%s\\GameCatalogue", appdata);
#ifdef _WIN32
    CreateDirectoryA(save_path, NULL);
#else
    { char cmd[520]; snprintf(cmd,sizeof(cmd),"mkdir -p \"%s\"",save_path); system(cmd); }
#endif
    snprintf(save_path + strlen(save_path),
             sizeof(save_path) - strlen(save_path),
             "\\%s", SAVEFILE_NAME);
}

/* ── Write ────────────────────────────────────────────────────── */
void save_d(void){
    FILE *fp = fopen(save_path, "wb");
    if(!fp) return;
    Uint32 v = SAVE_VER;
    fwrite(&v,   4,           1, fp);
    fwrite(&ndb, sizeof(int), 1, fp);
    for(int i = 0; i < ndb; i++) fwrite(db[i].st, 1, N_STATUS, fp);
    fwrite(&cur_theme,  sizeof(int), 1, fp);
    fwrite(&sort_mode,  sizeof(int), 1, fp);
    fwrite(&view_mode,  sizeof(int), 1, fp);
    /* v2: rating and notes */
    for(int i = 0; i < ndb; i++) fwrite(&db[i].rating, sizeof(int), 1, fp);
    for(int i = 0; i < ndb; i++) fwrite(db[i].notes,   512,         1, fp);
    fclose(fp);
}

/* ── Read ─────────────────────────────────────────────────────── */
void load_d(void){
    FILE *fp = fopen(save_path, "rb");
    if(!fp){ set_theme_instant(0); return; }
    Uint32 v = 0;
    fread(&v, 4, 1, fp);
    if(v != SAVE_VER){ fclose(fp); set_theme_instant(0); return; }
    int n;
    fread(&n, sizeof(int), 1, fp);
    int lim = n < ndb ? n : ndb;
    for(int i = 0; i < lim; i++) fread(db[i].st, 1, N_STATUS, fp);
    int t = 0;
    if(fread(&t, sizeof(int), 1, fp) == 1 && t >= 0 && t < N_THEMES)
        set_theme_instant(t);
    else
        set_theme_instant(0);
    { int sm = 0; if(fread(&sm, sizeof(int), 1, fp)==1 && sm>=0 && sm<5) sort_mode=(SortMode)sm; }
    { int vm = 0; if(fread(&vm, sizeof(int), 1, fp)==1 && vm>=0 && vm<2) view_mode=(ViewMode)vm; }
    /* v2: rating and notes — guarded so old saves load cleanly */
    for(int i = 0; i < lim; i++){
        int r = 0;
        if(fread(&r, sizeof(int), 1, fp)==1 && r>=0 && r<=10) db[i].rating = r;
    }
    for(int i = 0; i < lim; i++) fread(db[i].notes, 512, 1, fp);
    fclose(fp);
}

/* ── Deferred write helpers ───────────────────────────────────── */
void save_defer(void){ save_pending_at = SDL_GetTicks(); }
void save_flush(void){ if(save_pending_at){ save_d(); save_pending_at = 0; } }
