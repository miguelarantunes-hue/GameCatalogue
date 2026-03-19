/* ══════════════════════════════════════════════════════════════════
 *  audio.c  —  Procedural sound effects
 *
 *  All tones are synthesised once at startup (sfx_init) then queued
 *  to the SDL audio device with per-category cooldown guards so rapid
 *  clicks never stall the render thread.
 * ══════════════════════════════════════════════════════════════════ */
#include "audio.h"
#include <math.h>
#include <stdlib.h>

/* ── Audio device (declared extern in state.h) ───────────────── */
SDL_AudioDeviceID aud_dev = 0;

/* ── Pre-baked tone buffers ───────────────────────────────────── */
static SfxBuf sfx_click_buf;
static SfxBuf sfx_toggle_buf;
static SfxBuf sfx_tab_buf;
static SfxBuf sfx_type_buf;
static SfxBuf sfx_sort_buf;

/* ── Tone synthesis ───────────────────────────────────────────── */
static SfxBuf sfx_bake(float freq, float dur, float vol, float decay){
    SfxBuf b;
    b.n   = (int)(SFX_RATE * dur);
    b.buf = (Sint16*)malloc(b.n * sizeof(Sint16));
    if(!b.buf){ b.n = 0; return b; }
    for(int i = 0; i < b.n; i++){
        float t = (float)i / SFX_RATE;
        float s = sinf(2.f*(float)M_PI*freq*t) * expf(-t*decay) * vol * 32767.f;
        if(s >  32767.f) s =  32767.f;
        if(s < -32767.f) s = -32767.f;
        b.buf[i] = (Sint16)s;
    }
    return b;
}

/* ── Low-level queue ──────────────────────────────────────────── */
static void sfx_queue(SfxBuf b){
    if(!aud_dev || !b.buf) return;
    SDL_QueueAudio(aud_dev, b.buf, b.n * sizeof(Sint16));
}

/* ── Init — call once after aud_dev is open ───────────────────── */
void sfx_init(void){
    sfx_click_buf  = sfx_bake( 900.f, 0.055f, 0.10f, 45.f);
    sfx_toggle_buf = sfx_bake( 660.f, 0.075f, 0.14f, 35.f);
    sfx_tab_buf    = sfx_bake(1100.f, 0.045f, 0.08f, 55.f);
    sfx_type_buf   = sfx_bake(1400.f, 0.025f, 0.04f, 80.f);
    sfx_sort_buf   = sfx_bake(1100.f, 0.045f, 0.08f, 55.f);
}

/* ── Free — call once on shutdown ────────────────────────────── */
void sfx_free(void){
    free(sfx_click_buf.buf);
    free(sfx_toggle_buf.buf);
    free(sfx_tab_buf.buf);
    free(sfx_type_buf.buf);
    free(sfx_sort_buf.buf);
}

/* ── Debounced play functions ─────────────────────────────────── */
static Uint32 sfx_last_click  = 0;
static Uint32 sfx_last_toggle = 0;
static Uint32 sfx_last_tab    = 0;
static Uint32 sfx_last_type   = 0;
static Uint32 sfx_last_sort   = 0;

void sfx_click (void){ Uint32 n=SDL_GetTicks(); if(n-sfx_last_click < 60)  return; sfx_last_click =n; sfx_queue(sfx_click_buf);  }
void sfx_toggle(void){ Uint32 n=SDL_GetTicks(); if(n-sfx_last_toggle< 30)  return; sfx_last_toggle=n; sfx_queue(sfx_toggle_buf); }
void sfx_tab   (void){ Uint32 n=SDL_GetTicks(); if(n-sfx_last_tab   < 50)  return; sfx_last_tab   =n; sfx_queue(sfx_tab_buf);    }
void sfx_type  (void){ Uint32 n=SDL_GetTicks(); if(n-sfx_last_type  < 30)  return; sfx_last_type  =n; sfx_queue(sfx_type_buf);   }
void sfx_sort  (void){ Uint32 n=SDL_GetTicks(); if(n-sfx_last_sort  <200)  return; sfx_last_sort  =n; sfx_queue(sfx_sort_buf);   }