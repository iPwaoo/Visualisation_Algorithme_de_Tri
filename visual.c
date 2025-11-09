// visual.c
#include "visual.h"
#include "stats.h"
#include "common.h"
#include "utils.h"

/*
 * Function: measure_text
 * ----------------------
 * Measure the rendered width and height of a UTF-8 string using a TTF font.
 *
 * font: pointer to an opened TTF_Font
 * s: UTF-8 C-string to measure
 *
 * returns: Size struct { width, height } with measured pixel dimensions.
 */
static Size measure_text(TTF_Font *font, const char *s)
{
    Size z = {0, 0};
    if (!s || !*s)
        return z;
    TTF_SizeUTF8(font, s, &z.width, &z.height);
    return z;
}

/*
 * Function: init_SDL
 * ------------------
 * Initialize SDL, TTF and IMG subsystems, create window, renderer and optionally load an image.
 *
 * gfx: pointer to Graphisme struct to populate (window, renderer, font, image texture, sizes).
 *
 * returns: 0 on success, non-zero on failure (and prints an error).
 */
int init_SDL(Graphisme *gfx, const char *image_path)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    int img_flags = IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    if ((img_flags & (IMG_INIT_PNG | IMG_INIT_JPG)) == 0) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    FILE *f = fopen(image_path, "r");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d’ouvrir le fichier image '%s'\n", image_path);
        perror("fopen");
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    fclose(f);

    gfx->width = WIN_WIDTH;
    gfx->height = WIN_HEIGHT;
    gfx->window = SDL_CreateWindow("Sort Visualizer",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   gfx->width, gfx->height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!gfx->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    gfx->render = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gfx->render) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(gfx->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    gfx->font = TTF_OpenFont("Coolvetica Rg.otf", 16);
    if (!gfx->font) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(gfx->render);
        SDL_DestroyWindow(gfx->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface *img_surface = IMG_Load(image_path);
    if (!img_surface) {
        fprintf(stderr, "IMG_Load failed for '%s': %s\n", image_path, IMG_GetError());
        TTF_CloseFont(gfx->font);
        SDL_DestroyRenderer(gfx->render);
        SDL_DestroyWindow(gfx->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    gfx->img_texture = SDL_CreateTextureFromSurface(gfx->render, img_surface);
    if (!gfx->img_texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(img_surface);
        TTF_CloseFont(gfx->font);
        SDL_DestroyRenderer(gfx->render);
        SDL_DestroyWindow(gfx->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    gfx->img_width = img_surface->w;
    gfx->img_height = img_surface->h;
    SDL_FreeSurface(img_surface);

    return 0;
}


/*
 * Function: layout_tokens_height
 * ------------------------------
 * Compute the vertical space (height) needed to layout a list of tokens (strings) with wrapping.
 *
 * font: font used to measure tokens
 * tokens: array of C-strings
 * count: number of tokens
 * max_w: maximum width available for a line (in pixels)
 *
 * returns: total height in pixels needed for the wrapped tokens block.
 */
static int layout_tokens_height(TTF_Font *font, const char **tokens, int count, int max_w)
{
    int line_w = 0, line_h = TTF_FontLineSkip(font), total_h = line_h;
    for (int i = 0; i < count; ++i)
    {
        Size sz = measure_text(font, tokens[i]);
        if (i == 0)
        {
            line_w = sz.width;
        }
        else
        {
            if (line_w + TOKEN_SPACING_X + sz.width > max_w)
            {
                total_h += TOKEN_SPACING_Y + line_h;
                line_w = sz.width;
            }
            else
            {
                line_w += TOKEN_SPACING_X + sz.width;
            }
        }
    }
    return total_h;
}

/*
 * Function: draw_tokens_left_wrap
 * -------------------------------
 * Draw a list of tokens, left-aligned and wrapped when reaching max width.
 *
 * render: SDL_Renderer to draw onto
 * font: TTF_Font used for rendering
 * tokens/count: tokens to draw
 * x,y: top-left origin where drawing starts
 * max_w: maximum width allowed for wrapping
 * col: text color
 * out_bottom_y: optional out parameter receiving the baseline of the last line drawn
 */
static void draw_tokens_left_wrap(SDL_Renderer *render, TTF_Font *font, const char **tokens, int count,
                                  int x, int y, int max_w, SDL_Color col, int *out_bottom_y)
{
    int cx = x, cy = y;
    int line_h = TTF_FontLineSkip(font);

    for (int i = 0; i < count; ++i)
    {
        const char *txt = tokens[i];
        if (!txt || !*txt)
            continue;
        Size sz = measure_text(font, txt);

        if (cx != x && cx + TOKEN_SPACING_X + sz.width > x + max_w)
        {
            cy += TOKEN_SPACING_Y + line_h;
            cx = x;
        }
        if (cx != x)
            cx += TOKEN_SPACING_X;

        SDL_Color c = col;
        SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, c);
        if (s)
        {
            SDL_Texture *t = SDL_CreateTextureFromSurface(render, s);
            SDL_Rect dst = {cx, cy + (line_h - s->h) / 2, s->w, s->h};
            SDL_RenderCopy(render, t, NULL, &dst);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
            cx += sz.width; 
        }
    }
    if (out_bottom_y)
        *out_bottom_y = cy + line_h; 
}

/*
 * Function: draw_bar
 * ------------------
 * Draw a single bar of the array visualization. If an image texture is loaded,
 * paint a vertical slice of the image mapped to the bar. Otherwise, fill with color.
 *
 * graph: Graphisme context containing renderer and image texture info
 * arr: array object providing values
 * i: index of the bar to draw
 * barRect: destination rectangle on screen for the bar
 */
void draw_bar(Graphisme *graph, Array *arr, int i, SDL_Rect barRect)
{
    if (graph->img_texture)
    {
        int n = arr->size;          
        int value = get_value(arr, i); 

        int slice_width = graph->img_width / n;

        SDL_Rect src;
        src.x = value * slice_width;
        src.w = slice_width;
        src.h = graph->img_height;

        int visible_height = (int)((barRect.h / (float)WIN_HEIGHT) * graph->img_height);
        src.y = graph->img_height - visible_height;
        src.h = visible_height;

        SDL_Rect dest = barRect;
        dest.y = WIN_HEIGHT - barRect.h;
        dest.h = barRect.h;

        SDL_RenderCopy(graph->render, graph->img_texture, &src, &dest);
    }
    else
    {
        SDL_SetRenderDrawColor(graph->render, 255, 255, 255, 255);
        SDL_RenderFillRect(graph->render, &barRect);
    }
}

/*
 * Function: render_array
 * ----------------------
 * Render the full visualization frame: background, top tokens, bottom controls,
 * the array bars (image or color), subtitle, and present the frame.
 *
 * gfx: Graphisme context (renderer, font, image)
 * arr: array data to visualize
 * subtitle: optional subtitle text (e.g. "Compare", "Swap")
 * status: status struct that drives highlights, sorting state and metrics display
 */
void render_array(Graphisme *gfx, Array *arr, const char *subtitle, Status *status)
{
    SDL_SetRenderDrawColor(gfx->render, 20, 22, 28, 255);
    SDL_RenderClear(gfx->render);

    SDL_Color white = (SDL_Color){235, 235, 235, 255};

    char tAlgo[32], tEtat[32], tN[24], tReads[32], tWrites[32], tComps[32], tSwaps[32], tTime[32], tFrames[24];
    snprintf(tAlgo, sizeof tAlgo, "Algorithme: %s", get_algorithm_name(status->aAlg));
    snprintf(tEtat, sizeof tEtat, "Etat: %s", get_state_name(status));
    snprintf(tN, sizeof tN, "N: %d", arr->size);
    snprintf(tReads, sizeof tReads, "Lectures: %llu", (unsigned long long)get_metrics().reads);
    snprintf(tWrites, sizeof tWrites, "Ecritures: %llu", (unsigned long long)get_metrics().writes);
    snprintf(tComps, sizeof tComps, "Comparaisons: %llu", (unsigned long long)get_metrics().comps);
    snprintf(tSwaps, sizeof tSwaps, "Swaps: %llu", (unsigned long long)get_metrics().swaps);
    snprintf(tTime, sizeof tTime, "Temps: %.1f ms", get_metrics().elapsed_ms);
    snprintf(tFrames, sizeof tFrames, "Frames: %llu", (unsigned long long)get_metrics().frames);

    const char *top_tokens[] = {tAlgo, tEtat, tN, tReads, tWrites, tComps, tSwaps, tTime, tFrames};
    int top_count = (int)(sizeof(top_tokens) / sizeof(top_tokens[0]));

    int top_height = layout_tokens_height(gfx->font, top_tokens, top_count, gfx->width - 2 * SIDE_MARGIN);
    (void)top_height;

    const char *bot_tokens[] = {
        "[ESPACE] Lancer / Pause / Reprendre",
        "[B] Bubble", "[S] Selection", "[I] Insertion", "[Q] Quick", "[M] Merge",
        "[R] Nouveau tableau",
        "[UP/DOWN] Taille",
        "[Esc] Quitter"};
    int bot_count = (int)(sizeof(bot_tokens) / sizeof(bot_tokens[0]));

    int bottom_height = layout_tokens_height(gfx->font, bot_tokens, bot_count, gfx->width - 2 * SIDE_MARGIN);

    int after_top_y;
    draw_tokens_left_wrap(gfx->render, gfx->font, top_tokens, top_count,
                          SIDE_MARGIN, TOP_MARGIN, gfx->width - 2 * SIDE_MARGIN, white, &after_top_y);

    int bottom_start_y = gfx->height - bottom_height - BOTTOM_PAD;
    int after_bottom_y;
    draw_tokens_left_wrap(gfx->render, gfx->font, bot_tokens, bot_count,
                          SIDE_MARGIN, bottom_start_y, gfx->width - 2 * SIDE_MARGIN, white, &after_bottom_y);

    int avail_top = after_top_y + BETWEEN_SECTIONS;
    int avail_bottom = bottom_start_y - BETWEEN_SECTIONS;
    int avail_h = avail_bottom - avail_top;
    if (avail_h < 40)
        avail_h = 40;

    arr->max_value = avail_h - 8;
    if (arr->max_value < 10)
        arr->max_value = 10;

    int bw = (gfx->width) / arr->size;
    if (bw < 1)
        bw = 1;
    for (int i = 0; i < arr->size; ++i)
    {
        int h = (arr->array[i] * (avail_h - 2)) / (arr->max_value > 0 ? arr->max_value : 1);
        if (h < 1)
            h = 1;

        int x = i * bw;
        int y = avail_top + (avail_h - h);
        SDL_Rect r = {x, y, bw - 1, h};
        if (gfx->img_texture)
        {
            int n = arr->size;
            int value = arr->array[i]; 
            if (value < 0)
                value = 0;
            if (value >= n)
                value = n - 1;

            int slice_x = (value * gfx->img_width) / n;
            int slice_w = ((value + 1) * gfx->img_width) / n - slice_x;
            if (slice_w <= 0)
                slice_w = 1;

            
            SDL_Rect src;
            src.x = slice_x;
            src.w = slice_w;


            int visible_h = (int)((r.h / (float)avail_h) * gfx->img_height);
            if (visible_h < 1)
                visible_h = 1;
            if (visible_h > gfx->img_height)
                visible_h = gfx->img_height;

            src.y = gfx->img_height - visible_h;
            src.h = visible_h;

            SDL_RenderCopy(gfx->render, gfx->img_texture, &src, &r);
        }
        else
        {
            if (i == status->highlight_a || i == status->highlight_b)
                SDL_SetRenderDrawColor(gfx->render, 230, 90, 90, 255);
            else
                SDL_SetRenderDrawColor(gfx->render, 90, 170, 255, 255);

            SDL_RenderFillRect(gfx->render, &r);
        }
    }

    if (subtitle && *subtitle)
    {
        SDL_Surface *s = TTF_RenderUTF8_Blended(gfx->font, subtitle, white);
        if (s)
        {
            SDL_Texture *t = SDL_CreateTextureFromSurface(gfx->render, s);
            int y = bottom_start_y - 8 - s->h;
            SDL_Rect dst = {SIDE_MARGIN, y, s->w, s->h};
            if (dst.y > avail_top)
                SDL_RenderCopy(gfx->render, t, NULL, &dst);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }
    }

    SDL_RenderPresent(gfx->render);
    if (status->bSorting) increment_frame();
}

/*
 * Function: handle_key
 * --------------------
 * Poll and handle SDL events (keyboard, window, quit). Update status and array
 * accordingly (start/pause sorting, change algorithm, resize, generate new array).
 *
 * gfx: Graphisme context used for rendering or resizing calls
 * arr: array object to modify on commands
 * status: status object updated by keyboard events
 */
void handle_key(Graphisme *gfx, Array *arr, Status *status)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        print_keyboard_event(&e.key);
        switch (e.type)
        {
        case SDL_QUIT:
            status->bAbort = true;
            status->bSorting = false;
            break;
        case SDL_KEYUP:

            switch (e.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                status->bRunning = false;
                status->bAbort = true;
                break;
            case SDLK_SPACE:
                printf("handle_key space");
                fflush(stdout);
                if (status->bSorting)
                {
                    status->bPaused = !status->bPaused;
                }
                else
                {
                    status->bSorting = true;
                    status->bPaused = false;
                    status->bSorted = false;
                    status->bAbort = false;
                    reset_metrics();
                    start_timer();
                }
                break;
                print_keyboard_event(&e.key);
                break;

            case SDLK_r:
                generate_random_array(arr);
                reset_metrics();
                status->bSorting = false;
                status->bSorted = false;
                status->bPaused = false;
                status->bAbort = false;
                status->bResetting = true;
                render_array(gfx, arr, "", status);
                break;

            case SDLK_b:
                status->aAlg = ALG_BUBBLE;
                break;
            case SDLK_s:
                status->aAlg = ALG_SELECTION;
                break;
            case SDLK_i:
                status->aAlg = ALG_INSERTION;
                break;
            case SDLK_q:
                status->aAlg = ALG_QUICK;
                break;
            case SDLK_m:
                status->aAlg = ALG_MERGE;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYDOWN:
            if (!status->bSorting)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_UP:
                    if (arr->size < MAX_N)
                    {
                        arr->size += 5;
                        generate_random_array(arr);
                        status->bSorted = false;
                        reset_metrics();
                    }
                    break;
                case SDLK_DOWN:
                    if (arr->size > MIN_N)
                    {
                        arr->size -= 5;
                        generate_random_array(arr);
                        status->bSorted = false;
                        reset_metrics();
                    }
                default:
                    break;
                }
            }

            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                apply_resize(gfx, arr);
                render_array(gfx, arr, "", status);
            }
            break;
        default:
            break;
            SDL_Delay(DELAY_MS);
        }
    }
}

/*
 * Function: visual_tick
 * ---------------------
 * Called frequently by sorting algorithms to update highlights, render a frame,
 * apply pause behavior and handle user input.
 *
 * gfx: Graphisme context
 * arr: array being sorted
 * a,b: indices currently highlighted by the algorithm
 * subtitle: descriptive text for the current operation
 * status: status flags that control flow (paused, abort, sorting)
 */
void visual_tick(Graphisme *gfx, Array *arr, int a, int b, const char *subtitle, Status *status)
{
    status->highlight_a = a;
    status->highlight_b = b;

    if (status->bAbort)
        return;

    if (status->bSorting)
    {
        update_time();
    }
    render_array(gfx, arr, subtitle, status);

    while (status->bPaused && !status->bAbort)
    {
        handle_key(gfx, arr, status);
        render_array(gfx, arr, "PAUSE", status);
        SDL_Delay(16);
    }
    handle_key(gfx, arr, status);

    SDL_Delay(DELAY_MS);
}

/*
 * Function: apply_resize
 * ----------------------
 * Update stored window dimensions and compute an approximate max value for bars
 * based on new window height so subsequent renders adapt to size changes.
 *
 * Graphisme: Graphisme context whose window size is read and width/height updated
 * arr: array whose max_value is adjusted to match available vertical space
 */
void apply_resize(Graphisme *Graphisme, Array *arr)
{
    SDL_GetWindowSize(Graphisme->window, &Graphisme->width, &Graphisme->height);
    int approx_top = TOP_MARGIN + TTF_FontLineSkip(Graphisme->font) * 2;
    int approx_bot = TTF_FontLineSkip(Graphisme->font) * 2 + BOTTOM_PAD;
    arr->max_value = Graphisme->height - approx_top - approx_bot - BETWEEN_SECTIONS * 2;
    if (arr->max_value < 10)
        arr->max_value = 10;
}
