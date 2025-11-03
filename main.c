#include "common.h"
#include "utils.h"
#include "sorting.h"
#include "visual.h"
#include "stats.h"



int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Status *status = NULL;
    status = calloc(1, sizeof(*status));
    if (!status) { perror("calloc status"); return 1; }

    status->aAlg = ALG_BUBBLE;
    status->hiA = -1;
    status->hiB = -1;

    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    Graphisme gfx = {0};
    gfx.width = WIN_WIDTH;
    gfx.height = WIN_HEIGHT;
    gfx.window = SDL_CreateWindow("Sort Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        gfx.width, gfx.height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    gfx.render = SDL_CreateRenderer(gfx.window, -1, SDL_RENDERER_ACCELERATED);
    gfx.font = TTF_OpenFont("Coolvetica Rg.otf", 16);

    Array arr = {0};
    arr.n = INIT_N;
    arr.a = malloc(MAX_N * sizeof(int));
    arr.maxVal = 200;
    random_array(&arr);
    status->bSorted = false;

    reset_metrics();
    render_array(&gfx, &arr, "", status);
    status->bRunning = true;
    while (status->bRunning && !status->bAbort) {
        handle_key(&gfx, &arr, status);

        if (status->bAbort)
            break;

        if (status->bSorting) {
            printf("sorting %d ", status->bSorting);
            fflush(stdout);
            status->bAbort = false;
            switch (status->aAlg)
            {
            case ALG_BUBBLE:
                sort_bubble(&gfx, &arr, status);
                printf("sort_bubble ");
                fflush(stdout);
                print_status(status);
                break;
            case ALG_SELECTION:
                sort_selection(&gfx, &arr, status);
                break;
            case ALG_INSERTION:
                sort_insertion(&gfx, &arr, status);
                break;
            case ALG_QUICK:
                sort_quick(&gfx, &arr, status);
                break;
            case ALG_MERGE:
                sort_merge(&gfx, &arr, status);
                break;
            }
            status->bSorting = false;
            if (!status->bAbort)
            {
                status->bSorted = true;
                update_time();
                visual_tick(&gfx, &arr, -1, -1, "Termine. [R] pour rejouer.", status);
            }
            else
            {
                status->bSorted = false;
                update_time();
                render_array(&gfx, &arr, "Annule. Pret.", status);
            }
        }
        else
        {
            SDL_Delay(10);
        }
    }

    free(arr.a);
    free(status);
    TTF_CloseFont(gfx.font);
    SDL_DestroyRenderer(gfx.render);
    SDL_DestroyWindow(gfx.window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
/**
 * SDL_Event e;
        while (SDL_PollEvent(&e)) 
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
                status->bAbort = true;
                status->bSorting = false;
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                apply_resize(&gfx, &arr);
                render_array(&gfx, &arr, status->bSorting ? (status->bPaused ? "PAUSE" : "EN COURS") : "Pret.", status);
            }
            else if (e.type == SDL_KEYDOWN)
            {
                print_keyboard_event(&e.key);
                handle_key(&arr, status);
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                printf("Key_handle = %d",e.key.keysym.sym);
                fflush(stdout);
                render_array(&gfx, &arr, status->bSorting ? (status->bPaused ? "PAUSE" : "EN COURS") : "Pret.",status);
            }
        }
 */