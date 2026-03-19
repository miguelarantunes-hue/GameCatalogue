/* ══════════════════════════════════════════════════════════════════
 *  search.h  —  Filter engine and genre helpers public API
 * ══════════════════════════════════════════════════════════════════ */
#ifndef SEARCH_H
#define SEARCH_H

#include "state.h"

/* ── String utility ───────────────────────────────────────────── */
/* Write lowercase copy of src into dst (max bytes, NUL-terminated) */
void strlower(const char *src, char *dst, int max);

/* ── Genre helpers ────────────────────────────────────────────── */
/* Build the sorted unique genre list from the full database */
void build_genre_list(void);

/* Toggle a genre in/out of the active filter set */
void toggle_genre_filter(const char *gen);

/* Returns 1 if the genre is currently in the active filter set */
int  genre_is_active(const char *gen);

/* Update chip_band based on current active filters */
void update_chip_band(void);

/* ── Main filter / sort pass ──────────────────────────────────── */
/* Rebuild flt[] / nflt from the current tab, search query,
   genre filters and sort mode.  Resets scroll to top.           */
void rebuild(void);

#endif /* SEARCH_H */