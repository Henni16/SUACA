#include "inst_list.h"


void resize(inst_list_t* list) {
  list->cap *= 2;
  list->array = (xed_decoded_inst_t*) realloc(list->array,
                                      list->cap * sizeof(xed_decoded_inst_t));
  list->printinfo = (xed_uint64_t*) realloc(list->printinfo,
                                    list->cap * sizeof(xed_uint64_t));
}


inst_list_t* newList(int initLength) {
  xed_decoded_inst_t* arr = (xed_decoded_inst_t*)
                              malloc(initLength * sizeof(xed_decoded_inst_t));
  xed_uint64_t* printinfo = (xed_uint64_t*) malloc(initLength * sizeof(xed_uint64_t));
  inst_list_t* list = (inst_list_t*) malloc(sizeof(inst_list_t));
  list->breakpoints = (int*) malloc(5 * sizeof(int));
  list->numBreaks = 0;
  list->array = arr;
  list->cap = initLength;
  list->size = 0;
  list->printinfo = printinfo;
  return list;
}

void add_to_list(inst_list_t* list, xed_decoded_inst_t elem,
                 xed_uint64_t runtime_instruction_address) {
  if (list->cap == list->size) {
    resize(list);
  }
  list->printinfo[list->size] = runtime_instruction_address;
  list->array[list->size++] = elem;
}


void set_breakpoint(inst_list_t* list) {
  if (!list->size)
    return;
  if (list->numBreaks > 0 && list->numBreaks % 5 == 0) {
    list->breakpoints = (int*) realloc(list->breakpoints,
                                (list->numBreaks+5)*sizeof(int));
  }
  list->breakpoints[list->numBreaks++] = list->size;
}


void disassemble(char* buf,
                 int buflen,
                 xed_decoded_inst_t* xedd,
                 xed_uint64_t runtime_instruction_address)
{
    int ok;
    xed_print_info_t pi;
    xed_init_print_info(&pi);
    pi.p = xedd;
    pi.blen = buflen;
    pi.buf = buf;


    // 0=use the default symbolic disassembly function registered via
    // xed_register_disassembly_callback(). If nonzero, it would be a
    // function pointer to a disassembly callback routine. See xed-disas.h
    pi.disassembly_callback = 0;

    pi.runtime_address = runtime_instruction_address;
    pi.syntax = XED_SYNTAX_INTEL;
    pi.format_options_valid = 1;

    xed_format_options_t format_options;
    memset(&format_options,0,sizeof(xed_format_options_t));
  #if defined(XED_NO_HEX_BEFORE_SYMBOLIC_NAMES)
    format_options.hex_address_before_symbolic_name=0;
  #else
    format_options.hex_address_before_symbolic_name=1;
  #endif
    format_options.write_mask_curly_k0 = 1;
    format_options.lowercase_hex = 1;
    pi.format_options = format_options;

    pi.buf[0]=0; //allow use of strcat

    ok = xed_format_generic(&pi);
    if (!ok)
    {
        pi.blen = xed_strncpy(pi.buf,"Error disassembling ",pi.blen);
        pi.blen = xed_strncat(pi.buf,
                               xed_syntax_enum_t2str(pi.syntax),
                               pi.blen);
        pi.blen = xed_strncat(pi.buf," syntax.",pi.blen);
    }
}


void print_list(inst_list_t* list) {
  int breaks = 0;
  int line = 1;
  for (int i = 0; i < list->size; i++) {
    if (list->breakpoints[breaks] == i) {
      breaks++;
      line = 1;
      printf("\nNew block!\n\n");
    }
    char buffer[XED_TMP_BUF_LEN];
    disassemble(buffer, XED_TMP_BUF_LEN, &list->array[i],
                 list->printinfo[i]);
    printf("%i: %s\n", line++, buffer);
  }
}


void free_list(inst_list_t* list) {
  free(list->array);
  free(list->breakpoints);
  free(list);
}
