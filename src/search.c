/* ══════════════════════════════════════════════════════════════════
 *  search.c  —  Filter engine, search scoring, and genre helpers
 *
 *  Pure logic — no SDL_Renderer, no font access.
 *  All state is accessed through extern globals declared in state.h.
 * ══════════════════════════════════════════════════════════════════ */
#include "search.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/* ── String utility ───────────────────────────────────────────── */
void strlower(const char *src, char *dst, int max){
    int i = 0;
    for(; src[i] && i < max-1; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = 0;
}

/* ── Genre helpers ────────────────────────────────────────────── */
static int game_has_genre(const Game *g, const char *gen){
    return strcasecmp(g->genre, gen)==0 ||
           (g->genre2[0] && strcasecmp(g->genre2, gen)==0);
}

int genre_is_active(const char *gen){
    for(int i = 0; i < n_filt_genres; i++)
        if(strcasecmp(filt_genres[i], gen)==0) return 1;
    return 0;
}

void toggle_genre_filter(const char *gen){
    for(int i = 0; i < n_filt_genres; i++){
        if(strcasecmp(filt_genres[i], gen)==0){
            /* already active — remove it */
            for(int j = i; j < n_filt_genres-1; j++)
                strncpy(filt_genres[j], filt_genres[j+1], 31);
            n_filt_genres--;
            return;
        }
    }
    if(n_filt_genres < 8){
        strncpy(filt_genres[n_filt_genres], gen, 31);
        filt_genres[n_filt_genres][31] = 0;
        n_filt_genres++;
    }
}

void update_chip_band(void){
    chip_band = (n_filt_genres > 0 || filt_year) ? 26 /* CHIP_H */ : 0;
}

/* ── Genre list builder ───────────────────────────────────────── */
static int cmp_str(const void *a, const void *b){
    return strcasecmp((const char*)a, (const char*)b);
}

void build_genre_list(void){
    char tmp[64][32]; int n = 0;
    for(int i = 0; i < ndb; i++){
        for(int pass = 0; pass < 2; pass++){
            const char *gn = (pass==0) ? db[i].genre : db[i].genre2;
            if(!gn[0]) continue;
            int found = 0;
            for(int j = 0; j < n; j++) if(strcasecmp(tmp[j], gn)==0){ found=1; break; }
            if(!found && n < 64){ strncpy(tmp[n], gn, 31); tmp[n][31]=0; n++; }
        }
    }
    qsort(tmp, n, 32, cmp_str);
    n_genres = n;
    for(int j = 0; j < n; j++){
        strncpy(genre_list[j], tmp[j], 31); genre_list[j][31]=0;
        genre_counts[j] = 0;
        for(int i = 0; i < ndb; i++)
            if(strcasecmp(db[i].genre, genre_list[j])==0 ||
               (db[i].genre2[0] && strcasecmp(db[i].genre2, genre_list[j])==0))
                genre_counts[j]++;
    }
}

/* ── Intelligent search scoring ──────────────────────────────────
   Returns 0 = no match, higher = better match.
   Layers (cumulative):
     1000  exact substring in name
      800  all query tokens found as substrings (any order)
      600  acronym match (query == first letters of each word)
      300  fuzzy subsequence (query chars appear in order in name)
      +bonus for early position, token prefix hits, genre match
   ──────────────────────────────────────────────────────────────── */
static int search_score(const char *name_l, const char *genre_l,
                        const char *q, int qlen){
    if(qlen == 0) return 1;
    int score = 0;

    /* 1. Exact substring in name */
    const char *hit = strstr(name_l, q);
    if(hit){
        score += 1000;
        if(hit==name_l || (hit>name_l && *(hit-1)==' ')) score += 200;
        score += (int)(100 - (hit - name_l)*2);
        return score;
    }

    /* 2. Exact substring in genre */
    if(strstr(genre_l, q)) score += 400;

    /* 3. All tokens found as substrings */
    {
        char qcopy[128]; strncpy(qcopy, q, 127); qcopy[127]=0;
        int all_found=1, tok_score=0;
        char *tok = strtok(qcopy, " ");
        while(tok){
            const char *th = strstr(name_l, tok);
            if(!th){ all_found=0; break; }
            if(th==name_l || (th>name_l && *(th-1)==' ')) tok_score += 50;
            else tok_score += 20;
            tok = strtok(NULL, " ");
        }
        if(all_found && tok_score > 0){ score += 800+tok_score; return score; }
    }

    /* 4. Acronym: query matches first letter of each word */
    {
        char initials[64]; int ic = 0;
        initials[ic++] = (char)name_l[0];
        for(int i=1; name_l[i] && ic<63; i++)
            if(name_l[i-1]==' ' || name_l[i-1]==':' || name_l[i-1]=='-')
                initials[ic++] = name_l[i];
        initials[ic] = 0;
        if(ic >= qlen && strstr(initials, q)) score += 600;
    }

    /* 5. Fuzzy subsequence: all query chars appear in order */
    if(score == 0){
        int qi = 0;
        for(int ni=0; name_l[ni] && qi<qlen; ni++)
            if(name_l[ni] == q[qi]) qi++;
        if(qi == qlen){
            int compactness = (int)(strlen(name_l)) - qlen;
            score += 300 + (100-compactness < 0 ? 0 : 100-compactness);
        }
    }

    /* 6. Partial token prefix */
    if(score == 0){
        char qcopy[128]; strncpy(qcopy, q, 127); qcopy[127]=0;
        char *tok = strtok(qcopy, " ");
        while(tok){
            int tl = (int)strlen(tok);
            if(strncmp(name_l, tok, tl)==0){ score += 150; break; }
            for(int i=1; name_l[i]; i++)
                if(name_l[i-1]==' ' && strncmp(name_l+i, tok, tl)==0){ score += 150; break; }
            tok = strtok(NULL, " ");
        }
    }

    return score;
}

/* ── qsort comparators ────────────────────────────────────────── */
static int cmp_flt_az    (const void *a, const void *b){ return strcasecmp(db[*(int*)a].name, db[*(int*)b].name); }
static int cmp_flt_za    (const void *a, const void *b){ return strcasecmp(db[*(int*)b].name, db[*(int*)a].name); }
static int cmp_flt_new   (const void *a, const void *b){ return db[*(int*)b].year - db[*(int*)a].year; }
static int cmp_flt_old   (const void *a, const void *b){ return db[*(int*)a].year - db[*(int*)b].year; }
static int cmp_flt_rating(const void *a, const void *b){
    int ra=db[*(int*)a].rating, rb=db[*(int*)b].rating;
    int sa=ra?ra:-1,            sb=rb?rb:-1;
    if(sb != sa) return sb - sa;
    return strcasecmp(db[*(int*)a].name, db[*(int*)b].name);
}

/* ── Main filter / sort pass ──────────────────────────────────── */
void rebuild(void){
    if(cur_tab == T_STATS){ nflt=0; return; }
    nflt = 0;
    char ql[128]; int qlen = (int)strlen(srch);
    strlower(srch, ql, 128);
    strncpy(srch_lc, ql, 127); srch_lc[127]=0;

    for(int i=0; i < ndb; i++){
        if(cur_tab != T_ALL && !db[i].st[(int)cur_tab-1]) continue;
        if(n_filt_genres > 0){
            int match=0;
            for(int fi=0; fi<n_filt_genres && !match; fi++)
                if(game_has_genre(&db[i], filt_genres[fi])) match=1;
            if(!match) continue;
        }
        if(filt_year && db[i].year != filt_year) continue;
        if(qlen > 0){
            char combined_genre[66];
            if(db[i].genre2[0])
                snprintf(combined_genre, sizeof(combined_genre), "%s %s",
                         db[i].genre_lc, db[i].genre2_lc);
            else
                strncpy(combined_genre, db[i].genre_lc, 65);
            int s = search_score(db[i].name_lc, combined_genre, ql, qlen);
            if(s == 0) continue;
            flt_score[nflt] = s;
        } else {
            flt_score[nflt] = 0;
        }
        flt[nflt++] = i;
    }

    if(qlen > 0){
        /* insertion sort — stable, fast for small N after filter */
        for(int a=1; a < nflt; a++){
            int ki=flt[a], ks=flt_score[a], b=a-1;
            while(b>=0 && flt_score[b]<ks){
                flt[b+1]=flt[b]; flt_score[b+1]=flt_score[b]; b--;
            }
            flt[b+1]=ki; flt_score[b+1]=ks;
        }
    } else {
        switch(sort_mode){
        case SORT_AZ:     qsort(flt, nflt, sizeof(int), cmp_flt_az);     break;
        case SORT_ZA:     qsort(flt, nflt, sizeof(int), cmp_flt_za);     break;
        case SORT_NEW:    qsort(flt, nflt, sizeof(int), cmp_flt_new);    break;
        case SORT_OLD:    qsort(flt, nflt, sizeof(int), cmp_flt_old);    break;
        case SORT_RATING: qsort(flt, nflt, sizeof(int), cmp_flt_rating); break;
        }
    }
    scr_f=0.f; scr_tgt=0.f;
    clamp_page();
}