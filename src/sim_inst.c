#include "sim_inst.h"
#include "xmlParser.h"


int sim_inst_global_id = 0;

sim_inst_t *newSimInst(int line, port_ops_t *micro_ops, int num_micro_ops, int num_fathers,
                       int latency, int num_children, int numports, int num_insts) {
    sim_inst_t *ret = (sim_inst_t *) malloc(sizeof(sim_inst_t));
    ret->num_micro_ops = num_micro_ops;
    ret->micro_ops_loaded = 0;
    ret->micro_ops = copy_port_op(micro_ops, numports);
    ret->line = line;
    ret->id = sim_inst_global_id++;
    ret->fathers_todo = num_fathers;
    ret->fathers = malloc(num_fathers * sizeof(sim_inst_t*));
    ret->next = NULL;
    ret->cycles_delayed = 0;
    ret->delayed_cycles = 0;
    ret->latency = latency;
    ret->num_dep_children = num_children;
    ret->executed_cycles = 0;
    ret->used_ports = calloc(numports, sizeof(int));
    ret->dep_children = (reg_sim_inst_t *) malloc(num_children * sizeof(reg_sim_inst_t));
    ret->dep_delays = malloc(num_insts * sizeof(delays_t));
    for (int i = 0; i < num_insts; ++i) {
        ret->dep_delays[i] = (delays_t){0,0};
    }
    ret->port_delays = malloc(numports * sizeof(delays_t));
    for (int j = 0; j < numports; ++j) {
        ret->port_delays[j] = (delays_t){0,0};
    }
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
    free(si->dep_children);
    free(si->fathers);
    free(si->dep_delays);
    free(si->port_delays);
    free(si);
}

sim_inst_list_t *newSimInstList(int length) {
    sim_inst_list_t *list = malloc(sizeof(sim_inst_list_t));
    list->arr = calloc(length, sizeof(sim_inst_t *));
    list->size = length;
    return list;
}

void add_to_sim_list(sim_inst_list_t *list, sim_inst_t *elem, int num_ports, int num_insts) {
    int index = elem->line % num_insts;
    if (!list->arr[index])
        list->arr[index] = elem;
    else {
        sim_inst_t *old = list->arr[index];
        old->delayed_cycles += elem->delayed_cycles;
        old->cycles_delayed += elem->cycles_delayed;
        for (int i = 0; i < num_ports; ++i) {
            old->used_ports[i] += elem->used_ports[i];
            old->port_delays[i].delay_suffered += elem->port_delays[i].delay_suffered;
            old->port_delays[i].delay_caused += elem->port_delays[i].delay_caused;
        }
        for (int i = 0; i < num_insts; ++i) {
            old->dep_delays[i].delay_caused += elem->dep_delays[i].delay_caused;
            old->dep_delays[i].delay_suffered += elem->dep_delays[i].delay_suffered;
        }
    }
}


void free_sim_inst_list(sim_inst_list_t *list) {
    for (size_t i = 0; i < list->size; i++) {
        free_sim_inst(list->arr[i]);
    }
    free(list->arr);
    free(list);
}


void print_sim_inst_list(sim_inst_list_t *list, single_list_t *inst_list, int num_ports, char *arch_name, int num_iterations) {
    printf("\nAnalysis for architecture: %s\n\n", arch_name);
    printf(" Line  ||   Num   ||   had   || caused  ||            Used Ports\n");
    printf("       ||   Uops  || to wait || to wait ||");
    for (int i = 0; i < num_ports; ++i) {
        printf("   %i   ||", i);
    }
    printf("\n");
    for (int i = 0; i < (num_ports == 6 ? 96 : 114); ++i) {
        printf("-");
    }
    printf("\n");
    sim_inst_t *inst = NULL;
    char buffer[XED_TMP_BUF_LEN];
    for (int i = 0; i < list->size; ++i) {
        inst = list->arr[i];
        disassemble(buffer, XED_TMP_BUF_LEN, &inst_list->array[i],
                    inst_list->printinfo[i]);
        print_conditional_spaces(i);
        printf(" %i   ||", i);
        print_conditional_spaces(inst->num_micro_ops);
        printf("  %i    ||", inst->num_micro_ops);
        print_conditional_spaces(inst->cycles_delayed);
        if (inst->cycles_delayed > 0)
            printf("  %.2f    ||", ((double)inst->cycles_delayed)/num_iterations);
        else
            printf("       ||");
        print_conditional_spaces(inst->delayed_cycles);
        if (inst->delayed_cycles > 0)
            printf("  %.2f    ||", ((double)inst->delayed_cycles)/num_iterations);
        else
            printf("       ||");
        for (int j = 0; j < num_ports; ++j) {
            print_conditional_spaces(inst->used_ports[j]);
            if (inst->used_ports[j]>0)
                printf(" %.2f   ||", ((double)inst->used_ports[j])/num_iterations);
            else
                printf("     ||");
        }
        printf(" %s\n", buffer);
    }
}


void print_conditional_spaces(int i) {
    if (i < 100) {
        if (i < 10) {
            printf(" ");
        }
        printf(" ");
    }
}


void print_sim_inst_details(sim_inst_list_t *list, single_list_t *inst_list, int line, int num_ports, int num_iterations) {
    char buffer[XED_TMP_BUF_LEN];
    bool is_delayed = false;
    disassemble(buffer, XED_TMP_BUF_LEN, &inst_list->array[line],
                inst_list->printinfo[line]);
    printf("\nDetailed delay information for instruction: %s in line %i\n\n", buffer, line);
    printf("Delay caused by dependencies:\n");
    delays_t *delays = list->arr[line]->dep_delays;
    for (int i = 0; i < list->size; ++i) {
        if (delays[i].delay_caused || delays[i].delay_suffered) {
            if (!is_delayed) {
                printf("  Line || was delayed || has delayed\n");
                printf(" ----------------------------------\n");
                is_delayed = true;
            }
            print_conditional_spaces(i);
            printf(" %i   ||", i);
            print_conditional_spaces(delays[i].delay_caused);
            printf("    %.2f      ||", ((double)delays[i].delay_caused)/num_iterations);
            print_conditional_spaces(delays[i].delay_suffered);
            printf("   %.2f      \n", ((double)delays[i].delay_suffered)/num_iterations);
        }
    }
    if (!is_delayed) {
        printf(" None!\n");
    } else {
        is_delayed = false;
    }
    printf("\n\nDelay caused by blocked ports:\n");
    delays = list->arr[line]->port_delays;
    for (int i = 0; i < num_ports; ++i) {
        if (delays[i].delay_caused || delays[i].delay_suffered) {
            if (!is_delayed) {
                printf("  Port || was delayed || has delayed\n");
                printf(" ----------------------------------\n");
                is_delayed = true;
            }
            print_conditional_spaces(i);
            printf(" %i   ||", i);
            print_conditional_spaces(delays[i].delay_caused);
            printf("    %.2f      ||", ((double)delays[i].delay_caused)/num_iterations);
            print_conditional_spaces(delays[i].delay_suffered);
            printf("   %.2f      \n", ((double)delays[i].delay_suffered)/num_iterations);
        }
    }
    if (!is_delayed) {
        printf(" None!\n");
    }
    printf("\n");
}


void clear_father_from_list(sim_inst_t *si, int father_id) {
    for (int i = 0; i < si->fathers_todo; ++i) {
        if (si->fathers[i]->id == father_id) {
            si->fathers[i] = si->fathers[--si->fathers_todo];
            break;
        }
    }
}