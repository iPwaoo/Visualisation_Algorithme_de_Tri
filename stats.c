// stats.c
#include "stats.h"
#include "visual.h"

Metrics metrics;
static uint64_t t0_ticks = 0;

void reset_metrics(void) {
    memset(&metrics, 0, sizeof(metrics));
}

void start_timer(void) {
    t0_ticks = SDL_GetPerformanceCounter();
}

void update_time(void) {
    uint64_t t = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();
    metrics.elapsed_ms = (double)(t - t0_ticks) * 1000.0 / freq;
}

int get_value(Array *arr, int i) {
    metrics.reads++;
    return arr->array[i];
}

void set_value(Array *arr, int i, int v) {
    metrics.writes++;
    arr->array[i] = v;
}

int compare_indices(Graphisme *Graphisme, Array *arr, int i, int j, Status *status)
{
    metrics.comps++;
    int vi = get_value(arr, i), vj = get_value(arr, j);
    visual_tick(Graphisme, arr, i, j, "Comparaison", status);
    if (vi < vj)
        return -1;
    if (vi > vj)
        return 1;
    return 0;
}
void swap_indices(Graphisme *Graphisme, Array *arr, int i, int j, Status *status)
{
    if (i == j)
        return;
    int tmp = get_value(arr, i), vj = get_value(arr, j);
    set_value(arr, i, vj);
    set_value(arr, j, tmp);
    metrics.swaps++;
    visual_tick(Graphisme, arr, i, j, "Echange", status);
}

void increment_comparisons()
{
    metrics.comps++;
}

Metrics get_metrics()
{
    return metrics;
}

void increment_frame()
{
    metrics.frames++;
}
