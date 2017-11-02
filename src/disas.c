#include "disas.h"
#include <limits.h>

inst_list_t* build_inst_list(int argc, char** argv) {
  xed_bool_t sixty_four_bit = 0;
  unsigned int mpx_mode = 0;
  xed_bool_t decode_only = 1;
  char* input_file_name = 0;
  char* symbol_search_path = 0;
  xed_state_t dstate;
  xed_bool_t encode = 0;
  unsigned int ninst = INT_MAX; // FIXME: should use maxint...
  //perf_tail is for skipping first insts in performance measure mode
  unsigned int perf_tail = 0;
  xed_bool_t decode_encode = 0;
  int i,j;
  unsigned int loop_decode = 0;
  xed_bool_t decode_raw = 0;
  xed_bool_t decode_hex = 0;
  xed_bool_t assemble  = 0;
  char* target_section = 0;
  xed_bool_t use_binary_mode = 1;
  xed_bool_t emit_isa_set = 0;
  xed_int64_t addr_start = 0;
  xed_int64_t addr_end = 0;
  xed_int64_t fake_base = 0;
  xed_bool_t xml_format =0;
  xed_bool_t resync = 0;
  xed_bool_t ast = 0;
  xed_bool_t histo = 0;
  int line_numbers = 0;
  xed_chip_enum_t xed_chip = XED_CHIP_INVALID;
  xed_operand_enum_t operand = XED_OPERAND_INVALID;
  xed_uint32_t operand_value = 0;
  xed_bool_t filter = 0;
#if defined(XED_LINUX)
  char *prefix = NULL;
#endif
  inst_list_t* instructions;

  xed_decoded_inst_t xedd;
  xed_uint_t retval_okay = 1;
  unsigned int obytes=0;
  xed_disas_info_t decode_info;

  /* I have this here to test the functionality, if you are happy with
   * the XED formatting options, then you do not need to set this or call
   * xed_format_set_options() */

  xed_format_options_t format_options;
  memset(&format_options,0,sizeof(xed_format_options_t));
#if defined(XED_NO_HEX_BEFORE_SYMBOLIC_NAMES)
  format_options.hex_address_before_symbolic_name=0;
#else
  format_options.hex_address_before_symbolic_name=1;
#endif
  format_options.write_mask_curly_k0 = 1;
  format_options.lowercase_hex = 1;


  xed_state_init(&dstate,
                 XED_MACHINE_MODE_LEGACY_32,
                 XED_ADDRESS_WIDTH_32b,  /* 2nd parameter ignored */
                 XED_ADDRESS_WIDTH_32b);

  resync = 1;

  if (argc < 2)
    return NULL;
  else {
      input_file_name = argv[1];
  }

  xed_tables_init();


#if defined(XED_DECODER)
  xed_format_set_options(format_options);
#endif


  retval_okay = 1;
  obytes=0;

  xed_disas_info_init(&decode_info);
  decode_info.input_file_name  = input_file_name;
  decode_info.symbol_search_path = symbol_search_path;
  decode_info.dstate           = dstate;
  decode_info.ninst            = ninst;
  decode_info.decode_only      = decode_only;
  decode_info.sixty_four_bit   = sixty_four_bit;
  decode_info.target_section   = target_section;
  decode_info.use_binary_mode  = use_binary_mode;
  decode_info.addr_start       = addr_start;
  decode_info.addr_end         = addr_end;
  decode_info.xml_format       = xml_format;
  decode_info.fake_base        = fake_base;
  decode_info.resync           = resync;
  decode_info.line_numbers     = line_numbers;
  decode_info.perf_tail_start  = perf_tail;
  decode_info.ast              = ast;
  decode_info.histo            = histo;
  decode_info.chip             = xed_chip;
  decode_info.mpx_mode         = mpx_mode;
  decode_info.emit_isa_set     = emit_isa_set;
  decode_info.format_options   = format_options;
  decode_info.operand          = operand;
  decode_info.operand_value    = operand_value;

  init_xedd(&xedd, &decode_info);
  instructions = newList();


#if defined(__APPLE__)
          xed_disas_macho(&decode_info, instructions);
#elif defined(XED_ELF_READER)
          xed_disas_elf(&decode_info, instructions);
#elif defined(_WIN32)
          xed_disas_pecoff(&decode_info, instructions);
#else
          xedex_derror("No PECOFF, ELF or MACHO support compiled in");
#endif

  return instructions;
  (void) obytes;
}
