#include "sim_inst.h"
#include "xmlParser.h"

sim_inst_t *newSimInst(int line, port_ops_t *micro_ops, int num_micro_ops, int num_fathers,
                       int latency, int num_children, int numports) {
    sim_inst_t *ret = (sim_inst_t *) malloc(sizeof(sim_inst_t));
    ret->num_micro_ops = num_micro_ops;
    ret->micro_ops_loaded = 0;
    ret->micro_ops = micro_ops;
    ret->line = line;
    ret->fathers_todo = num_fathers;
    ret->next = NULL;
    ret->cycles_delayed = 0;
    ret->delayed_cycles = 0;
    ret->latency = latency;
    ret->num_dep_children = num_children;
    ret->executed_cycles = 0;
    ret->executed_microops = 0;
    ret->used_ports = calloc(numports, sizeof(int));
    ret->dep_children = (reg_sim_inst_t *) malloc(num_children * sizeof(reg_sim_inst_t));
    return ret;
}


bool all_fathers_done(sim_inst_t *si) {
    return !si->fathers_todo;
}


int get_loadable_micro_ops(sim_inst_t *inst) {
    return inst->num_micro_ops - inst->micro_ops_loaded;
}


void load_num_micro_ops(sim_inst_t *inst, int num_ops) {
    port_ops_t *po = inst->micro_ops;
    while (num_ops > 0 && po) {
        //all micro ops can be loaded
        if (num_ops >= po->numops) {
            po->loaded_ops += po->numops;
            num_ops -= po->numops;
            po->numops = 0;
            po = po->next;
        } else {
            //we have to load partially
            po->loaded_ops += num_ops;
            po->numops -= num_ops;
            num_ops = 0;
        }
    }
}


void free_sim_inst(sim_inst_t *si) {
    if (!si) return;
    free_port_op(si->micro_ops);
    free(si->used_ports);
    free(si);
}

sim_inst_list_t *newSimInstList(int length) {
    sim_inst_list_t *list = malloc(sizeof(sim_inst_list_t));
    list->arr = calloc(length, sizeof(sim_inst_t *));
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


//TODO
void print_sim_inst_list(sim_inst_list_t *list, single_list_t *inst_list, int num_ports) {
    /*sim_inst_t *inst = NULL;
    char buffer[XED_TMP_BUF_LEN];
    for (int i = 0; i < list->size; ++i) {
        inst = list->arr[i];
        disassemble(buffer, XED_TMP_BUF_LEN, &inst_list->array[i],
                    inst_list->printinfo[i]);
        printf("%s: port used: %i had to wait: %i caused to wait: %i numops: %i\n", buffer,
               inst->used_port, inst->cycles_delayed, inst->delayed_cycles, inst->num_micro_ops);
    }*/

    printf("Num Uops ||   had   || caused  || Used Ports\n");
    printf("         || to wait || to wait ||");
    for (int i = 0; i < num_ports; ++i) {
        printf(" %i ||", i);
    }
    printf("\n----------------------------------------------------------------\n");
    sim_inst_t *inst = NULL;
    char buffer[XED_TMP_BUF_LEN];
    for (int i = 0; i < list->size; ++i) {
        inst = list->arr[i];
        disassemble(buffer, XED_TMP_BUF_LEN, &inst_list->array[i],
                    inst_list->printinfo[i]);
        printf("   %i     ||    %i    ||    %i    ||", inst->num_micro_ops, inst->cycles_delayed, inst->delayed_cycles);
        for (int j = 0; j < num_ports; ++j) {
            printf(" %i ||", inst->used_ports[j]);
        }
        printf("%s\n", buffer);
    }
}