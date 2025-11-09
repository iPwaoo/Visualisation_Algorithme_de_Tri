// stats.c
#include "stats.h"
#include "visual.h"

Metrics metrics;
static uint64_t t0_ticks = 0;
static uint64_t pause_ticks = 0;
static double accumulated_ms = 0.0;
static bool is_paused = false;

/*
 * Function: reset_metrics
 * -----------------------
 * Reset all metric counters and timing state to their initial values.
 *
 * This clears reads/writes/comps/swaps/frames and resets the internal
 * performance counter bookkeeping so a new timed session can start cleanly.
 */
void reset_metrics(void)
{
    memset(&metrics, 0, sizeof(metrics));
    t0_ticks = 0;
    pause_ticks = 0;
    accumulated_ms = 0.0;
    is_paused = false;
}

/*
 * Function: start_timer
 * ---------------------
 * Start a new timing session for measuring elapsed milliseconds.
 *
 * Behavior:
 * - Records current performance counter as t0_ticks.
 * - Clears any previous accumulated time and pause markers.
 *
 * Uses SDL_GetPerformanceCounter / SDL_GetPerformanceFrequency to compute ms.
 */
void start_timer(void)
{
    t0_ticks = SDL_GetPerformanceCounter();
    pause_ticks = 0;
    accumulated_ms = 0.0;
    is_paused = false;
}

/*
 * Function: pause_timer
 * ---------------------
 * Mark the timer as paused.
 *
 * If not already paused, records the current performance counter in pause_ticks.
 * The elapsed time between t0_ticks and pause_ticks will be added to accumulated_ms
 * when resume_timer is called.
 */
void pause_timer(void)
{
    if (!is_paused)
    {
        pause_ticks = SDL_GetPerformanceCounter();
        is_paused = true;
    }
}

/*
 * Function: resume_timer
 * ----------------------
 * Resume timing after a pause.
 *
 * If the timer was paused, this function:
 * - Adds the time elapsed between t0_ticks and pause_ticks to accumulated_ms (in ms),
 *   which preserves the elapsed time before the pause.
 * - Sets t0_ticks to the current performance counter so new elapsed time is measured
 *   from now on.
 * - Clears the paused flag.
 *
 * Uses SDL_GetPerformanceFrequency to convert counter ticks into milliseconds.
 */
void resume_timer(void)
{
    if (is_paused)
    {
        uint64_t now = SDL_GetPerformanceCounter();
        double freq = (double)SDL_GetPerformanceFrequency();
        accumulated_ms += (double)(pause_ticks - t0_ticks) * 1000.0 / freq;
        t0_ticks = now;
        is_paused = false;
    }
}

/*
 * Function: update_time
 * ---------------------
 * Update the metrics.elapsed_ms field with the current elapsed time in milliseconds.
 *
 * If the timer is paused, no update is performed. Otherwise, elapsed_ms is computed as:
 *   accumulated_ms + (now - t0_ticks) * 1000.0 / frequency
 *
 * This ensures that paused intervals are not counted.
 */
void update_time(void)
{
    if (is_paused)
        return;

    uint64_t now = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();
    metrics.elapsed_ms = accumulated_ms + (double)(now - t0_ticks) * 1000.0 / freq;
}

/*
 * Function: get_value
 * -------------------
 * Read a value from the array at index i and increment the read counter.
 *
 * Returns the integer stored at arr->array[i].
 */
int get_value(Array *arr, int i)
{
    metrics.reads++;
    return arr->array[i];
}

/*
 * Function: set_value
 * -------------------
 * Write a value v into the array at index i and increment the write counter.
 */
void set_value(Array *arr, int i, int v)
{
    metrics.writes++;
    arr->array[i] = v;
}

/*
 * Function: compare_indices
 * -------------------------
 * Compare values at indices i and j, increment comparison counter and trigger a visual tick.
 *
 * Returns:
 *  -1 if arr[i] < arr[j]
 *   0 if equal
 *  +1 if arr[i] > arr[j]
 */
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

/*
 * Function: swap_indices
 * ----------------------
 * Swap two elements in the array (i and j), update metrics and trigger a visual tick.
 *
 * If i == j, no action is done.
 */
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

/*
 * Function: increment_comparisons
 * -------------------------------
 * Convenience helper to increment the comparison counter by one.
 */
void increment_comparisons()
{
    metrics.comps++;
}

/*
 * Function: get_metrics
 * ---------------------
 * Return the current Metrics struct by value (reads/writes/comps/swaps/frames/elapsed_ms).
 */
Metrics get_metrics()
{
    return metrics;
}

/*
 * Function: increment_frame
 * -------------------------
 * Increment the rendered frame counter (used to measure FPS or frames progressed).
 */
void increment_frame()
{
    metrics.frames++;
}
