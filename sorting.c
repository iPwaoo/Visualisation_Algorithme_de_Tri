// sorting.c
#include "sorting.h"
#include "stats.h"
#include "visual.h"
#include "utils.h"

void init_Array(Array *arr, Status *status)
{
    arr->n = INIT_N;
    arr->a = malloc(MAX_N * sizeof(int));
    arr->maxVal = arr->n;
    random_array(arr);
    status->bSorted = false;
}

void sort_bubble(Graphisme *gfx, Array *arr, Status *status)
{
    for (int i = 0; i < arr->n - 1 && !status->bAbort && !status->bReseting; ++i)
    {
        bool swapped = false;
        for (int j = 0; j < arr->n - 1 - i && !status->bAbort && !status->bReseting; ++j)
        {
            if (cmp_idx(gfx, arr, j, j + 1, status) > 0)
            {
                swap_idx(gfx, arr, j, j + 1, status);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
    status->bReseting = false;
}
void sort_selection(Graphisme *Graphisme, Array *arr, Status *status)
{
    for (int i = 0; i < arr->n - 1 && !status->bAbort && !status->bReseting; ++i)
    {
        int minIdx = i;
        for (int j = i + 1; j < arr->n && !status->bAbort; ++j)
            if (cmp_idx(Graphisme, arr, j, minIdx, status) < 0)
                minIdx = j;
        swap_idx(Graphisme, arr, i, minIdx, status);
    }
}
void sort_insertion(Graphisme *Graphisme, Array *arr, Status *status)
{
    for (int i = 1; i < arr->n && !status->bAbort && !status->bReseting; ++i)
    {
        int key = getA(arr, i);
        int j = i - 1;
        status->hiA = i;
        status->hiB = -1;
        visual_tick(Graphisme, arr, i, -1, "Insertion: element cle",status);
        while (j >= 0 && !status->bAbort)
        {
            addComps();
            int vj = getA(arr, j);
            visual_tick(Graphisme, arr, j, j + 1, "Comparaison/decallage",status);
            if (vj > key)
            {
                setA(arr, j + 1, vj);
                visual_tick(Graphisme, arr, j, j + 1, "Decalage",status);
                j--;
            }
            else
                break;
        }
        setA(arr, j + 1, key);
        visual_tick(Graphisme, arr, j + 1, i, "Insertion cle",status);
    }
}
static int partition(Graphisme *Graphisme, Array *arr, int low, int high, Status *status)
{
    int pivot = getA(arr, high);
    int i = low - 1;
    for (int j = low; j <= high - 1 && !status->bAbort; ++j)
    {
        addComps();
        visual_tick(Graphisme, arr, j, high, "QS: comparer au pivot",status);
        int vj = getA(arr, j);
        if (vj <= pivot)
        {
            i++;
            swap_idx(Graphisme, arr, i, j,status);
        }
    }
    if (!status->bAbort)
        swap_idx(Graphisme, arr, i + 1, high, status);
    return i + 1;
}
static void quicksort(Graphisme *Graphisme, Array *arr, int low, int high, Status *status)
{
    if (status->bAbort || low >= high)
        return;
    int pi = partition(Graphisme, arr, low, high, status);
    if (status->bAbort)
        return;
    quicksort(Graphisme, arr, low, pi - 1,status);
    quicksort(Graphisme, arr, pi + 1, high,status);
}
void sort_quick(Graphisme *Graphisme, Array *arr, Status *status) { quicksort(Graphisme, arr, 0, arr->n - 1, status); }
static void merge(Graphisme *Graphisme, Array *arr, int l, int m, int r, Status *status)
{
    int n1 = m - l + 1, n2 = r - m;
    int *L = malloc(n1 * sizeof(int)), *R = malloc(n2 * sizeof(int));
    for (int i = 0; i < n1; ++i)
        L[i] = getA(arr, l + i);
    for (int j = 0; j < n2; ++j)
        R[j] = getA(arr, m + 1 + j);
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2 && !status->bAbort)
    {
        addComps();
        visual_tick(Graphisme, arr, k, -1, "Merge: ecriture", status);
        if (L[i] <= R[j])
        {
            setA(arr, k, L[i]);
            i++;
        }
        else
        {
            setA(arr, k, R[j]);
            j++;
        }
        k++;
    }
    while (i < n1 && !status->bAbort)
    {
        setA(arr, k, L[i]);
        visual_tick(Graphisme, arr, k, -1, "Merge: reste L", status);
        i++;
        k++;
    }
    while (j < n2 && !status->bAbort)
    {
        setA(arr, k, R[j]);
        visual_tick(Graphisme, arr, k, -1, "Merge: reste R", status);
        j++;
        k++;
    }
    free(L);
    free(R);
}
void mergesort(Graphisme *Graphisme, Array *arr, int l, int r, Status *status)
{
    if (status->bAbort || l >= r)
        return;
    int m = l + (r - l) / 2;
    mergesort(Graphisme, arr, l, m, status);
    mergesort(Graphisme, arr, m + 1, r, status);
    if (!status->bAbort)
        merge(Graphisme, arr, l, m, r, status);
}
void sort_merge(Graphisme *Graphisme, Array *arr, Status *status) { mergesort(Graphisme, arr, 0, arr->n - 1, status); }

/* idem: copy your insertion, selection, quick, merge from original main.c here */
