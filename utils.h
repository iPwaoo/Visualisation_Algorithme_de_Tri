// utils.h
#ifndef UTILS_H
#define UTILS_H
#include "common.h"

const char *algo_name(Algorithm a);
const char *state_name(Status *status);
void random_array(Array *arr);
void print_status(Status *st);
void print_keyboard_event(SDL_KeyboardEvent *ev);

#endif
