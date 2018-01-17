#include "sim_inst.h"

sim_inst_t *newSimInst(int line, bool *ports, int micro_ops, int num_fathers,
                       int latency, int num_children) {
    sim_inst_t *ret = (sim_inst_t *) malloc(sizeof(sim_inst_t));
    ret->num_micro_ops = micro_ops;
    ret->micro_ops_loaded = 0;
    //ret->micro_ops_processed = 0;
    //ret->being_processed = 0;
    ret->usable_ports = ports;
    ret->line = line;
    ret->fathers_todo = num_fathers;
    ret->next = NULL;
    ret->cycles_delayed = 0;
    ret->delayed_cycles = 0;
    ret->latency = latency;
    ret->num_dep_children = num_children;
    ret->dep_children = (reg_sim_inst_t *) malloc(num_children * sizeof(reg_sim_inst_t));
    return ret;
}


bool all_fathers_done(sim_inst_t *si) {
    return !si->fathers_todo;
}


int get_loadable_micro_ops(sim_inst_t *inst) {
    return inst->num_micro_ops - inst->micro_ops_loaded;
}


void free_sim_inst(sim_inst_t *si) {
    free(si->usable_ports);
    free(si);
}

sim_inst_list_t *newSimInstList(int length) {
    sim_inst_list_t *list = malloc(sizeof(sim_inst_list_t));
    list->arr = malloc(length * sizeof(sim_inst_t *));
    list->size = length;
    return list;
}

void add_to_sim_list(sim_inst_list_t *list, sim_inst_t *elem) {
    list->arr[elem->line] = elem;
}


void free_sim_inst_list(sim_inst_list_t *list) {
    for (size_t i = 0; i < list->size; i++) {
        free_sim_inst(list->arr[i]);
    }
    free(list->arr);
    free(list);
}
