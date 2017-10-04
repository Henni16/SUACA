#if !defined(inst_list_H)
#define inst_list_H

#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"
#include <stdlib.h>



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
