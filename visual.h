// visual.h
#ifndef VISUAL_H
#define VISUAL_H
#include "common.h"

void render_array(Graphisme *gfx, Array *arr, const char *subtitle, Status *status);
void visual_tick(Graphisme *gfx, Array *arr, int a, int b, const char *subtitle, Status *status);
void apply_resize(Graphisme *gfx, Array *arr);
const char *get_algorithm_name(Algorithm algo);
void handle_key(Graphisme *gfx, Array *arr, Status *status);
void apply_resize(Graphisme *Graphisme, Array *arr);
int init_SDL(Graphisme *gfx);

#endif
