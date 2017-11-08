#ifndef DEPENDENCY_ANALYSIS_H
#define DEPENDENCY_ANALYSIS_H

#include <stdlib.h>
#include "headers.h"
#include "reg_map.h"
#include "disas.h"
#include "inst_list.h"
#include "graph.h"

reg_map_t* compute_dependencies(single_list_t* instructions);

graph_t* build_controlflowgraph(single_list_t* instructions);

graph_t* build_dependencygraph(reg_map_t* map, graph_t* flowgraph);


//"private" functions

void put_operand_in_map(const xed_operand_t* op, xed_decoded_inst_t* xedd,
                        reg_map_t* map, int line);


void compute_instruction(reg_map_t* map, xed_decoded_inst_t* xedd,
                          const xed_inst_t* xi, int line);


void compute_branch_flow(single_list_t* instructions, graph_t* graph, int line);

int branch_is_unconditional(xed_iform_enum_t iform);

void build_single_depency(access_t* first, graph_t* flowgraph, graph_t* dep_graph);

int is_successor(int from, int to, graph_t* graph);

int is_successor_seen(int from, int to, graph_t* graph, int* seen);

#endif
