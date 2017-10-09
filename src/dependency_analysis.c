#include "dependency_analysis.h"


void compute_instruction(reg_map_t* map, xed_decoded_inst_t* xedd,
                          const xed_inst_t* xi, int line);

reg_map_t* compute_dependencies(inst_list_t* list) {
  reg_map_t* map = newMap(xed_reg_enum_t_last());
  xed_decoded_inst_t* instructions = list->array;
  xed_decoded_inst_t xedd;

  for (size_t i = 0; i < list->size; i++) {
    xedd = instructions[i];
    const xed_inst_t* xi = xed_decoded_inst_inst(&xedd);
    if (xed_inst_noperands(xi) > 0)
      compute_instruction(map, &xedd, xi, i);
  }
  return map;
}

void compute_instruction(reg_map_t* map, xed_decoded_inst_t* xedd,
                          const xed_inst_t* xi, int line) {
  for (size_t i = 0; i < xed_inst_noperands(xi); i++) {
    const xed_operand_t* op = xed_inst_operand(xi,i);
    xed_operand_enum_t op_name = xed_operand_name(op);
    if (!xed_operand_is_register(op_name)) continue;
    xed_reg_enum_t reg;
    xed3_get_generic_operand(xedd, op_name, &reg);
    if (xed_operand_read(op))
      add_to_map(map, reg, line, READ);
    if (xed_operand_written(op))
      add_to_map(map, reg, line, WRITE);
  }
}
