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

typedef enum { ALG_BUBBLE, ALG_SELECTION, ALG_INSERTION, ALG_QUICK, ALG_MERGE } Algorithm;

typedef struct {
    uint64_t reads, writes, comps, swaps, frames;
    double   elapsed_ms;
} Metrics;

typedef struct {
    SDL_Window   *win;
    SDL_Renderer *ren;
    TTF_Font     *font;
    int          w, h;
} Gfx;

typedef struct {
    int *a;
    int n;
    int maxVal;
} Array;

static Metrics gM;
static Algorithm gAlg = ALG_BUBBLE;
static bool gSorting = false;   // tri en cours ?
static bool gSorted  = false;   // tri terminé ?
static bool gPaused  = false;   // pause en cours ?
static bool gAbort   = false;   // annulation demandée ?
static uint64_t t0_ticks = 0;
static double perf_freq_ms = 1.0;

static int hiA = -1, hiB = -1; // indices surlignés

// ---- Textes d’aide (wrappés) ----
static const char *help_text =
    "Commandes :\n"
    "  [ESPACE] Lancer / Pause / Reprendre    [C] Annuler le tri\n"
    "  [B] Bubble  [S] Selection  [I] Insertion  [Q] Quick  [M] Merge\n"
    "  [R] Nouveau tableau    [UP/DOWN] Changer N    [Esc] Quitter\n"
    "Fenetre redimensionnable : le layout s'ajuste automatiquement.";

// ---------- Utils ----------
static void rnd_array(Array *arr) {
    for (int i = 0; i < arr->n; ++i) arr->a[i] = 1 + rand() % (arr->maxVal - 1);
    gSorted = false;
}
static void reset_metrics() { memset(&gM, 0, sizeof(gM)); }
static void start_timer() { t0_ticks = SDL_GetPerformanceCounter(); }
static void update_time() {
    uint64_t t = SDL_GetPerformanceCounter();
    gM.elapsed_ms = (double)(t - t0_ticks) * perf_freq_ms;
}

// ---------- Dessin ----------
static void draw_panel(Gfx *gfx, SDL_Rect r, Uint8 a) {
    SDL_SetRenderDrawBlendMode(gfx->ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(gfx->ren, 10, 10, 14, a);
    SDL_RenderFillRect(gfx->ren, &r);
}

static void draw_wrapped_text(Gfx *gfx, const char *text, int x, int y, int wrap_w) {
    SDL_Color fg = {235,235,235,255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended_Wrapped(gfx->font, text, fg, wrap_w);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(gfx->ren, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(gfx->ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

static void render_array(Gfx *gfx, Array *arr, const char *subtitle) {
    SDL_SetRenderDrawColor(gfx->ren, 20, 22, 28, 255);
    SDL_RenderClear(gfx->ren);

    // Barres
    int bw = (gfx->w) / arr->n; if (bw < 1) bw = 1;
    for (int i = 0; i < arr->n; ++i) {
        int h = (arr->a[i] * (gfx->h - 150)) / arr->maxVal; // on garde un bandeau pour les textes
        if (h < 1) h = 1;
        int x = i * bw;
        int y = gfx->h - h;

        if (i == hiA || i == hiB) SDL_SetRenderDrawColor(gfx->ren, 230, 90, 90, 255);
        else                      SDL_SetRenderDrawColor(gfx->ren, 90, 170, 255, 255);

        SDL_Rect r = {x, y, bw - 1, h};
        SDL_RenderFillRect(gfx->ren, &r);
    }

    // Bandeau stats + aides (wrappés)
    SDL_Rect panel = {12, 12, gfx->w - 24, 120};
    draw_panel(gfx, panel, 130);

    const char *algName = (gAlg==ALG_BUBBLE?"Bubble":
                          (gAlg==ALG_SELECTION?"Selection":
                          (gAlg==ALG_INSERTION?"Insertion":
                          (gAlg==ALG_QUICK?"QuickSort":"MergeSort"))));

    char line1[256], line2[256], line3[256];
    snprintf(line1, sizeof(line1),
             "Algorithme: %s | N=%d | %s",
             algName, arr->n, gSorted? "TERMINE" : (gSorting? (gPaused? "PAUSE" : "EN COURS") : "PRET"));
    snprintf(line2, sizeof(line2),
             "Acces mem: R=%llu W=%llu | cmp=%llu | swaps=%llu | time=%.1f ms | frames=%llu",
             (unsigned long long)gM.reads, (unsigned long long)gM.writes,
             (unsigned long long)gM.comps, (unsigned long long)gM.swaps,
             gM.elapsed_ms, (unsigned long long)gM.frames);
    snprintf(line3, sizeof(line3), "%s", subtitle ? subtitle : "");

    // Trois lignes “titre/stats”, puis aide wrappée en dessous
    SDL_Color fg = {235,235,235,255};
    SDL_Surface *s1 = TTF_RenderUTF8_Blended(gfx->font, line1, fg);
    SDL_Surface *s2 = TTF_RenderUTF8_Blended(gfx->font, line2, fg);
    SDL_Surface *s3 = TTF_RenderUTF8_Blended(gfx->font, line3, fg);

    SDL_Texture *t1 = SDL_CreateTextureFromSurface(gfx->ren, s1);
    SDL_Texture *t2 = SDL_CreateTextureFromSurface(gfx->ren, s2);
    SDL_Texture *t3 = SDL_CreateTextureFromSurface(gfx->ren, s3);

    int y = 18;
    SDL_Rect r1 = {18, y, s1->w, s1->h}; y += s1->h + 6;
    SDL_Rect r2 = {18, y, s2->w, s2->h}; y += s2->h + 6;
    SDL_Rect r3 = {18, y, s3->w, s3->h}; y += s3->h + 10;

    SDL_RenderCopy(gfx->ren, t1, NULL, &r1);
    SDL_RenderCopy(gfx->ren, t2, NULL, &r2);
    SDL_RenderCopy(gfx->ren, t3, NULL, &r3);

    SDL_FreeSurface(s1); SDL_FreeSurface(s2); SDL_FreeSurface(s3);
    SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3);

    draw_wrapped_text(gfx, help_text, 18, y, gfx->w - 36);

    SDL_RenderPresent(gfx->ren);
    gM.frames++;
}

// ---------- Instrumentation ----------
static int getA(Array *arr, int i) { gM.reads++; return arr->a[i]; }
static void setA(Array *arr, int i, int v) { gM.writes++; arr->a[i]=v; }

// ---------- Gestion évènements temps réel (interruptions, resize) ----------
static void apply_resize(Gfx *gfx, Array *arr) {
    SDL_GetWindowSize(gfx->win, &gfx->w, &gfx->h);
    arr->maxVal = gfx->h - 160;
    if (arr->maxVal < 10) arr->maxVal = 10; // garde-fou
}

static void handle_key(Gfx *gfx, Array *arr, SDL_Keycode k) {
    if (k == SDLK_ESCAPE) { gAbort = true; gSorting = false; } // quitter dans boucle principale
    else if (k == SDLK_SPACE) {
        if (!gSorting && !gSorted) { gSorting = true; gPaused = false; gAbort = false; reset_metrics(); start_timer(); }
        else if (gSorting) { gPaused = !gPaused; }
        else if (gSorted) { gSorted = false; } // prêt à relancer
    } else if (k == SDLK_c) {
        if (gSorting) { gAbort = true; gSorting = false; gPaused = false; }
    } else if (k == SDLK_r) {
        rnd_array(arr); reset_metrics(); gSorting=false; gSorted=false;
    } else if (k == SDLK_b) { gAlg = ALG_BUBBLE; }
    else if (k == SDLK_s)   { gAlg = ALG_SELECTION; }
    else if (k == SDLK_i)   { gAlg = ALG_INSERTION; }
    else if (k == SDLK_q)   { gAlg = ALG_QUICK; }
    else if (k == SDLK_m)   { gAlg = ALG_MERGE; }
    else if (k == SDLK_UP)   { if (arr->n < MAX_N) { arr->n += 5; rnd_array(arr); reset_metrics(); } }
    else if (k == SDLK_DOWN) { if (arr->n > MIN_N) { arr->n -= 5; rnd_array(arr); reset_metrics(); } }
    (void)gfx;
}

// Cette étape est appelée très souvent (après chaque op visuelle)
static void pump_realtime(Gfx *gfx, Array *arr) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { gAbort = true; gSorting = false; }
        else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            apply_resize(gfx, arr);
        } else if (e.type == SDL_KEYDOWN) {
            handle_key(gfx, arr, e.key.keysym.sym);
        }
    }

    // Pause coopérative : on reste réactif et on rerend
    while (gSorting && gPaused && !gAbort) {
        update_time();
        render_array(gfx, arr, "PAUSE - [ESPACE] pour reprendre, [C] pour annuler.");
        SDL_Delay(10);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { gAbort = true; gSorting = false; break; }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                apply_resize(gfx, arr);
            } else if (e.type == SDL_KEYDOWN) {
                handle_key(gfx, arr, e.key.keysym.sym);
            }
        }
    }
}

// Tick visuel + gestion temps réel + délai
static void visual_tick(Gfx *gfx, Array *arr, int a, int b, const char *subtitle) {
    hiA = a; hiB = b;
    update_time();
    render_array(gfx, arr, subtitle);
    pump_realtime(gfx, arr);
    SDL_Delay(DELAY_MS);
}

// ---------- Comparaisons / échanges ----------
static int cmp_idx(Gfx *gfx, Array *arr, int i, int j) {
    gM.comps++;
    int vi = getA(arr,i), vj = getA(arr,j);
    visual_tick(gfx, arr, i, j, "Comparaison");
    if (vi < vj) return -1;
    if (vi > vj) return 1;
    return 0;
}
static void swap_idx(Gfx *gfx, Array *arr, int i, int j) {
    if (i==j) return;
    int tmp = getA(arr,i);
    int vj  = getA(arr,j);
    setA(arr,i, vj);
    setA(arr,j, tmp);
    gM.swaps++;
    visual_tick(gfx, arr, i, j, "Echange");
}

// ---------- Algorithmes (coopératifs / interruptibles) ----------
static void sort_bubble(Gfx *gfx, Array *arr) {
    for (int i = 0; i < arr->n-1 && !gAbort; ++i) {
        bool swapped = false;
        for (int j = 0; j < arr->n-1-i && !gAbort; ++j) {
            if (cmp_idx(gfx, arr, j, j+1) > 0) { swap_idx(gfx, arr, j, j+1); swapped = true; }
        }
        if (!swapped) break;
    }
}

static void sort_selection(Gfx *gfx, Array *arr) {
    for (int i = 0; i < arr->n-1 && !gAbort; ++i) {
        int minIdx = i;
        for (int j = i+1; j < arr->n && !gAbort; ++j) {
            if (cmp_idx(gfx, arr, j, minIdx) < 0) minIdx = j;
        }
        swap_idx(gfx, arr, i, minIdx);
    }
}

static void sort_insertion(Gfx *gfx, Array *arr) {
    for (int i = 1; i < arr->n && !gAbort; ++i) {
        int key = getA(arr, i);
        int j = i - 1;
        hiA = i; hiB = -1;
        visual_tick(gfx, arr, i, -1, "Insertion: element cle");
        while (j >= 0 && !gAbort) {
            gM.comps++;
            int vj = getA(arr, j);
            visual_tick(gfx, arr, j, j+1, "Comparaison/decallage");
            if (vj > key) {
                setA(arr, j+1, vj);
                visual_tick(gfx, arr, j, j+1, "Decalage");
                j--;
            } else break;
        }
        setA(arr, j+1, key);
        visual_tick(gfx, arr, j+1, i, "Insertion cle");
    }
}

// QuickSort
static int partition(Gfx *gfx, Array *arr, int low, int high) {
    int pivot = getA(arr, high);
    int i = low - 1;
    for (int j = low; j <= high-1 && !gAbort; ++j) {
        gM.comps++;
        visual_tick(gfx, arr, j, high, "QS: comparer au pivot");
        int vj = getA(arr, j);
        if (vj <= pivot) { i++; swap_idx(gfx, arr, i, j); }
    }
    if (!gAbort) swap_idx(gfx, arr, i+1, high);
    return i+1;
}
static void quicksort(Gfx *gfx, Array *arr, int low, int high) {
    if (gAbort || low >= high) return;
    int pi = partition(gfx, arr, low, high);
    if (gAbort) return;
    quicksort(gfx, arr, low, pi-1);
    quicksort(gfx, arr, pi+1, high);
}
static void sort_quick(Gfx *gfx, Array *arr) { quicksort(gfx, arr, 0, arr->n-1); }

// MergeSort
static void merge(Gfx *gfx, Array *arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    int *L = (int*)malloc(n1*sizeof(int));
    int *R = (int*)malloc(n2*sizeof(int));
    for (int i=0;i<n1;++i){ L[i]=getA(arr, l+i); }
    for (int j=0;j<n2;++j){ R[j]=getA(arr, m+1+j); }

    int i=0,j=0,k=l;
    while (i<n1 && j<n2 && !gAbort) {
        gM.comps++;
        visual_tick(gfx, arr, k, -1, "Merge: ecriture");
        if (L[i] <= R[j]) { setA(arr, k, L[i]); i++; }
        else { setA(arr, k, R[j]); j++; }
        k++;
    }
    while (i<n1 && !gAbort){ setA(arr, k, L[i]); visual_tick(gfx, arr, k, -1, "Merge: reste L"); i++; k++; }
    while (j<n2 && !gAbort){ setA(arr, k, R[j]); visual_tick(gfx, arr, k, -1, "Merge: reste R"); j++; k++; }

    free(L); free(R);
}
static void mergesort(Gfx *gfx, Array *arr, int l, int r) {
    if (gAbort || l>=r) return;
    int m = l + (r-l)/2;
    mergesort(gfx, arr, l, m);
    mergesort(gfx, arr, m+1, r);
    if (!gAbort) merge(gfx, arr, l, m, r);
}
static void sort_merge(Gfx *gfx, Array *arr) { mergesort(gfx, arr, 0, arr->n-1); }

// ---------- Main ----------
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) { fprintf(stderr,"SDL_Init: %s\n",SDL_GetError()); return 1; }
    if (TTF_Init() != 0) { fprintf(stderr,"TTF_Init: %s\n",TTF_GetError()); return 1; }

    Gfx gfx = {0};
    gfx.w = WIN_W; gfx.h = WIN_H;
    gfx.win = SDL_CreateWindow("Sort Viz SDL2",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                gfx.w, gfx.h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    gfx.ren = SDL_CreateRenderer(gfx.win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gfx.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!gfx.win || !gfx.ren || !gfx.font) {
        fprintf(stderr, "Init error: %s | %s\n", SDL_GetError(), TTF_GetError()); return 1;
    }

    perf_freq_ms = 1000.0 / (double)SDL_GetPerformanceFrequency();

    Array arr = {0};
    arr.n = INIT_N;
    arr.a = (int*)malloc(MAX_N * sizeof(int));
    arr.maxVal = WIN_H - 160;
    rnd_array(&arr);

    render_array(&gfx, &arr, "Pret. [ESPACE] pour lancer.");

    bool running = true;
    while (running) {
        // Gestion événements “idle”
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; gAbort = true; gSorting=false; }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                apply_resize(&gfx, &arr);
                render_array(&gfx, &arr, gSorting? (gPaused? "PAUSE" : "EN COURS") : "Pret.");
            } else if (e.type == SDL_KEYDOWN) {
                handle_key(&gfx, &arr, e.key.keysym.sym);
                if (e.key.keysym.sym == SDLK_ESCAPE) { running = false; }
                render_array(&gfx, &arr, gSorting? (gPaused? "PAUSE" : "EN COURS") : "Pret.");
            }
        }

        if (gSorting) {
            gAbort = false; // (si relance)
            // L’algo choisi (coopératif). Chaque op appelle visual_tick -> events, pause, resize.
            switch (gAlg) {
                case ALG_BUBBLE:    sort_bubble(&gfx, &arr); break;
                case ALG_SELECTION: sort_selection(&gfx, &arr); break;
                case ALG_INSERTION: sort_insertion(&gfx, &arr); break;
                case ALG_QUICK:     sort_quick(&gfx, &arr); break;
                case ALG_MERGE:     sort_merge(&gfx, &arr); break;
            }
            gSorting = false;
            if (!gAbort) { gSorted = true; update_time(); visual_tick(&gfx, &arr, -1, -1, "Termine. [R] pour rejouer."); }
            else { gSorted = false; update_time(); render_array(&gfx, &arr, "Annule. Pret."); }
        } else {
            // Idle: on reste fluide pour l’UX et pour les resizes
            SDL_Delay(10);
        }
    }

    free(arr.a);
    TTF_CloseFont(gfx.font);
    SDL_DestroyRenderer(gfx.ren);
    SDL_DestroyWindow(gfx.win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
