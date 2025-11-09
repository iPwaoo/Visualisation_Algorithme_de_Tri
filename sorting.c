// sorting.c
#include "sorting.h"
#include "stats.h"
#include "visual.h"
#include "utils.h"

void init_array(Array *arr, Status *status)
{
    arr->size = INIT_N;
    arr->array = malloc(MAX_N * sizeof(int));
    arr->max_value = arr->size;
    generate_random_array(arr);
    status->bSorted = false;
}

void sort_bubble(Graphisme *gfx, Array *arr, Status *status)
{
    for (int i = 0; i < arr->size - 1 && !status->bAbort && !status->bResetting; ++i)
    {
        bool swapped = false;
        for (int j = 0; j < arr->size - 1 - i && !status->bAbort && !status->bResetting; ++j)
        {
            if (compare_indices(gfx, arr, j, j + 1, status) > 0)
            {
                swap_indices(gfx, arr, j, j + 1, status);
                swapped = true;
            }
        }
        if (!swapped)
            break;
    }
    status->bResetting = false;
}
void sort_selection(Graphisme *Graphisme, Array *arr, Status *status)
{
    for (int i = 0; i < arr->size - 1 && !status->bAbort && !status->bResetting; ++i)
    {
        int minIdx = i;
        for (int j = i + 1; j < arr->size && !status->bAbort; ++j)
            if (compare_indices(Graphisme, arr, j, minIdx, status) < 0)
                minIdx = j;
        swap_indices(Graphisme, arr, i, minIdx, status);
    }
    status->bResetting = false;

}
void sort_insertion(Graphisme *Graphisme, Array *arr, Status *status)
{
    for (int i = 1; i < arr->size && !status->bAbort && !status->bResetting; ++i)
    {
        int key = get_value(arr, i);
        int j = i - 1;
        status->highlight_a = i;
        status->highlight_b = -1;
        visual_tick(Graphisme, arr, i, -1, "Insertion: element cle", status);
        while (j >= 0 && !status->bAbort)
        {
            increment_comparisons();
            int vj = get_value(arr, j);
            visual_tick(Graphisme, arr, j, j + 1, "Comparaison/decallage", status);
            if (vj > key)
            {
                set_value(arr, j + 1, vj);
                visual_tick(Graphisme, arr, j, j + 1, "Decalage", status);
                j--;
            }
            else
                break;
        }
        set_value(arr, j + 1, key);
        visual_tick(Graphisme, arr, j + 1, i, "Insertion cle", status);
    }
        status->bResetting = false;

}
static int partition(Graphisme *Graphisme, Array *arr, int low, int high, Status *status)
{
    int pivot = get_value(arr, high);
    int i = low - 1;

    for (int j = low; j <= high - 1 && !status->bAbort && !status->bResetting; ++j)
    {
        increment_comparisons();
        visual_tick(Graphisme, arr, j, high, "QS: comparer au pivot", status);
        int vj = get_value(arr, j);

        if (vj <= pivot)
        {
            i++;
            swap_indices(Graphisme, arr, i, j, status);
        }

        if (status->bAbort || status->bResetting)
            return i;
    }

    if (!status->bAbort && !status->bResetting)
        swap_indices(Graphisme, arr, i + 1, high, status);

    return i + 1;
}

static void quicksort(Graphisme *Graphisme, Array *arr, int low, int high, Status *status)
{
    if (status->bAbort || status->bResetting || low >= high)
        return;

    int pi = partition(Graphisme, arr, low, high, status);
    if (status->bAbort || status->bResetting)
        return;

    quicksort(Graphisme, arr, low, pi - 1, status);
    quicksort(Graphisme, arr, pi + 1, high, status);
}

void sort_quick(Graphisme *Graphisme, Array *arr, Status *status)
{
    quicksort(Graphisme, arr, 0, arr->size - 1, status);
    status->bResetting = false;

}

static void merge(Graphisme *Graphisme, Array *arr, int l, int m, int r, Status *status)
{
    int n1 = m - l + 1, n2 = r - m;
    int *L = malloc(n1 * sizeof(int)), *R = malloc(n2 * sizeof(int));
    for (int i = 0; i < n1; ++i)
        L[i] = get_value(arr, l + i);
    for (int j = 0; j < n2; ++j)
        R[j] = get_value(arr, m + 1 + j);

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2 && !status->bAbort && !status->bResetting)
    {
        increment_comparisons();
        visual_tick(Graphisme, arr, k, -1, "Merge: écriture", status);

        if (L[i] <= R[j])
            set_value(arr, k, L[i++]);
        else
            set_value(arr, k, R[j++]);

        k++;
    }

    while (i < n1 && !status->bAbort && !status->bResetting)
    {
        set_value(arr, k, L[i++]);
        visual_tick(Graphisme, arr, k, -1, "Merge: reste L", status);
        k++;
    }

    while (j < n2 && !status->bAbort && !status->bResetting)
    {
        set_value(arr, k, R[j++]);
        visual_tick(Graphisme, arr, k, -1, "Merge: reste R", status);
        k++;
    }

    free(L);
    free(R);
}

void mergesort(Graphisme *Graphisme, Array *arr, int l, int r, Status *status)
{
    if (status->bAbort || status->bResetting || l >= r)
        return;

    int m = l + (r - l) / 2;
    mergesort(Graphisme, arr, l, m, status);
    if (status->bAbort || status->bResetting)
        return;

    mergesort(Graphisme, arr, m + 1, r, status);
    if (!status->bAbort && !status->bResetting)
        merge(Graphisme, arr, l, m, r, status);
}

void sort_merge(Graphisme *Graphisme, Array *arr, Status *status)
{
    mergesort(Graphisme, arr, 0, arr->size - 1, status);
    status->bResetting = false;

}
