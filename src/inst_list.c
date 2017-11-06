#include "inst_list.h"


single_list_t* newSingleList();

void add_new_list(inst_list_t* list){
  list->lists = (single_list_t**) realloc(list->lists, ++list->numLists *
                                         sizeof(single_list_t*));
  list->lists[++list->curList] = newSingleList();
}


void resize(inst_list_t* list) {
  single_list_t* cur = list->lists[list->curList];
  cur->cap *= 2;
  cur->array = (xed_decoded_inst_t*) realloc(cur->array,
                                      cur->cap * sizeof(xed_decoded_inst_t));
  cur->printinfo = (xed_uint64_t*) realloc(cur->printinfo,
                                    cur->cap * sizeof(xed_uint64_t));
}

single_list_t* newSingleList() {
  xed_decoded_inst_t* arr = (xed_decoded_inst_t*)
                            malloc(INITIAL_LIST_LENGTH * sizeof(xed_decoded_inst_t));
  xed_uint64_t* printinfo = (xed_uint64_t*)
                            malloc(INITIAL_LIST_LENGTH * sizeof(xed_uint64_t));
  single_list_t* list = (single_list_t*) malloc(sizeof(single_list_t));
  list->array = arr;
  list->cap = INITIAL_LIST_LENGTH;
  list->size = 0;
  list->printinfo = printinfo;
  return list;
}


inst_list_t* newList() {
  single_list_t** arr = (single_list_t**)
                              malloc(sizeof(single_list_t*));
  inst_list_t* list = (inst_list_t*) malloc(sizeof(inst_list_t));
  single_list_t* first = newSingleList();
  list->lists = arr;
  list->lists[0] = first;
  list->numLists = 1;
  list->curList = 0;
  return list;
}

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem,
                 xed_uint64_t runtime_instruction_address) {
  single_list_t* cur = list->lists[list->curList];
  if (cur->cap == cur->size) {
    resize(list);
  }
  cur->printinfo[cur->size] = runtime_instruction_address;
  cur->array[cur->size++] = elem;
}

void test(xed_decoded_inst_t* xedd) {
  const xed_inst_t*           xi = xed_decoded_inst_inst(xedd);
  const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xedd);
  printf("\ndisplacement: %i\n", xed_decoded_inst_get_branch_displacement(xedd));
  for (size_t i = 0; i < xed_inst_noperands(xi); i++) {
    const xed_operand_t*        op = xed_inst_operand(xi,i);
    xed_operand_enum_t     op_name = xed_operand_name(op);
    //printf("op: %s\n", xed_operand_enum_t2str(op_name));
  }
  printf("size: %i\n", xed_decoded_inst_get_length(xedd));
}

void print_list(inst_list_t* list) {
  for (int i = 0; i < list->numLists; i++) {
    single_list_t* cur = list->lists[i];
    for (size_t j = 0; j < cur->size; j++) {
      char buffer[XED_TMP_BUF_LEN];
      //printf("adress: %i\n", cur->printinfo[j]);
      //test(&cur->array[j]);
      disassemble(buffer, XED_TMP_BUF_LEN, &cur->array[j],
                   cur->printinfo[j]);

      printf("%i: %s\n", j+1, buffer);
    }
    if (i+1 < list->numLists)
      printf("\n\n================================================\n\n\n");
  }
}

void free_single_list(single_list_t* list) {
  free(list->array);
  free(list->printinfo);
  free(list);
}

void free_list(inst_list_t* list) {
  for (size_t i = 0; i < list->numLists; i++) {
    free_single_list(list->lists[i]);
  }
  free(list->lists);
  free(list);
}
