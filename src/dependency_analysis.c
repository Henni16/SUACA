#include "dependency_analysis.h"
#include "inst_list.h"
#include "graph.h"


reg_map_t *compute_dependencies(single_list_t *list) {
    reg_map_t *map = newMap(xed_reg_enum_t_last());
    xed_decoded_inst_t *instructions = list->array;
    xed_decoded_inst_t xedd;

    for (size_t i = 0; i < list->size; i++) {
        xedd = instructions[i];
        const xed_inst_t *xi = xed_decoded_inst_inst(&xedd);
        if (xed_inst_noperands(xi) > 0)
            compute_instruction(map, &xedd, xi, i);
    }
    order_map(map);
    return map;
}


void compute_instruction(reg_map_t *map, xed_decoded_inst_t *xedd,
                         const xed_inst_t *xi, int line) {
    for (size_t i = 0; i < xed_inst_noperands(xi); i++) {
        const xed_operand_t *op = xed_inst_operand(xi, i);
        put_operand_in_map(op, xedd, map, line);
    }
}


void put_operand_in_map(const xed_operand_t *op, xed_decoded_inst_t *xedd,
                        reg_map_t *map, int line) {
    xed_operand_enum_t op_name = xed_operand_name(op);
    switch (op_name) {
        case XED_OPERAND_AGEN:
        case XED_OPERAND_MEM0: {
            if (xed3_operand_get_base0(xedd) != XED_REG_INVALID)
                add_to_map(map, xed3_operand_get_base0(xedd), line, READ);
            if (xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
                add_to_map(map, xed3_operand_get_seg0(xedd), line, READ);
            if (xed3_operand_get_index(xedd) != XED_REG_INVALID)
                add_to_map(map, xed3_operand_get_index(xedd), line, READ);
            break;
        }
        case XED_OPERAND_MEM1: {
            if (xed3_operand_get_base0(xedd) != XED_REG_INVALID)
                add_to_map(map, xed3_operand_get_base0(xedd), line, READ);
            if (xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
                add_to_map(map, xed3_operand_get_seg0(xedd), line, READ);
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


graph_t *build_controlflowgraph(single_list_t *instructions) {
    graph_t *graph = newGraph(instructions->size);
    for (size_t i = 0; i < instructions->size; i++) {
        if (is_branch_instruction(&instructions->array[i]))
            compute_branch_flow(instructions, graph, i);
        else
            add_graph_dependency(i, i + 1, graph, XED_REG_INVALID);
    }
    return graph;
}


void compute_branch_flow(single_list_t *instructions, graph_t *graph, int line) {
    xed_decoded_inst_t *xedd = &instructions->array[line];
    xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);

    if (!branch_is_unconditional(iform))
        add_graph_dependency(line, line + 1, graph, XED_REG_INVALID);

    int displacement = xed_decoded_inst_get_branch_displacement(xedd);
    //for backbranches
    int toLine = displacement > 0 ? line + 1 : line;
    if (displacement < 0) {
        while (displacement < 0 && toLine >= 0) {
            displacement += xed_decoded_inst_get_length(&instructions->array[toLine--]);
        }
        displacement == 0 ? toLine++ : 0;
    }
        //normal branches
    else {
        while (displacement > 0 && toLine < graph->size) {
            displacement -= xed_decoded_inst_get_length(&instructions->array[toLine++]);
        }
    }
    add_graph_dependency(line, toLine, graph, XED_REG_INVALID);
}

int branch_is_unconditional(xed_iform_enum_t iform) {
    return iform >= 636 && iform <= 638 || iform >= 656 && iform <= 658;
}


graph_t *build_dependencygraph(reg_map_t *map, graph_t *flowgraph) {
    graph_t *dep_graph = newGraph(flowgraph->size);
    access_t *cur;
    for (size_t i = 0; i < map->size; i++) {
        cur = map->map[i];
        if (cur != NULL && i != XED_REG_RIP)
            build_single_depency(cur, flowgraph, dep_graph);
    }
    return dep_graph;
}


void build_single_depency(access_t *first, graph_t *flowgraph, graph_t *dep_graph) {
    int fromLine;
    access_t *back = NULL;
    while (first != NULL) {
        if (first->read_write == WRITE) {
            fromLine = first->line;
            first = first->next;
            while (1) {
                if (first == NULL) break;
                if (first->read_write == WRITE) {
                    if (is_successor(fromLine, first->line, flowgraph)) break;
                    if (back == NULL) back = first;
                }
                if (is_successor(fromLine, first->line, flowgraph))
                    add_graph_dependency(fromLine, first->line, dep_graph, first->used_reg);
                first = first->next;
            }
            if (back != NULL) {
                first = back;
                back = NULL;
            }
        } else
            first = first->next;
    }
}


graph_t *build_dependencygraph_cfg(single_list_t *instructions, graph_t *cfg) {
    graph_t *dg = newGraph(instructions->size);

    int write_ops[xed_reg_enum_t_last()];
    int write_flags[xed_flag_enum_t_last()];

    for (int i = 0; i < xed_reg_enum_t_last(); ++i) {
        write_ops[i] = -1;
    }
    for (int i = 0; i < xed_flag_enum_t_last(); ++i) {
        write_flags[i] = -1;
    }
    branch_analysis_root(dg, instructions, cfg, write_ops, write_flags, 0);
    return dg;
}


void branch_analysis_root(graph_t *dg, single_list_t *instructions, graph_t *cfg, int *write_ops, int *write_flags, int start) {
    node_t *cur_node = cfg->nodes[start];
    int cur_line = start;
    while (true) {
        add_all_dependencies(dg, cur_line, write_ops, write_flags, &instructions->array[cur_line]);
        if (cur_node->num_successors > 1) {
            int write_ops_single[xed_reg_enum_t_last()];
            int write_flags_single[xed_flag_enum_t_last()];
            for (int i = 0; i < cur_node->num_successors; ++i) {
                if (cur_line >= cur_node->successors[i].line) {
                    continue;
                }
                for (int j = 0; j < xed_reg_enum_t_last(); ++j) {
                    write_ops_single[j] = write_ops[j];
                }
                for (int j = 0; j < xed_flag_enum_t_last(); ++j) {
                    write_flags_single[j] = write_flags[j];
                }
                branch_analysis_root(dg, instructions, cfg, write_ops_single, write_flags_single, cur_node->successors[i].line);
            }
            return;
        } else if (cur_node->num_successors == 0) {
            return;
        } else {
            int child_line = cur_node->successors[0].line;
            if (cur_line >= child_line) {
                return;
            }
            cur_node = cfg->nodes[child_line];
            cur_line = child_line;
        }
    }
}


void add_all_dependencies(graph_t *dg, int toline, int *write_ops, int *write_flags, xed_decoded_inst_t *xedd) {

    // flags
    if (xed_decoded_inst_uses_rflags(xedd)) {
        const xed_simple_flag_t  *simple_flag = xed_decoded_inst_get_rflags_info(xedd);
        const xed_flag_action_t *flag_action;
        xed_flag_enum_t name;
        for (unsigned int i = 0; i < xed_simple_flag_get_nflags(simple_flag); ++i) {
            flag_action = xed_simple_flag_get_flag_action(simple_flag, i);
            name = xed_flag_action_get_flag_name(flag_action);
            if (write_flags[name] != -1 && xed_flag_action_read_flag(flag_action)) {
                add_graph_dependency_flag(write_flags[name], toline, dg, name);
            } else if (xed_flag_action_writes_flag(flag_action)) {
                write_flags[name] = toline;
            }
        }
    }

    // registers
    const xed_inst_t *xi = xed_decoded_inst_inst(xedd);
    int num_writen = 0;
    int num_read = 0;
    xed_reg_enum_t writen[xed_inst_noperands(xi)];
    xed_reg_enum_t read[xed_inst_noperands(xi)];
    for (size_t i = 0; i < xed_inst_noperands(xi); i++) {
        const xed_operand_t *op = xed_inst_operand(xi, i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        switch (op_name) {
            case XED_OPERAND_AGEN:
            case XED_OPERAND_MEM0: {
                if (xed3_operand_get_base0(xedd) != XED_REG_INVALID)
                    read[num_read++] = xed3_operand_get_base0(xedd);
                if (xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
                    read[num_read++] = xed3_operand_get_seg0(xedd);
                if (xed3_operand_get_index(xedd) != XED_REG_INVALID)
                    read[num_read++] = xed3_operand_get_index(xedd);
                break;
            }
            case XED_OPERAND_MEM1: {
                if (xed3_operand_get_base0(xedd) != XED_REG_INVALID)
                    read[num_read++] = xed3_operand_get_base0(xedd);
                if (xed3_operand_get_seg0(xedd) != XED_REG_INVALID)
                    read[num_read++] = xed3_operand_get_seg0(xedd);
                break;
            }
            default: {
                xed_reg_enum_t reg;
                if (!xed_operand_is_register(op_name)) break;
                xed3_get_generic_operand(xedd, op_name, &reg);
                if (reg == XED_REG_RFLAGS) break;
                if (xed_operand_read(op))
                    read[num_read++] = reg;
                if (xed_operand_written(op) && reg != XED_REG_RIP)
                    writen[num_writen++] = compute_register(reg);
            }
        }
    }
    int fromLine;
    for (int i = 0; i < num_read; ++i) {
        fromLine = write_ops[compute_register(read[i])];
        if (fromLine != -1) {
            // check if edge already exists, we only want it once
            bool exists = false;
            for (int j = 0; j < dg->nodes[toline]->num_fathers; ++j) {
                if (dg->nodes[toline]->fathers[j].line == fromLine) {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                add_graph_dependency(fromLine, toline, dg, read[i]);
        }
    }
    for (int i = 0; i < num_writen; ++i) {
        write_ops[writen[i]] = toline;
    }
}










































