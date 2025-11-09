// stats.h
#ifndef STATS_H
#define STATS_H
#include "common.h"

extern Metrics metrics;

void reset_metrics(void);
void start_timer(void);
void update_time(void);
int get_value(Array *arr, int i);
void set_value(Array *arr, int i, int v);
int compare_indices(Graphisme *Graphisme, Array *arr, int i, int j, Status *status);
void swap_indices(Graphisme *Graphisme, Array *arr, int i, int j, Status *status);
void increment_comparisons();
Metrics get_metrics();
void increment_frame();


#endif
