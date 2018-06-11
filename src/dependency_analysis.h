#ifndef DEPENDENCY_ANALYSIS_H
#define DEPENDENCY_ANALYSIS_H

#include <stdlib.h>
#include <stdbool.h>
#include "headers.h"
#include "reg_map.h"
#include "xed.h"
#include "inst_list.h"
#include "graph.h"

reg_map_t* compute_dependencies(single_list_t* instructions);

graph_t* build_controlflowgraph(single_list_t* instructions);

graph_t* build_dependencygraph(reg_map_t* map, graph_t* flowgraph);


void put_operand_in_map(const xed_operand_t* op, xed_decoded_inst_t* xedd,
                        reg_map_t* map, int line);


void compute_instruction(reg_map_t* map, xed_decoded_inst_t* xedd,
                          const xed_inst_t* xi, int line);


void compute_branch_flow(single_list_t* instructions, graph_t* graph, int line);

int branch_is_unconditional(xed_iform_enum_t iform);

void build_single_depency(access_t* first, graph_t* flowgraph, graph_t* dep_graph);

//computes the depency graph in regards to the cfg
graph_t *build_dependencygraph_cfg(single_list_t *instructions, graph_t *cfg);


void branch_analysis_root(graph_t *dg, single_list_t *instructions, graph_t *cfg, int *write_ops, int *write_flags, int start);


//checks all operands for dependencies and adds them respectively
//returns the number of remaining writes
void add_all_dependencies(graph_t *dg, int toline, int *write_ops, int *write_flags, xed_decoded_inst_t *xedd);




#endif
