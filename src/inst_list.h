#if !defined(inst_list_H)
#define inst_list_H

#include <stdlib.h>
#include "headers.h"


typedef struct {
  xed_decoded_inst_t* array;
  int cap;
  int size;
  int* breakpoints;
  int numBreaks;
} inst_list_t;

inst_list_t* newList(int initLength);

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem);

void set_breakpoint(inst_list_t* list);

void free_list(inst_list_t* list);

#endif
