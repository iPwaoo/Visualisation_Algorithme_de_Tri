// utils.h
#ifndef UTILS_H
#define UTILS_H
#include "common.h"

const char *get_algorithm_name(Algorithm a);
const char *get_state_name(Status *status);
void generate_random_array(Array *arr);
void print_status(Status *st);
void print_keyboard_event(SDL_KeyboardEvent *ev);

#endif
