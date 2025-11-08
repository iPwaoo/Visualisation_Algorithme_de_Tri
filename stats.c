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

int getA(Array *arr, int i) {
    metrics.reads++;
    return arr->a[i];
}

void setA(Array *arr, int i, int v) {
    metrics.writes++;
    arr->a[i] = v;
}

int cmp_idx(Graphisme *Graphisme, Array *arr, int i, int j, Status *status)
{
    metrics.comps++;
    int vi = getA(arr, i), vj = getA(arr, j);
    visual_tick(Graphisme, arr, i, j, "Comparaison", status);
    if (vi < vj)
        return -1;
    if (vi > vj)
        return 1;
    return 0;
}
void swap_idx(Graphisme *Graphisme, Array *arr, int i, int j, Status *status)
{
    if (i == j)
        return;
    int tmp = getA(arr, i), vj = getA(arr, j);
    setA(arr, i, vj);
    setA(arr, j, tmp);
    metrics.swaps++;
    visual_tick(Graphisme, arr, i, j, "Echange", status);
}

void addComps()
{
    metrics.comps++;
}

Metrics getMetrics()
{
    return metrics;
}

void addFrame()
{
    metrics.frames++;
}
