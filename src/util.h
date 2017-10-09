#if !defined(UTIL_H)
#define UTIL_H

#include <stdio.h>
#include "headers.h"
#include "inst_list.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define XED_HEX_BUFLEN 200
#define XED_TMP_BUF_LEN (1024*4)

typedef struct {
    xed_state_t dstate;
    int ninst;
    int decode_only;
    int sixty_four_bit;
    int mpx_mode;
    char* input_file_name;
    char* symbol_search_path;     // for dbghelp symbol caches
    char* target_section;
    xed_bool_t use_binary_mode;
    xed_int64_t addr_start;
    xed_int64_t addr_end;
    xed_bool_t xml_format;
    xed_int64_t fake_base;
    xed_bool_t resync; /* turn on/off symbol-based resynchronization */
    xed_bool_t line_numbers; /* control for printing file/line info */
    unsigned int perf_tail_start;
    xed_bool_t ast;
    xed_bool_t histo;
    xed_chip_enum_t chip;
    xed_bool_t emit_isa_set;
    xed_format_options_t format_options;
    xed_operand_enum_t operand;
    xed_uint32_t operand_value;

    xed_uint64_t errors;
    xed_uint64_t errors_chip_check;

    unsigned char* s; // start of image
    unsigned char* a; // start of instructions to decode region
    unsigned char* q; // end of region
    // where this region would live at runtime
    xed_uint64_t runtime_vaddr;
    // where to start in program space, if not zero
    xed_uint64_t runtime_vaddr_disas_start;

    // where to stop in program space, if not zero
    xed_uint64_t runtime_vaddr_disas_end;

    // a function to convert addresses to symbols
    char* (*symfn)(xed_uint64_t, void*);
    void* caller_symbol_data;

    void (*line_number_info_fn)(xed_uint64_t addr);

} xed_disas_info_t;




void xed_disas_info_init(xed_disas_info_t* p);

void xed_map_region(const char* path,
                    void** start,
                    unsigned int* length);

void init_xedd(xed_decoded_inst_t* xedd,
               xed_disas_info_t* di);



static int
all_zeros(xed_uint8_t* p, unsigned int len);

static void XED_NORETURN
die_zero_len(
    xed_uint64_t runtime_instruction_address,
    unsigned char* z,
    xed_disas_info_t* di,
    xed_error_enum_t xed_error);

static char nibble_to_ascii_hex(xed_uint8_t i);

void xed_print_hex_line(char* buf,
                        const xed_uint8_t* array,
                        const int length,
                        const int buflen);

static unsigned int
check_resync(xed_disas_info_t* di,
             xed_uint64_t runtime_instruction_address,
             unsigned int length,
             unsigned char* z);

void disassemble(xed_disas_info_t* di,
                 char* buf,
                 int buflen,
                 xed_decoded_inst_t* xedd,
                 xed_uint64_t runtime_instruction_address,
                 void* caller_data);


void xed_disas_test(xed_disas_info_t* di, inst_list_t* instructions);

#endif
