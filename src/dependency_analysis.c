#include "dependency_analysis.h"


reg_map_t* compute_dependencies(single_list_t* list) {
  reg_map_t* map = newMap(xed_reg_enum_t_last());
  xed_decoded_inst_t* instructions = list->array;
  xed_decoded_inst_t xedd;

  for (size_t i = 0; i < list->size; i++) {
    xedd = instructions[i];
    const xed_inst_t* xi = xed_decoded_inst_inst(&xedd);
    if (xed_inst_noperands(xi) > 0)
      compute_instruction(map, &xedd, xi, i+1);
  }
  return map;
}


void compute_instruction(reg_map_t* map, xed_decoded_inst_t* xedd,
                          const xed_inst_t* xi, int line) {
  for (size_t i = 0; i < xed_inst_noperands(xi); i++) {
    const xed_operand_t* op = xed_inst_operand(xi,i);
    put_operand_in_map(op, xedd, map, line);
  }
}


void put_operand_in_map(const xed_operand_t* op, xed_decoded_inst_t* xedd,
                        reg_map_t* map, int line) {
  xed_operand_enum_t op_name = xed_operand_name(op);
  switch (op_name) {
    case XED_OPERAND_AGEN:
    case XED_OPERAND_MEM0: {
      if(xed3_operand_get_base0(xedd) != XED_REG_INVALID)
        add_to_map(map, xed3_operand_get_base0(xedd), line , READ);
      if(xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
        add_to_map(map, xed3_operand_get_seg0(xedd), line , READ);
      if(xed3_operand_get_index(xedd) != XED_REG_INVALID)
        add_to_map(map, xed3_operand_get_index(xedd), line , READ);
      break;
    }
    case XED_OPERAND_MEM1: {
      if(xed3_operand_get_base0(xedd) != XED_REG_INVALID)
        add_to_map(map, xed3_operand_get_base0(xedd), line , READ);
      if(xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
        add_to_map(map, xed3_operand_get_seg0(xedd), line , READ);
      break;
    }
    default: {
      xed_reg_enum_t reg;
      if (!xed_operand_is_register(op_name)) break;
      xed3_get_generic_operand(xedd, op_name, &reg);
      if (xed_operand_read(op))
        add_to_map(map, reg, line, READ);
      if (xed_operand_written(op))
        add_to_map(map, reg, line, WRITE);
    }
    }
}




graph_t build_controlflowgraph(single_list_t* instructions){
  graph_t graph = newGraph(instructions->size);
  for (size_t i = 0; i < instructions->size-1; i++) {
    if (is_branch_instruction(&instructions->array[i]))
      compute_branch_flow(instructions, graph, i);
    else
      add_graph_dependency(i, i+1, graph);
  }
  return graph;
}


void compute_branch_flow(single_list_t* instructions, graph_t graph, int line){
  xed_decoded_inst_t* xedd = &instructions->array[line];
  xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);
  if (!branch_is_unconditional(iform))
    add_graph_dependency(line, line+1, graph);
  int displacement = xed_decoded_inst_get_branch_displacement(xedd);
  int toLine = line+1;
  while (displacement != 0) {
    displacement -= xed_decoded_inst_get_length(&instructions->array[toLine++]);
  }
  add_graph_dependency(line, toLine, graph);
}

int branch_is_unconditional(xed_iform_enum_t iform){
  return iform >= 636 && iform <= 638 || iform >= 656 && iform <= 658;
}
