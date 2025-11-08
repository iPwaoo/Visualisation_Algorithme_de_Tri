// visual.c
#include "visual.h"
#include "stats.h"
#include "common.h"
#include "utils.h"

// mesure largeur/hauteur d’un token (chaine UTF-8)
static Size measure_text(TTF_Font *font, const char *s)
{
    Size z = {0, 0};
    if (!s || !*s)
        return z;
    TTF_SizeUTF8(font, s, &z.width, &z.h);
    return z;
}

int init_SDL(Graphisme *gfx)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    // Init SDL_image
    int img_flags = IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    if ((img_flags & (IMG_INIT_PNG | IMG_INIT_JPG)) == 0)
    {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        // on continue quand même: mode couleur fallback
    }

    // Fenêtre + renderer d'abord !
    gfx->width = WIN_WIDTH;
    gfx->height = WIN_HEIGHT;
    gfx->window = SDL_CreateWindow("Sort Visualizer",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   gfx->width, gfx->height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!gfx->window)
    {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    gfx->render = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gfx->render)
    {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    gfx->font = TTF_OpenFont("Coolvetica Rg.otf", 16);
    if (!gfx->font)
    {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        return 1;
    }

    // Image optionnelle
    gfx->img_texture = NULL;
    gfx->img_width = gfx->img_height = 0;

    SDL_Surface *img_surface = IMG_Load("ressources/image.jpeg"); // si tu la mets plus tard, garde ce code
    if (!img_surface)
    {
        fprintf(stderr, "Warning: no image loaded (ressources/singe.jpeg). IMG_Load: %s\n", IMG_GetError());
        return 1;
    }

    gfx->img_texture = SDL_CreateTextureFromSurface(gfx->render, img_surface);
    if (!gfx->img_texture)
    {
        fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(img_surface);
        return 1;
    }
    gfx->img_width = img_surface->w;
    gfx->img_height = img_surface->h;
    SDL_FreeSurface(img_surface);

    return 0;
}

// Dessine les tokens alignés à gauche, en retour à la ligne si dépassement
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

// Dessine les tokens alignés à gauche, en retour à la ligne si dépassement
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

        // retour à la ligne ?
        if (cx != x && cx + TOKEN_SPACING_X + sz.width > x + max_w)
        {
            // nouvelle ligne
            cy += TOKEN_SPACING_Y + line_h;
            cx = x;
        }
        // si début de ligne, pas d’espace; sinon ajouter le spacing
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
            cx += sz.width; // avance
        }
    }
    if (out_bottom_y)
        *out_bottom_y = cy + line_h; // baseline de fin
}

void draw_bar(Graphisme *graph, Array *arr, int i, SDL_Rect barRect)
{
    if (graph->img_texture)
    {
        int n = arr->n;           // nombre total d'éléments
        int value = getA(arr, i); // valeur de la barre (sert à retrouver sa "part" d'image)

        // Calcule la largeur d'une "tranche" de l'image
        int slice_width = graph->img_width / n;

        // Détermine la portion de l'image correspondant à CETTE valeur
        SDL_Rect src;
        src.x = value * slice_width;
        src.w = slice_width;
        src.h = graph->img_height;

        // Si tu veux que la hauteur de l'image soit "coupée" selon la taille du rectangle :
        int visible_height = (int)((barRect.h / (float)WIN_HEIGHT) * graph->img_height);
        src.y = graph->img_height - visible_height;
        src.h = visible_height;

        // Destination à l'écran
        SDL_Rect dest = barRect;
        dest.y = WIN_HEIGHT - barRect.h; // bas aligné
        dest.h = barRect.h;

        SDL_RenderCopy(graph->render, graph->img_texture, &src, &dest);
    }
    else
    {
        // Fallback : couleur normale si pas d'image
        SDL_SetRenderDrawColor(graph->render, 255, 255, 255, 255);
        SDL_RenderFillRect(graph->render, &barRect);
    }
}

void render_array(Graphisme *gfx, Array *arr, const char *subtitle, Status *status)
{
    SDL_SetRenderDrawColor(gfx->render, 20, 22, 28, 255); // fond
    SDL_RenderClear(gfx->render);

    SDL_Color white = (SDL_Color){235, 235, 235, 255};

    // --- 1) Construire les tokens TOP (stats + infos)
    char tAlgo[32], tEtat[32], tN[24], tReads[32], tWrites[32], tComps[32], tSwaps[32], tTime[32], tFrames[24];
    snprintf(tAlgo, sizeof tAlgo, "Algorithme: %s", algo_name(status->aAlg));
    snprintf(tEtat, sizeof tEtat, "Etat: %s", state_name(status));
    snprintf(tN, sizeof tN, "N: %d", arr->n);
    snprintf(tReads, sizeof tReads, "Lectures: %llu", (unsigned long long)getMetrics().reads);
    snprintf(tWrites, sizeof tWrites, "Ecritures: %llu", (unsigned long long)getMetrics().writes);
    snprintf(tComps, sizeof tComps, "Comparaisons: %llu", (unsigned long long)getMetrics().comps);
    snprintf(tSwaps, sizeof tSwaps, "Swaps: %llu", (unsigned long long)getMetrics().swaps);
    snprintf(tTime, sizeof tTime, "Temps: %.1f ms", getMetrics().elapsed_ms);
    snprintf(tFrames, sizeof tFrames, "Frames: %llu", (unsigned long long)getMetrics().frames);

    const char *top_tokens[] = {tAlgo, tEtat, tN, tReads, tWrites, tComps, tSwaps, tTime, tFrames};
    int top_count = (int)(sizeof(top_tokens) / sizeof(top_tokens[0]));

    // Mesurer la hauteur occupée en haut
    int top_height = layout_tokens_height(gfx->font, top_tokens, top_count, gfx->width - 2 * SIDE_MARGIN);
    (void)top_height;
    // --- 2) Construire les tokens BOTTOM (contrôles)
    const char *bot_tokens[] = {
        "[ESPACE] Lancer / Pause / Reprendre",
        "[B] Bubble", "[S] Selection", "[I] Insertion", "[Q] Quick", "[M] Merge",
        "[R] Nouveau tableau",
        "[UP/DOWN] Taille",
        "[Esc] Quitter"};
    int bot_count = (int)(sizeof(bot_tokens) / sizeof(bot_tokens[0]));

    int bottom_height = layout_tokens_height(gfx->font, bot_tokens, bot_count, gfx->width - 2 * SIDE_MARGIN);

    // --- 3) Dessiner TOP (aligné à gauche, wrap)
    int after_top_y;
    draw_tokens_left_wrap(gfx->render, gfx->font, top_tokens, top_count,
                          SIDE_MARGIN, TOP_MARGIN, gfx->width - 2 * SIDE_MARGIN, white, &after_top_y);

    // --- 4) Dessiner BOTTOM (aligné à gauche, wrap) au-dessus du bord bas
    int bottom_start_y = gfx->height - bottom_height - BOTTOM_PAD;
    int after_bottom_y;
    draw_tokens_left_wrap(gfx->render, gfx->font, bot_tokens, bot_count,
                          SIDE_MARGIN, bottom_start_y, gfx->width - 2 * SIDE_MARGIN, white, &after_bottom_y);

    // --- 5) Zone centrale: barres
    int avail_top = after_top_y + BETWEEN_SECTIONS;
    int avail_bottom = bottom_start_y - BETWEEN_SECTIONS;
    int avail_h = avail_bottom - avail_top;
    if (avail_h < 40)
        avail_h = 40;

    // Ajuster maxVal pour coller à la zone (préserve proportions)
    arr->maxVal = avail_h - 8;
    if (arr->maxVal < 10)
        arr->maxVal = 10;

    int bw = (gfx->width) / arr->n;
    if (bw < 1)
        bw = 1;
    for (int i = 0; i < arr->n; ++i)
    {
        int h = (arr->a[i] * (avail_h - 2)) / (arr->maxVal > 0 ? arr->maxVal : 1);
        if (h < 1)
            h = 1;

        int x = i * bw;
        int y = avail_top + (avail_h - h);
        SDL_Rect r = {x, y, bw - 1, h};
        //TODO HAAAAAAAAAAAAAAA
        if (gfx->img_texture)
        {
            // --- MODE IMAGE ---
            int n = arr->n;
            int value = arr->a[i]; // supposé dans [0..n-1] si tableau de permutation
            if (value < 0)
                value = 0;
            if (value >= n)
                value = n - 1;

            // largeur d'une tranche théorique dans l’image
            // mais pour coller le rectangle, on projette sur la largeur réelle du rect
            int slice_x = (value * gfx->img_width) / n;
            int slice_w = ((value + 1) * gfx->img_width) / n - slice_x; // tranche exacte pour cette "colonne"
            if (slice_w <= 0)
                slice_w = 1;

            // Ajuster pour coller le dest : on mappe la tranche sur la largeur du rect
            SDL_Rect src;
            src.x = slice_x;
            src.w = slice_w;

            // hauteur visible proportionnelle à la hauteur de la barre
            // on "coupe" par le bas pour que les petites barres n'affichent que le bas de l'image
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
            // --- FALLBACK COULEUR ---
            if (i == status->hiA || i == status->hiB)
                SDL_SetRenderDrawColor(gfx->render, 230, 90, 90, 255);
            else
                SDL_SetRenderDrawColor(gfx->render, 90, 170, 255, 255);

            SDL_RenderFillRect(gfx->render, &r);
        }
    }

    // Sous-titre (ex: “Comparaison”, “Echange”, etc.), au-dessus de la zone bas
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
    if (status->bSorting) addFrame();
}

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
                    // Pause / reprise pendant le tri
                    status->bPaused = !status->bPaused;
                }
                else
                {
                    // Lancer ou relancer le tri
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
                random_array(arr);
                reset_metrics();
                status->bSorting = false;
                status->bSorted = false;
                status->bPaused = false;
                status->bAbort = false;
                status->bReseting = true;
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
                    if (arr->n < MAX_N)
                    {
                        arr->n += 5;
                        random_array(arr);
                        status->bSorted = false;
                        reset_metrics();
                    }
                    break;
                case SDLK_DOWN:
                    if (arr->n > MIN_N)
                    {
                        arr->n -= 5;
                        random_array(arr);
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

void visual_tick(Graphisme *gfx, Array *arr, int a, int b, const char *subtitle, Status *status)
{
    status->hiA = a;
    status->hiB = b;

    if (status->bAbort)
        return;

    if (status->bSorting)
    {
        update_time();
    }
    render_array(gfx, arr, subtitle, status);

    // Pause : on reste dans une boucle SDL non bloquante
    while (status->bPaused && !status->bAbort)
    {
        handle_key(gfx, arr, status);
        render_array(gfx, arr, "PAUSE", status);
        SDL_Delay(16);
    }
    handle_key(gfx, arr, status);

    SDL_Delay(DELAY_MS);
}

void apply_resize(Graphisme *Graphisme, Array *arr)
{
    SDL_GetWindowSize(Graphisme->window, &Graphisme->width, &Graphisme->height);
    // maxVal = hauteur dispo (approximative) pour les barres; ajustée plus finement à chaque rendu
    int approx_top = TOP_MARGIN + TTF_FontLineSkip(Graphisme->font) * 2;
    int approx_bot = TTF_FontLineSkip(Graphisme->font) * 2 + BOTTOM_PAD;
    arr->maxVal = Graphisme->height - approx_top - approx_bot - BETWEEN_SECTIONS * 2;
    if (arr->maxVal < 10)
        arr->maxVal = 10;
}
