/* ══════════════════════════════════════════════════════════════════
 *  themes.c  —  Theme data, colour transitions, and background rendering
 * ══════════════════════════════════════════════════════════════════ */
#include "themes.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── External window/renderer context provided by main.c ─────────── */
extern SDL_Renderer *ren;
extern int win_w, win_h;

/* ── Renderer primitives provided by main.c ─────────────────────── */
extern void sc_   (C4 c);
extern void fblend(int x, int y, int w, int h, C4 c);

/* ── Internal transition state ──────────────────────────────────── */
static C4    tc_from[N_CSLOTS];
static C4    tc_to  [N_CSLOTS];
float        tc_t = 1.f;   /* exposed so main.c/anim_tick can check progress */
float        galaxy_dot_f = 0.f; /* 0..1 — animates in when Galaxy is selected */


const Theme THEMES[N_THEMES] = {
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
    {"Galaxy",
     {3,2,12,255},{5,3,18,255},{2,1,8,255},{8,5,24,255},
     {10,6,28,255},{8,5,22,255},{20,12,52,255},
     {180,80,255,255},{235,220,255,255},{120,90,170,255},{55,38,88,255},
     {4,2,14,255},{60,20,120,255},{100,40,190,255},
     {2,1,10,255},{10,5,30,255},{14,8,40,255},{12,6,34,255},{8,4,26,255},{220,58,58,255}},
};

int   cur_theme = 0;
float tdot_hov   [N_THEMES];
float tdot_bounce[N_THEMES];
float sel_ring_if = 0.f;
float theme_pulse = 0.f;
int   dots_in_tb  = 0;
int   dots_in_tb_prev = -1;
int   dots_x0     = 0;
int   dots_cy     = 0;
int   sr_x        = 380;
int   sr_w        = 380;


C4 C_BG,C_HDR,C_TITLE,C_TBAR,C_ROWA,C_ROWB,C_ROWH,
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
void tick_theme(float dt){
    if(tc_t>=1.f) return;
    tc_t+=dt*TC_SPEED; if(tc_t>1.f) tc_t=1.f;
    C4 cur[N_CSLOTS];
    for(int i=0;i<N_CSLOTS;i++) cur[i]=lerpc(tc_from[i],tc_to[i],tc_t);
    theme_apply_slots(cur);
}
void set_theme(int idx){
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
void set_theme_instant(int idx){
    cur_theme=idx;
    theme_pack(&THEMES[idx],tc_to);
    for(int i=0;i<N_CSLOTS;i++) tc_from[i]=tc_to[i];
    tc_t=1.f;
    theme_apply_slots(tc_to);
    sel_ring_if=(float)idx;
}

void compute_dot_layout(void){
    /* Galaxy dot (last, index 8) has gr=TDOT_R+3=12 and spikes reach gr+4=16px right.
       right_pad must be >= 17 to stay on screen — use 22 for a clean margin. */
    int right_pad  = 22;
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
        /* Galaxy titlebar spike reach = gr+3 = 13 — pull right_edge in by 13 extra */
        int right_edge = win_w - 3*TB_BTN_W - 33;
        dots_x0    = right_edge - (N_THEMES-1)*TDOT_STEP;
        dots_cy    = TITLE_H/2;
        sr_w       = 380;
        sr_x       = win_w - right_pad - 10 - sr_w;
    }
}

int hit_theme_dot(int mx,int my){
    for(int i=0;i<N_THEMES;i++){
        int cx=dots_x0+i*TDOT_STEP,dx=mx-cx,dy=my-dots_cy;
        if(dx*dx+dy*dy<=(TDOT_R+4)*(TDOT_R+4)) return i;
    }
    return -1;
}
/* ── Background effect textures ──────────────────────────────── */
static SDL_Texture *grain_tex  = NULL;   /* static film-grain overlay  */
static SDL_Texture *blob_tex   = NULL;   /* soft radial gradient blob  */
static SDL_Texture *galaxy_tex = NULL;   /* galaxy: static star field  */

void bg_init(void){
    /* ── Grain: random noise tiled at low opacity ── */
    grain_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STATIC, GRAIN_SZ, GRAIN_SZ);
    if(grain_tex){
        Uint32 *px = (Uint32*)malloc(GRAIN_SZ*GRAIN_SZ*sizeof(Uint32));
        if(px){
            Uint32 rng = 0xDEADBEEF;
            for(int i=0;i<GRAIN_SZ*GRAIN_SZ;i++){
                rng = rng*1664525u + 1013904223u;
                Uint8 v = (rng>>24) & 0xFF;
                Uint8 a = (Uint8)(v * 0.09f);
                px[i] = ((Uint32)255<<24)|((Uint32)255<<16)|((Uint32)255<<8)|a;
            }
            SDL_UpdateTexture(grain_tex, NULL, px, GRAIN_SZ*sizeof(Uint32));
            free(px);
        }
        SDL_SetTextureBlendMode(grain_tex, SDL_BLENDMODE_BLEND);
    }

    /* ── Blob: normalised white radial gradient, scaled at draw time ── */
    blob_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_STATIC, BLOB_TEX_SZ, BLOB_TEX_SZ);
    if(blob_tex){
        Uint32 *px = (Uint32*)malloc(BLOB_TEX_SZ*BLOB_TEX_SZ*sizeof(Uint32));
        if(px){
            float rf = (float)BLOB_TEX_R;
            for(int y=0;y<BLOB_TEX_SZ;y++){
                for(int x=0;x<BLOB_TEX_SZ;x++){
                    float dx=(float)(x-BLOB_TEX_R), dy=(float)(y-BLOB_TEX_R);
                    float d = sqrtf(dx*dx+dy*dy)/rf;
                    if(d>=1.f){px[y*BLOB_TEX_SZ+x]=0;continue;}
                    /* soft Gaussian falloff */
                    float g = expf(-2.8f*d*d);
                    Uint8 a = (Uint8)(g*255.f);
                    px[y*BLOB_TEX_SZ+x] = 0xFFFFFF00|a;
                }
            }
            SDL_UpdateTexture(blob_tex, NULL, px, BLOB_TEX_SZ*sizeof(Uint32));
            free(px);
        }
        SDL_SetTextureBlendMode(blob_tex, SDL_BLENDMODE_BLEND);
    }

    /* ── Galaxy star field: 1024×1024 transparent texture with ~900 stars ── */
    {
        int GSZ = 1024;
        galaxy_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STATIC, GSZ, GSZ);
        if(galaxy_tex){
            Uint32 *px = (Uint32*)calloc(GSZ*GSZ, sizeof(Uint32));
            if(px){
                Uint32 rng = 0xBADC0DE7;
                #define GRNG(r) ((r)=((r)*1664525u+1013904223u), (r)>>16)
                /* tiny background haze */
                for(int i=0;i<GSZ*GSZ;i++){
                    rng=rng*1664525u+1013904223u;
                    Uint8 v=(rng>>24)&0xFF;
                    if(v>245){
                        Uint8 br=(Uint8)(((rng>>16)&0xFF)*0.18f+20);
                        /* slight warm/cool tint variation */
                        Uint8 r2=br, g2=(Uint8)(br*0.85f), b2=(Uint8)(br*1.15f<255?br*1.15f:255);
                        px[i]=(255u<<24)|(r2<<16)|(g2<<8)|b2;
                    }
                }
                /* medium stars */
                for(int s=0;s<600;s++){
                    int sx=(int)(GRNG(rng)%GSZ);
                    int sy=(int)(GRNG(rng)%GSZ);
                    Uint8 br=(Uint8)(60+GRNG(rng)%180);
                    Uint8 r2=br,g2=(Uint8)(br*0.80f),b2=(Uint8)(br*1.20f<255?br*1.20f:255);
                    /* chance of warm star */
                    if(GRNG(rng)%10==0){ r2=(Uint8)(br*1.1f<255?br*1.1f:255); b2=(Uint8)(br*0.7f); g2=(Uint8)(br*0.85f); }
                    px[sy*GSZ+sx]=(255u<<24)|(r2<<16)|(g2<<8)|b2;
                }
                /* large bright stars with 3x3 soft glow */
                for(int s=0;s<120;s++){
                    int sx=2+(int)(GRNG(rng)%(GSZ-4));
                    int sy=2+(int)(GRNG(rng)%(GSZ-4));
                    Uint8 br=(Uint8)(180+GRNG(rng)%76);
                    Uint8 r2=br,g2=(Uint8)(br*0.88f),b2=255;
                    /* bright center */
                    px[sy*GSZ+sx]=(255u<<24)|(r2<<16)|(g2<<8)|b2;
                    /* soft cross diffraction spike */
                    Uint8 dim=(Uint8)(br*0.4f);
                    C4 dc={r2,g2,b2,dim};
                    for(int d=1;d<=2;d++){
                        Uint8 da=(Uint8)(br*0.4f/(float)d);
                        Uint32 dv=(da<<24)|(r2<<16)|(g2<<8)|b2;
                        if(sx+d<GSZ) px[sy*GSZ+sx+d]=dv;
                        if(sx-d>=0)  px[sy*GSZ+sx-d]=dv;
                        if(sy+d<GSZ) px[(sy+d)*GSZ+sx]=dv;
                        if(sy-d>=0)  px[(sy-d)*GSZ+sx]=dv;
                    }
                    (void)dc;
                }
                #undef GRNG
                SDL_UpdateTexture(galaxy_tex, NULL, px, GSZ*sizeof(Uint32));
                free(px);
            }
            SDL_SetTextureBlendMode(galaxy_tex, SDL_BLENDMODE_BLEND);
        }
    }
} /* end bg_init */
void bg_free(void){
    if(grain_tex)  { SDL_DestroyTexture(grain_tex);  grain_tex=NULL;  }
    if(blob_tex)   { SDL_DestroyTexture(blob_tex);   blob_tex=NULL;   }
    if(galaxy_tex) { SDL_DestroyTexture(galaxy_tex); galaxy_tex=NULL; }
}


/* Draw blob centred at (cx,cy), radius r px, scaled to window */
static void draw_blob(int cx, int cy, int r, float opacity){
    if(!blob_tex) return;
    C4 ac = C_ACC;
    SDL_SetTextureColorMod(blob_tex, ac.r, ac.g, ac.b);
    SDL_SetTextureAlphaMod(blob_tex, (Uint8)(opacity*255.f));
    SDL_Rect dst = { cx-r, cy-r, r*2, r*2 };
    SDL_RenderCopy(ren, blob_tex, NULL, &dst);
}

void draw_background(void){
    sc_(C_BG); SDL_RenderClear(ren);

    if(cur_theme == 8){ /* Galaxy — special starfield + nebula */
        /* 1. Star field — tile the 1024 texture to cover the window */
        if(galaxy_tex){
            int GSZ=1024;
            SDL_SetTextureAlphaMod(galaxy_tex, 255);
            for(int ty=0;ty<win_h;ty+=GSZ)
                for(int tx=0;tx<win_w;tx+=GSZ){
                    SDL_Rect dst={tx,ty,GSZ,GSZ};
                    SDL_RenderCopy(ren, galaxy_tex, NULL, &dst);
                }
        }
        /* 2. Nebula blobs in distinct galaxy colors */
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        if(blob_tex){
            int base=(win_w<win_h?win_w:win_h);
            /* deep purple nebula — top right */
            SDL_SetTextureColorMod(blob_tex, 120, 30, 200);
            SDL_SetTextureAlphaMod(blob_tex, 60);
            { int r=(int)(base*0.75f); SDL_Rect d={win_w-r,  -r/3, r*2,r*2}; SDL_RenderCopy(ren,blob_tex,NULL,&d); }
            /* magenta nebula — bottom left */
            SDL_SetTextureColorMod(blob_tex, 200, 30, 140);
            SDL_SetTextureAlphaMod(blob_tex, 45);
            { int r=(int)(base*0.60f); SDL_Rect d={-r/2, win_h-r, r*2,r*2}; SDL_RenderCopy(ren,blob_tex,NULL,&d); }
            /* teal core — center */
            SDL_SetTextureColorMod(blob_tex, 40, 80, 200);
            SDL_SetTextureAlphaMod(blob_tex, 28);
            { int r=(int)(base*0.45f); SDL_Rect d={win_w/2-r, win_h/2-r, r*2,r*2}; SDL_RenderCopy(ren,blob_tex,NULL,&d); }
            /* accent glow — top center */
            SDL_SetTextureColorMod(blob_tex, 180, 80, 255);
            SDL_SetTextureAlphaMod(blob_tex, 22);
            { int r=(int)(base*0.35f); SDL_Rect d={win_w/2-r, -r/2, r*2,r*2}; SDL_RenderCopy(ren,blob_tex,NULL,&d); }
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    } else {
        /* Normal themes — accent-tinted blobs */
        int base = (win_w < win_h ? win_w : win_h);
        int r1 = (int)(base * 0.70f);
        int r2 = (int)(base * 0.58f);
        int r3 = (int)(base * 0.42f);

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        draw_blob(win_w,          0,              r1, 0.82f);
        draw_blob(0,              win_h,          r2, 0.65f);
        draw_blob(win_w*55/100,   win_h*50/100,   r3, 0.40f);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    /* Grain overlay — all themes */
    if(grain_tex){
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        for(int ty=0;ty<win_h;ty+=GRAIN_SZ)
            for(int tx=0;tx<win_w;tx+=GRAIN_SZ){
                SDL_Rect dst={tx,ty,GRAIN_SZ,GRAIN_SZ};
                SDL_RenderCopy(ren, grain_tex, NULL, &dst);
            }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
}