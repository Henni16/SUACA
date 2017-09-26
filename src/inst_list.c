#include "inst_list.h"


void resize(inst_list_t* list) {
  list->cap *= 2;
  list->array = (xed_decoded_inst_t*) realloc(list->array,
                                      list->cap * sizeof(xed_decoded_inst_t));
}


inst_list_t* newList(int initLength) {
  xed_decoded_inst_t* arr = (xed_decoded_inst_t*)
                              malloc(initLength * sizeof(xed_decoded_inst_t));
  inst_list_t* list = (inst_list_t*) malloc(sizeof(inst_list_t*));
  list->breakpoints = (int*) malloc(5 * sizeof(int));
  list->numBreaks = 0;
  list->array = arr;
  list->cap = initLength;
  list->size = 0;
  return list;
}

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem) {
  if (list->cap == list->size) {
    resize(list);
  }
  list->array[list->size++] = elem;
}

void set_breakpoint(inst_list_t* list) {
  if (!list->size)
    return;
  if (list->numBreaks > 0 && list->numBreaks%5 == 0) {
    list->breakpoints = (int*) realloc(list->breakpoints,
                        list->numBreaks + 5);
  }
  list->breakpoints[list->numBreaks++] = list->size;
}
