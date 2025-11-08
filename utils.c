// utils.c
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>   

const char *algo_name(Algorithm a)
{
    switch (a)
    {
    case ALG_BUBBLE:
        return "Bubble";
    case ALG_SELECTION:
        return "Selection";
    case ALG_INSERTION:
        return "Insertion";
    case ALG_QUICK:
        return "QuickSort";
    case ALG_MERGE:
        return "MergeSort";
    default:
        return "?";
    }
}

const char *state_name(Status *status)
{
    if (status->bSorting)
        return status->bPaused ? "PAUSE" : "EN COURS";
    return status->bSorted ? "TERMINE" : "PRET";
}

void random_array(Array *arr)
{
    // Sécurité : borne haute
    if (arr->n > MAX_N)
        arr->n = MAX_N;

    // Fixer la valeur max au nombre d'éléments
    arr->maxVal = arr->n;

    // Remplir le tableau avec une suite régulière : 1, 2, 3, ..., n
    for (int i = 0; i < arr->n; ++i)
        arr->a[i] = i + 1;

    // Mélange de Fisher-Yates pour obtenir une permutation aléatoire
    for (int i = arr->n - 1; i > 0; --i)
    {
        int j = rand() % (i + 1);
        int tmp = arr->a[i];
        arr->a[i] = arr->a[j];
        arr->a[j] = tmp;
    }
}


void print_status(Status *st)
{
    if (!st) {
        printf("Status = NULL\n");
        return;
    }

    printf("=== STATUS ===\n");
    printf("Algo       : %s\n", algo_name(st->aAlg));
    printf("Sorting    : %s\n", st->bSorting ? "true" : "false");
    printf("Paused     : %s\n", st->bPaused ? "true" : "false");
    printf("Sorted     : %s\n", st->bSorted ? "true" : "false");
    printf("Abort      : %s\n", st->bAbort ? "true" : "false");
    printf("Reseting      : %s\n", st->bReseting ? "true" : "false");
    printf("hiA, hiB   : %d, %d\n", st->hiA, st->hiB);
    printf("================\n");
}

void print_keyboard_event(SDL_KeyboardEvent *ev)
{
    if (!ev) return;

    const char *type =
        (ev->type == SDL_KEYDOWN) ? "KEYDOWN" :
        (ev->type == SDL_KEYUP)   ? "KEYUP"   : "UNKNOWN";

    const char *state =
        (ev->state == SDL_PRESSED)  ? "PRESSED"  :
        (ev->state == SDL_RELEASED) ? "RELEASED" : "???";

    printf("---- SDL Keyboard Event ----\n");
    printf(" Type:      %s (%"PRIu32") \n", type, ev->type);
    printf(" Timestamp: %u ms\n", ev->timestamp);
    printf(" WindowID:  %u\n", ev->windowID);
    printf(" State:     %s (%hhu)\n", state, ev->state);
    printf(" Repeat:    %u\n", ev->repeat);
    printf(" Keysym:\n");
    printf("   Scancode: %d\n", ev->keysym.scancode);
    printf("   Sym:      %d (%s)\n", ev->keysym.sym, SDL_GetKeyName(ev->keysym.sym));
    printf("   Mod:      0x%x\n", ev->keysym.mod);
    printf("----------------------------\n");
}

