#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL_image.h>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MIN_N 10
#define MAX_N 400
#define INIT_N 100
#define DELAY_MS 16

// Layout
#define SIDE_MARGIN 12
#define TOP_MARGIN 12
#define BETWEEN_SECTIONS 10 // espace entre haut / centre / bas
#define TOKEN_SPACING_X 16  // espace horizontal entre “tokens”
#define TOKEN_SPACING_Y 6   // espace vertical entre lignes de tokens
#define BOTTOM_PAD 12       // marge interne en bas au-dessus de l’écran


typedef enum {
    ALG_BUBBLE,
    ALG_SELECTION,
    ALG_INSERTION,
    ALG_QUICK,
    ALG_MERGE
} Algorithm;

typedef struct {
    uint64_t reads, writes, comps, swaps, frames;
    double elapsed_ms;
} Metrics;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *render;
    TTF_Font *font;
    SDL_Texture *img_texture;
    int img_width, img_height;
    int width, height;
} Graphisme;

typedef struct {
    int *array;
    int size;
    int max_value;
} Array;

typedef struct {
    bool bSorting;
    bool bSorted;
    bool bPaused;
    bool bAbort;
    Algorithm aAlg;
    int highlight_a;
    int highlight_b;
    bool bResetting;
    bool bRunning;
} Status;

typedef struct
{
    int width, height;
} Size;

#endif
