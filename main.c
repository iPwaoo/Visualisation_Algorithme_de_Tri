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

typedef enum {
    ALG_BUBBLE, ALG_SELECTION, ALG_INSERTION, ALG_QUICK, ALG_MERGE
} Algorithm;

typedef struct {
    uint64_t reads;
    uint64_t writes;
    uint64_t comps;
    uint64_t swaps;
    uint64_t frames;
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
static bool gSorting = false;
static bool gSorted  = false;
static uint64_t t0_ticks = 0;
static double perf_freq_ms = 1.0;

static int hiA = -1, hiB = -1; // indices surlignés

// ---- Utils ----
static void rnd_array(Array *arr) {
    for (int i = 0; i < arr->n; ++i) {
        arr->a[i] = 1 + rand() % (arr->maxVal - 1);
    }
    gSorted = false;
}

static void reset_metrics() {
    memset(&gM, 0, sizeof(gM));
}

static void start_timer() {
    t0_ticks = SDL_GetPerformanceCounter();
}

static void update_time() {
    uint64_t t = SDL_GetPerformanceCounter();
    gM.elapsed_ms = (double)(t - t0_ticks) * perf_freq_ms;
}

// ---- Comptage & visualisation ----
static void render_array(Gfx *gfx, Array *arr, const char *subtitle) {
    SDL_SetRenderDrawColor(gfx->ren, 20, 22, 28, 255);
    SDL_RenderClear(gfx->ren);

    int bw = (gfx->w) / arr->n; if (bw < 1) bw = 1;
    for (int i = 0; i < arr->n; ++i) {
        int h = (arr->a[i] * (gfx->h - 120)) / arr->maxVal; // laisser de la place au texte
        int x = i * bw;
        int y = gfx->h - h;

        if (i == hiA || i == hiB) SDL_SetRenderDrawColor(gfx->ren, 230, 90, 90, 255);       // comparés
        else                      SDL_SetRenderDrawColor(gfx->ren, 90, 170, 255, 255);      // normal

        SDL_Rect r = {x, y, bw - 1, h};
        SDL_RenderFillRect(gfx->ren, &r);
    }

    // Bandeau stats
    char line1[256], line2[256];
    const char *algName = (gAlg==ALG_BUBBLE?"Bubble":
                          (gAlg==ALG_SELECTION?"Selection":
                          (gAlg==ALG_INSERTION?"Insertion":
                          (gAlg==ALG_QUICK?"QuickSort":"MergeSort"))));
    snprintf(line1, sizeof(line1),
             "Algorithme: %s | N=%d | %s",
             algName, arr->n, gSorted? "TERMINE" : (gSorting? "EN COURS" : "PRET"));
    snprintf(line2, sizeof(line2),
             "Acces mem: R=%llu W=%llu | cmp=%llu | swaps=%llu | time=%.1f ms | frames=%llu",
             (unsigned long long)gM.reads, (unsigned long long)gM.writes,
             (unsigned long long)gM.comps, (unsigned long long)gM.swaps,
             gM.elapsed_ms, (unsigned long long)gM.frames);

    SDL_Color fg = {235, 235, 235, 255};
    SDL_Surface *s1 = TTF_RenderUTF8_Blended(gfx->font, line1, fg);
    SDL_Surface *s2 = TTF_RenderUTF8_Blended(gfx->font, line2, fg);
    SDL_Surface *s3 = subtitle && *subtitle ? TTF_RenderUTF8_Blended(gfx->font, subtitle, fg) : NULL;

    SDL_Texture *t1 = SDL_CreateTextureFromSurface(gfx->ren, s1);
    SDL_Texture *t2 = SDL_CreateTextureFromSurface(gfx->ren, s2);
    SDL_Texture *t3 = s3 ? SDL_CreateTextureFromSurface(gfx->ren, s3) : NULL;

    SDL_Rect r1 = {16, 16, s1->w, s1->h};
    SDL_Rect r2 = {16, 16 + s1->h + 6, s2->w, s2->h};
    SDL_Rect r3 = {16, 16 + s1->h + 6 + s2->h + 6, s3? s3->w : 0, s3? s3->h : 0};

    SDL_RenderCopy(gfx->ren, t1, NULL, &r1);
    SDL_RenderCopy(gfx->ren, t2, NULL, &r2);
    if (t3) SDL_RenderCopy(gfx->ren, t3, NULL, &r3);

    SDL_FreeSurface(s1); SDL_FreeSurface(s2); if (s3) SDL_FreeSurface(s3);
    SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); if (t3) SDL_DestroyTexture(t3);

    SDL_RenderPresent(gfx->ren);
    gM.frames++;
}

static void visual_tick(Gfx *gfx, Array *arr, int a, int b, const char *subtitle) {
    hiA = a; hiB = b;
    update_time();
    render_array(gfx, arr, subtitle);
    SDL_Delay(DELAY_MS);
}

// wrappers pour compter les accès (get/set)
static int getA(Array *arr, int i) { gM.reads++; return arr->a[i]; }
static void setA(Array *arr, int i, int v) { gM.writes++; arr->a[i]=v; }

// comparaison instrumentée
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
    int tmp = getA(arr,i);            // read
    int vj  = getA(arr,j);            // read
    setA(arr,i, vj);                  // write
    setA(arr,j, tmp);                 // write
    gM.swaps++;
    visual_tick(gfx, arr, i, j, "Echange");
}

// ---- Algorithmes pas-à-pas ----
static void sort_bubble(Gfx *gfx, Array *arr) {
    for (int i = 0; i < arr->n-1; ++i) {
        bool swapped = false;
        for (int j = 0; j < arr->n-1-i; ++j) {
            if (cmp_idx(gfx, arr, j, j+1) > 0) {
                swap_idx(gfx, arr, j, j+1);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

static void sort_selection(Gfx *gfx, Array *arr) {
    for (int i = 0; i < arr->n-1; ++i) {
        int minIdx = i;
        for (int j = i+1; j < arr->n; ++j) {
            if (cmp_idx(gfx, arr, j, minIdx) < 0) minIdx = j;
        }
        swap_idx(gfx, arr, i, minIdx);
    }
}

static void sort_insertion(Gfx *gfx, Array *arr) {
    for (int i = 1; i < arr->n; ++i) {
        int key = getA(arr, i); // read
        int j = i - 1;
        // surligne key via hiA
        hiA = i; hiB = -1;
        visual_tick(gfx, arr, i, -1, "Insertion: element cle");
        while (j >= 0) {
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

// QuickSort instrumenté
static int partition(Gfx *gfx, Array *arr, int low, int high) {
    int pivot = getA(arr, high);
    int i = low - 1;
    for (int j = low; j <= high-1; ++j) {
        gM.comps++;
        visual_tick(gfx, arr, j, high, "QS: comparer au pivot");
        int vj = getA(arr, j);
        if (vj <= pivot) {
            i++;
            swap_idx(gfx, arr, i, j);
        }
    }
    swap_idx(gfx, arr, i+1, high);
    return i+1;
}

static void quicksort(Gfx *gfx, Array *arr, int low, int high) {
    if (low < high) {
        int pi = partition(gfx, arr, low, high);
        quicksort(gfx, arr, low, pi-1);
        quicksort(gfx, arr, pi+1, high);
    }
}

static void sort_quick(Gfx *gfx, Array *arr) {
    quicksort(gfx, arr, 0, arr->n-1);
}

// MergeSort instrumenté
static void merge(Gfx *gfx, Array *arr, int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;
    int *L = (int*)malloc(n1 * sizeof(int));
    int *R = (int*)malloc(n2 * sizeof(int));
    for (int i=0;i<n1;++i){ L[i]=getA(arr, l+i); }
    for (int j=0;j<n2;++j){ R[j]=getA(arr, m+1+j); }

    int i=0,j=0,k=l;
    while (i<n1 && j<n2) {
        gM.comps++;
        visual_tick(gfx, arr, k, -1, "Merge: ecriture");
        if (L[i] <= R[j]) { setA(arr, k, L[i]); i++; }
        else { setA(arr, k, R[j]); j++; }
        k++;
    }
    while (i<n1){ setA(arr, k, L[i]); visual_tick(gfx, arr, k, -1, "Merge: reste L"); i++; k++; }
    while (j<n2){ setA(arr, k, R[j]); visual_tick(gfx, arr, k, -1, "Merge: reste R"); j++; k++; }

    free(L); free(R);
}

static void mergesort(Gfx *gfx, Array *arr, int l, int r) {
    if (l>=r) return;
    int m = l + (r-l)/2;
    mergesort(gfx, arr, l, m);
    mergesort(gfx, arr, m+1, r);
    merge(gfx, arr, l, m, r);
}

static void sort_merge(Gfx *gfx, Array *arr) {
    mergesort(gfx, arr, 0, arr->n-1);
}

// ---- App ----
static const char *help_text =
    "[R] nouveau tableau  [ESPACE] lancer  [B]ubble [S]election [I]nsertion [Q]uick [M]erge  [UP/DOWN] N  [Esc] sortir";

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        return 1;
    }

    Gfx gfx = {0};
    gfx.w = WIN_W; gfx.h = WIN_H;
    gfx.win = SDL_CreateWindow("Sort Viz SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gfx.w, gfx.h, SDL_WINDOW_SHOWN);
    gfx.ren = SDL_CreateRenderer(gfx.win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gfx.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!gfx.win || !gfx.ren || !gfx.font) {
        fprintf(stderr, "Init window/renderer/font failed: %s | %s\n", SDL_GetError(), TTF_GetError());
        return 1;
    }

    perf_freq_ms = 1000.0 / (double)SDL_GetPerformanceFrequency();

    Array arr = {0};
    arr.n = INIT_N;
    arr.a = (int*)malloc(MAX_N * sizeof(int)); // buffer max pour pouvoir augmenter N
    arr.maxVal = WIN_H - 140;
    rnd_array(&arr);

    // premier rendu
    render_array(&gfx, &arr, help_text);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = false;
                else if (k == SDLK_r) { rnd_array(&arr); reset_metrics(); gSorting=false; gSorted=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_SPACE && !gSorting) { gSorting = true; gSorted=false; reset_metrics(); start_timer(); }
                else if (k == SDLK_b) { gAlg = ALG_BUBBLE; gSorting=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_s) { gAlg = ALG_SELECTION; gSorting=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_i) { gAlg = ALG_INSERTION; gSorting=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_q) { gAlg = ALG_QUICK; gSorting=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_m) { gAlg = ALG_MERGE; gSorting=false; render_array(&gfx,&arr,help_text); }
                else if (k == SDLK_UP) {
                    if (arr.n < MAX_N) { arr.n += 5; rnd_array(&arr); reset_metrics(); gSorting=false; render_array(&gfx,&arr,help_text); }
                } else if (k == SDLK_DOWN) {
                    if (arr.n > MIN_N) { arr.n -= 5; rnd_array(&arr); reset_metrics(); gSorting=false; render_array(&gfx,&arr,help_text); }
                }
            }
        }

        if (gSorting) {
            // On lance l'algo choisi en pas-à-pas (chaque opération appelle visual_tick)
            switch (gAlg) {
                case ALG_BUBBLE:    sort_bubble(&gfx, &arr); break;
                case ALG_SELECTION: sort_selection(&gfx, &arr); break;
                case ALG_INSERTION: sort_insertion(&gfx, &arr); break;
                case ALG_QUICK:     sort_quick(&gfx, &arr); break;
                case ALG_MERGE:     sort_merge(&gfx, &arr); break;
            }
            gSorting = false; gSorted = true;
            update_time();
            visual_tick(&gfx, &arr, -1, -1, "Termine. [R] pour rejouer, changer d'algo puis [ESPACE].");
        } else {
            // idle: garder l’affichage fluide
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
