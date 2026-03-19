/* ══════════════════════════════════════════════════════════════════
 *  audio.h  —  Sound-effect public API
 * ══════════════════════════════════════════════════════════════════ */
#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include "state.h"

/* ── Pre-baked tone buffer ────────────────────────────────────── */
typedef struct { Sint16 *buf; int n; } SfxBuf;

/* ── Init: call once after SDL_OpenAudioDevice ────────────────── */
void sfx_init(void);

/* ── Free PCM buffers: call once on shutdown ─────────────────── */
void sfx_free(void);

/* ── Per-category play functions (debounced) ─────────────────── */
void sfx_click (void);
void sfx_toggle(void);
void sfx_tab   (void);
void sfx_type  (void);
void sfx_sort  (void);

#endif /* AUDIO_H */