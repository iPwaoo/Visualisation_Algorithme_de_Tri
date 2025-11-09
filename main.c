#include "common.h"
#include "utils.h"
#include "sorting.h"
#include "visual.h"
#include "stats.h"

static void init_status(Status *status)
{
    status->aAlg = ALG_BUBBLE;
    status->highlight_a = -1;
    status->highlight_b = -1;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char image_path[256];
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <image_file>\n", argv[0]);
        fprintf(stderr, "Example: %s singe.jpeg\n", argv[0]);
        snprintf(image_path, sizeof(image_path), "ressources/%s", URL_IMAGE);
    }
    else
    {
        snprintf(image_path, sizeof(image_path), "ressources/%s", argv[1]);
    }

    Status *status = calloc(1, sizeof(*status));
    if (!status)
    {
        perror("calloc status");
        return 1;
    }
    init_status(status);

    Graphisme gfx = {0};
    if (init_SDL(&gfx, image_path))
        return 1;

    Array arr = {0};
    init_array(&arr, status);

    reset_metrics();
    start_timer();
    render_array(&gfx, &arr, "", status);
    status->bRunning = true;
    while (status->bRunning || !status->bAbort)
    {
        if (status->bResetting)
        {
            status->bSorting = false;
            status->bSorted = false;
            update_time();
            render_array(&gfx, &arr, "RESET", status);
            status->bResetting = false;
            continue;
        }

        if (status->bSorting)
        {
            switch (status->aAlg)
            {
            case ALG_BUBBLE:
                sort_bubble(&gfx, &arr, status);
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
            default:
                fprintf(stderr, "Algorithme inconnu: %d\n", status->aAlg);
                break;
            }

            status->bSorting = false;

            if (!status->bAbort && !status->bResetting)
            {
                status->bSorted = true;
                update_time();
                visual_tick(&gfx, &arr, -1, -1, "Tri terminé", status);
            }
            else
            {
                status->bSorted = false;
                update_time();
                render_array(&gfx, &arr, "Tri interrompu", status);
            }
        }
        else
        {
            handle_key(&gfx, &arr, status);
            render_array(&gfx, &arr, "", status);
            SDL_Delay(DELAY_MS);
        }
    }

    free(arr.array);
    free(status);
    TTF_CloseFont(gfx.font);
    SDL_DestroyRenderer(gfx.render);
    SDL_DestroyWindow(gfx.window);
    if (gfx.img_texture)
        SDL_DestroyTexture(gfx.img_texture);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
