// visual.c
#include "visual.h"
#include "stats.h"
#include "common.h"
#include "utils.h"

// --- Rainbow: conversion HSV -> RGB ---
static void hsv_to_rgb(float h, float s, float v, Uint8 *r, Uint8 *g, Uint8 *b)
{
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1, g1, b1;

    if (h < 60)
        r1 = c, g1 = x, b1 = 0;
    else if (h < 120)
        r1 = x, g1 = c, b1 = 0;
    else if (h < 180)
        r1 = 0, g1 = c, b1 = x;
    else if (h < 240)
        r1 = 0, g1 = x, b1 = c;
    else if (h < 300)
        r1 = x, g1 = 0, b1 = c;
    else
        r1 = c, g1 = 0, b1 = x;

    *r = (Uint8)((r1 + m) * 255);
    *g = (Uint8)((g1 + m) * 255);
    *b = (Uint8)((b1 + m) * 255);
}

// mesure largeur/hauteur d’un token (chaine UTF-8)
static Size measure_text(TTF_Font *font, const char *s)
{
    Size z = {0, 0};
    if (!s || !*s)
        return z;
    TTF_SizeUTF8(font, s, &z.width, &z.h);
    return z;
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
        "[C] Annuler",
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
        if (i == status->hiA || i == status->hiB)
            SDL_SetRenderDrawColor(gfx->render, 230, 90, 90, 255);
        else
            SDL_SetRenderDrawColor(gfx->render, 90, 170, 255, 255);
        SDL_Rect r = {x, y, bw - 1, h};
        SDL_RenderFillRect(gfx->render, &r);
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
    addFrame();
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

            case SDLK_c:
                if (status->bSorting)
                {
                    status->bAbort = true;
                    status->bSorting = false;
                    status->bPaused = false;
                }
                break;
            case SDLK_r:
                random_array(arr);
                reset_metrics();
                status->bSorting = false;
                status->bSorted = false;
                status->bPaused = false;
                status->bAbort = false;
                status->bReseting = true;
                render_array(gfx, arr, "", status); // redessine immédiatement
                break;
                /**
                 * case SDLK_R:

                status->bReseting = true;
                random_array(arr);
                reset_metrics();
                status->bSorting = false;
                status->bSorted = false;
                status->bPaused = true;
                break;
                 */

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
            break;
        case SDL_WINDOWEVENT &&SDL_WINDOWEVENT_SIZE_CHANGED:
            apply_resize(gfx, arr);
            render_array(gfx, arr, status->bSorting ? (status->bPaused ? "PAUSE" : "EN COURS") : "Pret.", status);
            break;
        default:
            break;
            SDL_Delay(16);
        }
    }
}

void visual_tick(Graphisme *gfx, Array *arr, int a, int b, const char *subtitle, Status *status)
{
    status->hiA = a;
    status->hiB = b;

    if (status->bAbort)
        return;

    update_time();
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
