#if !defined(inst_list_H)
#define inst_list_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"
#define XED_TMP_BUF_LEN (1024*4)


typedef struct {
  xed_decoded_inst_t* array;
  int cap;
  int size;
  int* breakpoints;
  int numBreaks;
  xed_uint64_t* printinfo;
} inst_list_t;

inst_list_t* newList(int initLength);

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem,
                 xed_uint64_t runtime_instruction_address);

void set_breakpoint(inst_list_t* list);

void print_list(inst_list_t* list);

void free_list(inst_list_t* list);

#endif
