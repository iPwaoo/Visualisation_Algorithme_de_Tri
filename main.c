#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define WIN_W 800
#define WIN_H 600
#define MIN_N 10
#define MAX_N 400
#define INIT_N 100
#define DELAY_MS 5

// Layout “texte uniquement”
#define SIDE_MARGIN 12
#define TOP_MARGIN 12
#define BETWEEN_SECTIONS 10   // espace entre haut / centre / bas
#define TOKEN_SPACING_X 16    // espace horizontal entre “tokens”
#define TOKEN_SPACING_Y 6     // espace vertical entre lignes de tokens
#define BOTTOM_PAD 12         // marge interne en bas au-dessus de l’écran

typedef enum { ALG_BUBBLE, ALG_SELECTION, ALG_INSERTION, ALG_QUICK, ALG_MERGE } Algorithm;

typedef struct { uint64_t reads, writes, comps, swaps, frames; double elapsed_ms; } Metrics;

typedef struct {
    SDL_Window   *win;
    SDL_Renderer *ren;
    TTF_Font     *font;
    int          w, h;
} Gfx;

typedef struct { int *a; int n; int maxVal; } Array;

static Metrics gM;
static Algorithm gAlg = ALG_BUBBLE;
static bool gSorting=false, gSorted=false, gPaused=false, gAbort=false;
static uint64_t t0_ticks=0;
static double perf_freq_ms=1.0;
static int hiA=-1, hiB=-1;

// ---------- Utils ----------
static void rnd_array(Array *arr){ for(int i=0;i<arr->n;++i) arr->a[i]=1+rand()%((arr->maxVal-1>1)?arr->maxVal-1:1); gSorted=false; }
static void reset_metrics(){ memset(&gM,0,sizeof(gM)); }
static void start_timer(){ t0_ticks = SDL_GetPerformanceCounter(); }
static void update_time(){ uint64_t t=SDL_GetPerformanceCounter(); gM.elapsed_ms=(double)(t-t0_ticks)*perf_freq_ms; }

static const char* algo_name(Algorithm a){
    switch(a){ case ALG_BUBBLE: return "Bubble"; case ALG_SELECTION: return "Selection";
        case ALG_INSERTION: return "Insertion"; case ALG_QUICK: return "QuickSort";
        case ALG_MERGE: return "MergeSort"; default: return "?"; }
}
static const char* state_name(void){ if(gSorting) return gPaused?"PAUSE":"EN COURS"; return gSorted?"TERMINE":"PRET"; }

// ---------- Texte: mesure & rendu “wrap à gauche” ----------
typedef struct { int w, h; } Size;

// mesure largeur/hauteur d’un token (chaine UTF-8)
static Size measure_text(TTF_Font *font, const char *s){
    Size z={0,0}; if(!s||!*s) return z; TTF_SizeUTF8(font, s, &z.w, &z.h); return z;
}

// Calcule la hauteur totale que prendrait l’affichage des tokens wrappés
static int layout_tokens_height(TTF_Font *font, const char **tokens, int count, int max_w){
    int line_w=0, line_h=TTF_FontLineSkip(font), total_h=line_h;
    for(int i=0;i<count;++i){
        Size sz = measure_text(font, tokens[i]);
        if(i==0){ line_w = sz.w; }
        else{
            if(line_w + TOKEN_SPACING_X + sz.w > max_w){
                total_h += TOKEN_SPACING_Y + line_h;
                line_w = sz.w;
            } else {
                line_w += TOKEN_SPACING_X + sz.w;
            }
        }
    }
    return total_h;
}

// Dessine les tokens alignés à gauche, en retour à la ligne si dépassement
static void draw_tokens_left_wrap(SDL_Renderer *ren, TTF_Font *font, const char **tokens, int count,
                                  int x, int y, int max_w, SDL_Color col, int *out_bottom_y){
    int cx=x, cy=y;
    int line_h = TTF_FontLineSkip(font);

    for(int i=0;i<count;++i){
        const char *txt = tokens[i];
        if(!txt || !*txt) continue;
        Size sz = measure_text(font, txt);

        // retour à la ligne ?
        if(cx != x && cx + TOKEN_SPACING_X + sz.w > x + max_w){
            // nouvelle ligne
            cy += TOKEN_SPACING_Y + line_h;
            cx = x;
        }
        // si début de ligne, pas d’espace; sinon ajouter le spacing
        if(cx != x) cx += TOKEN_SPACING_X;

        SDL_Color c = col;
        SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, c);
        if(s){
            SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
            SDL_Rect dst = { cx, cy + (line_h - s->h)/2, s->w, s->h };
            SDL_RenderCopy(ren, t, NULL, &dst);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
            cx += sz.w; // avance
        }
    }
    if(out_bottom_y) *out_bottom_y = cy + line_h; // baseline de fin
}

// ---------- Instrumentation ----------
static int  getA(Array *arr, int i){ gM.reads++; return arr->a[i]; }
static void setA(Array *arr, int i, int v){ gM.writes++; arr->a[i]=v; }

// ---------- Gestion évènements ----------
static void apply_resize(Gfx *gfx, Array *arr){
    SDL_GetWindowSize(gfx->win, &gfx->w, &gfx->h);
    // maxVal = hauteur dispo (approximative) pour les barres; ajustée plus finement à chaque rendu
    int approx_top = TOP_MARGIN + TTF_FontLineSkip(gfx->font) * 2;
    int approx_bot = TTF_FontLineSkip(gfx->font) * 2 + BOTTOM_PAD;
    arr->maxVal = gfx->h - approx_top - approx_bot - BETWEEN_SECTIONS*2;
    if(arr->maxVal < 10) arr->maxVal = 10;
}

static void handle_key(Gfx *gfx, Array *arr, SDL_Keycode k){
    if(k==SDLK_ESCAPE){ gAbort=true; gSorting=false; }
    else if(k==SDLK_SPACE){
        if(!gSorting && !gSorted){ gSorting=true; gPaused=false; gAbort=false; reset_metrics(); start_timer(); }
        else if(gSorting){ gPaused=!gPaused; }
        else if(gSorted){ gSorted=false; }
    } else if(k==SDLK_c){
        if(gSorting){ gAbort=true; gSorting=false; gPaused=false; }
    } else if(k==SDLK_r){
        rnd_array(arr); reset_metrics(); gSorting=false; gSorted=false;
    } else if(k==SDLK_b){ gAlg=ALG_BUBBLE; }
    else if(k==SDLK_s){ gAlg=ALG_SELECTION; }
    else if(k==SDLK_i){ gAlg=ALG_INSERTION; }
    else if(k==SDLK_q){ gAlg=ALG_QUICK; }
    else if(k==SDLK_m){ gAlg=ALG_MERGE; }
    else if(k==SDLK_UP){ if(arr->n<MAX_N){ arr->n+=5; rnd_array(arr); reset_metrics(); } }
    else if(k==SDLK_DOWN){ if(arr->n>MIN_N){ arr->n-=5; rnd_array(arr); reset_metrics(); } }
    (void)gfx;
}

static void pump_realtime(Gfx *gfx, Array *arr){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT){ gAbort=true; gSorting=false; }
        else if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){ apply_resize(gfx,arr); }
        else if(e.type==SDL_KEYDOWN){ handle_key(gfx,arr,e.key.keysym.sym); }
    }
    while(gSorting && gPaused && !gAbort){
        update_time();
        SDL_Delay(10);
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){ gAbort=true; gSorting=false; break; }
            else if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){ apply_resize(gfx,arr); }
            else if(e.type==SDL_KEYDOWN){ handle_key(gfx,arr,e.key.keysym.sym); }
        }
    }
}

// ---------- Tri ----------
static void visual_tick(Gfx *gfx, Array *arr, int a, int b, const char *subtitle);

static int cmp_idx(Gfx *gfx, Array *arr, int i, int j){
    gM.comps++; int vi=getA(arr,i), vj=getA(arr,j);
    visual_tick(gfx,arr,i,j,"Comparaison");
    if(vi< vj) return -1; if(vi> vj) return 1; return 0;
}
static void swap_idx(Gfx *gfx, Array *arr, int i, int j){
    if(i==j) return; int tmp=getA(arr,i), vj=getA(arr,j);
    setA(arr,i,vj); setA(arr,j,tmp); gM.swaps++;
    visual_tick(gfx,arr,i,j,"Echange");
}

static void sort_bubble(Gfx *gfx, Array *arr){
    for(int i=0;i<arr->n-1 && !gAbort;++i){
        bool swapped=false;
        for(int j=0;j<arr->n-1-i && !gAbort;++j){
            if(cmp_idx(gfx,arr,j,j+1)>0){ swap_idx(gfx,arr,j,j+1); swapped=true; }
        }
        if(!swapped) break;
    }
}
static void sort_selection(Gfx *gfx, Array *arr){
    for(int i=0;i<arr->n-1 && !gAbort;++i){
        int minIdx=i;
        for(int j=i+1;j<arr->n && !gAbort;++j)
            if(cmp_idx(gfx,arr,j,minIdx)<0) minIdx=j;
        swap_idx(gfx,arr,i,minIdx);
    }
}
static void sort_insertion(Gfx *gfx, Array *arr){
    for(int i=1;i<arr->n && !gAbort;++i){
        int key=getA(arr,i); int j=i-1; hiA=i; hiB=-1;
        visual_tick(gfx,arr,i,-1,"Insertion: element cle");
        while(j>=0 && !gAbort){
            gM.comps++; int vj=getA(arr,j);
            visual_tick(gfx,arr,j,j+1,"Comparaison/decallage");
            if(vj>key){ setA(arr,j+1,vj); visual_tick(gfx,arr,j,j+1,"Decalage"); j--; }
            else break;
        }
        setA(arr,j+1,key);
        visual_tick(gfx,arr,j+1,i,"Insertion cle");
    }
}
static int partition(Gfx *gfx, Array *arr, int low, int high){
    int pivot=getA(arr,high); int i=low-1;
    for(int j=low;j<=high-1 && !gAbort;++j){
        gM.comps++; visual_tick(gfx,arr,j,high,"QS: comparer au pivot");
        int vj=getA(arr,j); if(vj<=pivot){ i++; swap_idx(gfx,arr,i,j); }
    }
    if(!gAbort) swap_idx(gfx,arr,i+1,high);
    return i+1;
}
static void quicksort(Gfx *gfx, Array *arr, int low, int high){
    if(gAbort || low>=high) return;
    int pi=partition(gfx,arr,low,high); if(gAbort) return;
    quicksort(gfx,arr,low,pi-1); quicksort(gfx,arr,pi+1,high);
}
static void sort_quick(Gfx *gfx, Array *arr){ quicksort(gfx,arr,0,arr->n-1); }
static void merge(Gfx *gfx, Array *arr, int l, int m, int r){
    int n1=m-l+1,n2=r-m; int *L=malloc(n1*sizeof(int)), *R=malloc(n2*sizeof(int));
    for(int i=0;i<n1;++i) L[i]=getA(arr,l+i);
    for(int j=0;j<n2;++j) R[j]=getA(arr,m+1+j);
    int i=0,j=0,k=l;
    while(i<n1 && j<n2 && !gAbort){
        gM.comps++; visual_tick(gfx,arr,k,-1,"Merge: ecriture");
        if(L[i]<=R[j]){ setA(arr,k,L[i]); i++; } else { setA(arr,k,R[j]); j++; } k++;
    }
    while(i<n1 && !gAbort){ setA(arr,k,L[i]); visual_tick(gfx,arr,k,-1,"Merge: reste L"); i++; k++; }
    while(j<n2 && !gAbort){ setA(arr,k,R[j]); visual_tick(gfx,arr,k,-1,"Merge: reste R"); j++; k++; }
    free(L); free(R);
}
static void mergesort(Gfx *gfx, Array *arr, int l, int r){
    if(gAbort || l>=r) return; int m=l+(r-l)/2;
    mergesort(gfx,arr,l,m); mergesort(gfx,arr,m+1,r); if(!gAbort) merge(gfx,arr,l,m,r);
}
static void sort_merge(Gfx *gfx, Array *arr){ mergesort(gfx,arr,0,arr->n-1); }

// ---------- Rendu principal (texte-only responsive) ----------
static void render_array(Gfx *gfx, Array *arr, const char *subtitle){
    SDL_SetRenderDrawColor(gfx->ren, 20,22,28,255); // fond
    SDL_RenderClear(gfx->ren);

    SDL_Color white = (SDL_Color){235,235,235,255};

    // --- 1) Construire les tokens TOP (stats + infos)
    char tAlgo[32], tEtat[32], tN[24], tReads[32], tWrites[32], tComps[32], tSwaps[32], tTime[32], tFrames[24];
    snprintf(tAlgo,sizeof tAlgo,"Algorithme: %s", algo_name(gAlg));
    snprintf(tEtat,sizeof tEtat,"Etat: %s", state_name());
    snprintf(tN,   sizeof tN,  "N: %d", arr->n);
    snprintf(tReads,sizeof tReads,"Lectures: %llu",(unsigned long long)gM.reads);
    snprintf(tWrites,sizeof tWrites,"Ecritures: %llu",(unsigned long long)gM.writes);
    snprintf(tComps,sizeof tComps,"Comparaisons: %llu",(unsigned long long)gM.comps);
    snprintf(tSwaps,sizeof tSwaps,"Swaps: %llu",(unsigned long long)gM.swaps);
    snprintf(tTime,sizeof tTime,"Temps: %.1f ms", gM.elapsed_ms);
    snprintf(tFrames,sizeof tFrames,"Frames: %llu",(unsigned long long)gM.frames);

    const char *top_tokens[] = { tAlgo, tEtat, tN, tReads, tWrites, tComps, tSwaps, tTime, tFrames };
    int top_count = (int)(sizeof(top_tokens)/sizeof(top_tokens[0]));

    // Mesurer la hauteur occupée en haut
    int top_height = layout_tokens_height(gfx->font, top_tokens, top_count, gfx->w - 2*SIDE_MARGIN);

    // --- 2) Construire les tokens BOTTOM (contrôles)
    const char *bot_tokens[] = {
        "[ESPACE] Lancer / Pause / Reprendre",
        "[C] Annuler",
        "[B] Bubble", "[S] Selection", "[I] Insertion", "[Q] Quick", "[M] Merge",
        "[R] Nouveau tableau",
        "[UP/DOWN] Taille",
        "[Esc] Quitter"
    };
    int bot_count = (int)(sizeof(bot_tokens)/sizeof(bot_tokens[0]));

    int bottom_height = layout_tokens_height(gfx->font, bot_tokens, bot_count, gfx->w - 2*SIDE_MARGIN);

    // --- 3) Dessiner TOP (aligné à gauche, wrap)
    int after_top_y;
    draw_tokens_left_wrap(gfx->ren, gfx->font, top_tokens, top_count,
                          SIDE_MARGIN, TOP_MARGIN, gfx->w - 2*SIDE_MARGIN, white, &after_top_y);

    // --- 4) Dessiner BOTTOM (aligné à gauche, wrap) au-dessus du bord bas
    int bottom_start_y = gfx->h - bottom_height - BOTTOM_PAD;
    int after_bottom_y;
    draw_tokens_left_wrap(gfx->ren, gfx->font, bot_tokens, bot_count,
                          SIDE_MARGIN, bottom_start_y, gfx->w - 2*SIDE_MARGIN, white, &after_bottom_y);

    // --- 5) Zone centrale: barres
    int avail_top = after_top_y + BETWEEN_SECTIONS;
    int avail_bottom = bottom_start_y - BETWEEN_SECTIONS;
    int avail_h = avail_bottom - avail_top;
    if(avail_h < 40) avail_h = 40;

    // Ajuster maxVal pour coller à la zone (préserve proportions)
    arr->maxVal = avail_h - 8; if(arr->maxVal < 10) arr->maxVal = 10;

    int bw = (gfx->w) / arr->n; if(bw < 1) bw = 1;
    for(int i=0;i<arr->n;++i){
        int h = (arr->a[i] * (avail_h-2)) / (arr->maxVal>0?arr->maxVal:1);
        if(h<1) h=1;
        int x = i*bw;
        int y = avail_top + (avail_h - h);
        if(i==hiA || i==hiB) SDL_SetRenderDrawColor(gfx->ren, 230, 90, 90, 255);
        else                 SDL_SetRenderDrawColor(gfx->ren,  90,170,255,255);
        SDL_Rect r={x,y,bw-1,h};
        SDL_RenderFillRect(gfx->ren,&r);
    }

    // Sous-titre (ex: “Comparaison”, “Echange”, etc.), au-dessus de la zone bas
    if(subtitle && *subtitle){
        SDL_Surface *s = TTF_RenderUTF8_Blended(gfx->font, subtitle, white);
        if(s){
            SDL_Texture *t = SDL_CreateTextureFromSurface(gfx->ren, s);
            int y = bottom_start_y - 8 - s->h;
            SDL_Rect dst = { SIDE_MARGIN, y, s->w, s->h };
            if(dst.y > avail_top) SDL_RenderCopy(gfx->ren, t, NULL, &dst);
            SDL_DestroyTexture(t); SDL_FreeSurface(s);
        }
    }

    SDL_RenderPresent(gfx->ren);
    gM.frames++;
}

static void visual_tick(Gfx *gfx, Array *arr, int a, int b, const char *subtitle){
    hiA=a; hiB=b; update_time(); render_array(gfx,arr,""); pump_realtime(gfx,arr); SDL_Delay(DELAY_MS);
}

// ---------- Main ----------
int main(int argc, char **argv){
    (void)argc; (void)argv; srand((unsigned)time(NULL));
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){ fprintf(stderr,"SDL_Init: %s\n",SDL_GetError()); return 1; }
    if(TTF_Init()!=0){ fprintf(stderr,"TTF_Init: %s\n",TTF_GetError()); return 1; }

    Gfx gfx={0}; gfx.w=WIN_W; gfx.h=WIN_H;
    gfx.win = SDL_CreateWindow("Sort Viz SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gfx.w, gfx.h,
                               SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    gfx.ren = SDL_CreateRenderer(gfx.win, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    gfx.font= TTF_OpenFont("Coolvetica Rg.otf", 16);
    if(!gfx.win || !gfx.ren || !gfx.font){ fprintf(stderr,"Init error: %s | %s\n", SDL_GetError(), TTF_GetError()); return 1; }

    perf_freq_ms = 1000.0 / (double)SDL_GetPerformanceFrequency();

    Array arr={0}; arr.n=INIT_N; arr.a=(int*)malloc(MAX_N*sizeof(int));
    arr.maxVal = 200; rnd_array(&arr);

    render_array(&gfx,&arr,"");

    bool running=true;
    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){ running=false; gAbort=true; gSorting=false; }
            else if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){
                apply_resize(&gfx,&arr);
                render_array(&gfx,&arr, gSorting?(gPaused?"PAUSE":"EN COURS"):"Pret.");
            } else if(e.type==SDL_KEYDOWN){
                handle_key(&gfx,&arr,e.key.keysym.sym);
                if(e.key.keysym.sym==SDLK_ESCAPE) running=false;
                render_array(&gfx,&arr, gSorting?(gPaused?"PAUSE":"EN COURS"):"Pret.");
            }
        }

        if(gAbort) break;

        if(gSorting){
            gAbort=false;
            switch(gAlg){
                case ALG_BUBBLE:    sort_bubble(&gfx,&arr); break;
                case ALG_SELECTION: sort_selection(&gfx,&arr); break;
                case ALG_INSERTION: sort_insertion(&gfx,&arr); break;
                case ALG_QUICK:     sort_quick(&gfx,&arr); break;
                case ALG_MERGE:     sort_merge(&gfx,&arr); break;
            }
            gSorting=false;
            if(!gAbort){ gSorted=true; update_time(); visual_tick(&gfx,&arr,-1,-1,"Termine. [R] pour rejouer."); }
            else       { gSorted=false; update_time(); render_array(&gfx,&arr,"Annule. Pret."); }
        } else {
            SDL_Delay(10);
        }
    }

    free(arr.a);
    TTF_CloseFont(gfx.font);
    SDL_DestroyRenderer(gfx.ren);
    SDL_DestroyWindow(gfx.win);
    TTF_Quit(); SDL_Quit();
    return 0;
}
