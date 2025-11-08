#include "common.h"
#include "utils.h"
#include "sorting.h"
#include "visual.h"
#include "stats.h"

static void init_Status(Status *status)
{
    status->aAlg = ALG_BUBBLE;
    status->hiA = -1;
    status->hiB = -1;
}

//TODO init dans utils

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Status *status = status = calloc(1, sizeof(*status));
    if (!status) { perror("calloc status"); return 1; };
    init_Status(status);

    Graphisme gfx = {0};
    if(init_SDL(&gfx)) return 1;

    Array arr = {0};
    init_Array(&arr,status);

    reset_metrics();
    start_timer();
    render_array(&gfx, &arr, "", status);
    printf("Metrics.elapsed_ms : %f", getMetrics().elapsed_ms);
    status->bRunning = true;
    while (status->bRunning && !status->bAbort) {
        visual_tick(&gfx,&arr,status->hiA,status->hiA,"",status);

        if (status->bAbort)
            break;

        if (status->bSorting) {
            printf("sorting %d ", status->bSorting);
            fflush(stdout);
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
            SDL_Delay(DELAY_MS);
        }
    }

    free(arr.a);
    free(status);
    TTF_CloseFont(gfx.font);
    SDL_DestroyRenderer(gfx.render);
    SDL_DestroyWindow(gfx.window);
    if (gfx.img_texture) SDL_DestroyTexture(gfx.img_texture);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
