// bubble_sort_viz.c
// Visualize Bubble Sort with SDL2: vertical bars, per-step highlighting & delay.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define WINDOW_W   800
#define WINDOW_H   600
#define N          100        // number of elements/bars
#define DELAY_MS   15         // delay after each comparison (ms)

// Colors (RGBA)
static const SDL_Color BG_COLOR        = {  20,  20,  28, 255};
static const SDL_Color BAR_COLOR       = { 220, 220, 235, 255};
static const SDL_Color COMPARE_COLOR_A = { 255,  80,  80, 255}; // j
static const SDL_Color COMPARE_COLOR_B = {  80, 180, 255, 255}; // j+1
static const SDL_Color SORTED_COLOR    = { 120, 220, 120, 255}; // optional tail

static void set_color(SDL_Renderer* r, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

// Draw the array as bars. highlighted1 and highlighted2 are indices being compared,
// and sorted_from is the starting index of the sorted tail (n - 1 - i during bubble sort).
static void draw_array(SDL_Renderer* r, const int *a, int n,
                       int highlighted1, int highlighted2,
                       int sorted_from, int win_w, int win_h)
{
    // Clear background
    set_color(r, BG_COLOR);
    SDL_RenderClear(r);

    if (n <= 0) {
        SDL_RenderPresent(r);
        return;
    }

    // Find max to scale heights
    int maxv = 1;
    for (int i = 0; i < n; ++i) if (a[i] > maxv) maxv = a[i];

    // Compute bar width and spacing
    float bar_w_f = (float)win_w / (float)n;
    int bar_w = (int)bar_w_f;
    if (bar_w <= 0) bar_w = 1;

    // Slight gap between bars for readability when possible
    int gap = (bar_w >= 3) ? 1 : 0;

    for (int i = 0; i < n; ++i) {
        // Choose color
        if (i == highlighted1) {
            set_color(r, COMPARE_COLOR_A);
        } else if (i == highlighted2) {
            set_color(r, COMPARE_COLOR_B);
        } else if (sorted_from >= 0 && i >= sorted_from) {
            set_color(r, SORTED_COLOR);
        } else {
            set_color(r, BAR_COLOR);
        }

        // Height scaled to window
        float ratio = (float)a[i] / (float)maxv;
        int h = (int)(ratio * (win_h - 10)); // leave a tiny top margin
        if (h < 1) h = 1;

        // x position
        int x = (int)(i * bar_w_f);
        SDL_Rect rect = { x, win_h - h, bar_w - gap, h };
        SDL_RenderFillRect(r, &rect);
    }

    SDL_RenderPresent(r);
}

// Handle quit events (window close, ESC). Returns false if user requested to quit.
static bool pump_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Bubble Sort Visualization (SDL2)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Generate data: values between 1 and WINDOW_H (so we don't need extra scaling).
    int n = N;
    int *a = (int*)malloc(n * sizeof(int));
    if (!a) {
        fprintf(stderr, "Allocation failed.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    srand((unsigned)time(NULL));
    for (int i = 0; i < n; ++i) {
        a[i] = 10 + rand() % (WINDOW_H - 20); // keep some margin
    }

    // Initial draw
    int win_w = WINDOW_W, win_h = WINDOW_H;
    SDL_GetWindowSize(window, &win_w, &win_h);
    draw_array(renderer, a, n, -1, -1, -1, win_w, win_h);

    bool running = true;
    // Bubble Sort with per-step visualization
    for (int i = 0; i < n - 1 && running; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1 && running; ++j) {
            // Allow window events and resizing every step
            if (!pump_events()) { running = false; break; }
            SDL_GetWindowSize(window, &win_w, &win_h);

            // Show comparison (highlight j and j+1)
            draw_array(renderer, a, n, j, j + 1, n - i, win_w, win_h);
            SDL_Delay(DELAY_MS);

            if (a[j] > a[j + 1]) {
                int tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
                swapped = true;

                // Optional: brief extra frame to emphasize the swap
                if (!pump_events()) { running = false; break; }
                SDL_GetWindowSize(window, &win_w, &win_h);
                draw_array(renderer, a, n, j, j + 1, n - i, win_w, win_h);
                SDL_Delay(DELAY_MS);
            }
        }

        // Early exit if already sorted
        if (!swapped) break;
    }

    // Final frame (no highlights)
    if (running) {
        SDL_GetWindowSize(window, &win_w, &win_h);
        draw_array(renderer, a, n, -1, -1, 0, win_w, win_h);
    }

    // Wait until user closes or presses ESC (so they can view the result)
    while (running) {
        if (!pump_events()) break;
        SDL_Delay(10);
    }

    free(a);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
