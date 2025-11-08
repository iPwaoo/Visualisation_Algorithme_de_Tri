// sorting.h
#ifndef SORTING_H
#define SORTING_H
#include "common.h"

void init_Array(Array *arr, Status *status);
void sort_bubble(Graphisme *gfx, Array *arr, Status *status);
void sort_selection(Graphisme *gfx, Array *arr, Status *status);
void sort_insertion(Graphisme *gfx, Array *arr, Status *status);
void sort_quick(Graphisme *gfx, Array *arr, Status *status);
void sort_merge(Graphisme *gfx, Array *arr, Status *status);

#endif
