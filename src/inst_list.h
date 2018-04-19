#if !defined(inst_list_H)
#define inst_list_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"
#define XED_TMP_BUF_LEN (1024*4)
#define INITIAL_LIST_LENGTH 10

//forward declaration to avoid warning
void disassemble(char* buf,
                 int buflen,
                 xed_decoded_inst_t* xedd,
                 xed_uint64_t runtime_instruction_address);

typedef struct {
  xed_decoded_inst_t* array;
  int cap;
  int size;
  int single_loop_size;
  xed_uint64_t* printinfo;
} single_list_t;


typedef struct {
  single_list_t** lists;
  int numLists;
  int curList;
} inst_list_t;


inst_list_t* newList();

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem,
                 xed_uint64_t runtime_instruction_address);

void add_new_list(inst_list_t* list);

void print_list(inst_list_t* list);

int add_loop_instructions(single_list_t *list);

void free_list(inst_list_t* list);

#endif
