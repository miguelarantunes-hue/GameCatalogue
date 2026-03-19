/* ══════════════════════════════════════════════════════════════════
 *  save.h  —  Save / load public API
 * ══════════════════════════════════════════════════════════════════ */
#ifndef SAVE_H
#define SAVE_H

#include "state.h"

/* Build the full save-file path once at startup */
void init_save_path(void);

/* Write the catalogue to disk immediately */
void save_d(void);

/* Load the catalogue from disk (called once at startup) */
void load_d(void);

/* Mark the save as dirty; actual write happens after SAVE_DEFER_MS */
void save_defer(void);

/* Flush a pending deferred save right now (call each frame) */
void save_flush(void);

#endif /* SAVE_H */
