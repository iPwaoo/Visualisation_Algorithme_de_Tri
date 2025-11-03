// stats.h
#ifndef STATS_H
#define STATS_H
#include "common.h"

extern Metrics metrics;

void reset_metrics(void);
void start_timer(void);
void update_time(void);
int getA(Array *arr, int i);
void setA(Array *arr, int i, int v);
int cmp_idx(Graphisme *Graphisme, Array *arr, int i, int j, Status *status);
void swap_idx(Graphisme *Graphisme, Array *arr, int i, int j, Status *status);
void addComps();
Metrics getMetrics();
void addFrame();


#endif
