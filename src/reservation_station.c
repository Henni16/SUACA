#include "reservation_station.h"
#include "xmlParser.h"
#include "inst_list.h"
#include "graph.h"


station_t *create_initial_state(graph_t *dependencies, single_list_t *insts) {
    inst_info_t **table_info = parse_instruction_file(TABLE, ARCHITECTURE_NAME);
    station_t *station = parse_station_file(STATION_FILE);
    sim_inst_t *cur;
    sim_inst_t *prev = NULL;
    sim_inst_t *all[insts->size];
    //build all sim insts
    for (int i = 0; i < insts->size; i++) {
        int index = xed_decoded_inst_get_iform_enum(&insts->array[i]);
        inst_info_t *info = table_info[index];
        cur = newSimInst(i, info->usable_ports, info->num_micro_ops, dependencies->nodes[i]->num_fathers,
                         get_max_latency(info->latencies), dependencies->nodes[i]->num_successors);
        all[i] = cur;
        cur->previous = prev;
        if (prev != NULL)
            prev->next = cur;
        else {
            station->station_queue = cur;
            station->wait_queue = cur;
        }
        prev = cur;
    }
    //build dependencies
    for (int j = 0; j < insts->size; ++j) {
        int index = xed_decoded_inst_get_iform_enum(&insts->array[j]);
        inst_info_t *info = table_info[index];
        cur = all[j];
        for (int i = 0; i < cur->num_dep_children; ++i) {
            int_reg_tuple_t intreg = dependencies->nodes[j]->successors[i];
            cur->dep_children[i] = (reg_sim_inst_t) {all[intreg.line],
                                                     get_latency_for_register(info->latencies, intreg.reg)};
        }
    }
    free_info_array(table_info);
}

void perform_cycle(station_t *station) {
    load_instruction_into_station(station);
    put_executables_into_ports(station);
    execute_instructions_in_ports(station);
}


void execute_instructions_in_ports(station_t *station) {
    port_t *port;
    port_t *prev = NULL;
    for (size_t i = 0; i < station->num_ports; i++) {
        port = station->ports[i];
        while (port->inst->line >= 0) {
            port->availiable = true;
            port->num_cycles_in_port++;
            inform_children_im_done(port->inst, port->num_cycles_in_port);
            if (port->num_cycles_in_port == port->inst->latency) {
                if (prev != NULL)
                    prev->next = port->next;
                add_to_sim_list(station->done_insts, port->inst);
                freePort(port);
            } else {
                prev = port;
            }
            port = port->next;
        }
    }
}

void put_executables_into_ports(station_t *station) {
    sim_inst_t *cur = station->station_queue;
    while (cur != NULL && cur->micro_ops_loaded == cur->num_micro_ops) {
        if (!all_fathers_done(cur)) {
            cur->cycles_delayed++;
        } else {
            bool fits = false;
            for (size_t i = 0; i < station->num_ports && !fits; i++) {
                if (cur->usable_ports[i]) {
                    if (station->ports[i]->availiable) {
                        station->ports[i] = newPort(cur, station->ports[i]);
                        fits = true;
                        delete_inst_from_queue(cur);
                    }
                }
            }
            if (!fits) {
                cur->cycles_delayed++;
                for (size_t i = 0; i < station->num_ports; i++) {
                    if (cur->usable_ports[i])
                        station->ports[i]->inst->delayed_cycles++;
                }
            }
        }
        cur = cur->next;
    }
}

void load_instruction_into_station(station_t *station) {
    int loadable;
    if (station->cap - station->size < station->load_per_cycle)
        loadable = station->cap - station->size;
    else
        loadable = station->load_per_cycle;
    sim_inst_t *next = station->wait_queue;
    while (loadable > 0) {
        //all instruct of next can be loaded
        if (get_loadable_micro_ops(next) <= loadable) {
            loadable -= get_loadable_micro_ops(next);
            next->micro_ops_loaded = next->num_micro_ops;
            station->wait_queue = next->next;
            next = next->next;
        }
            //instruction can't be fully loaded
        else {
            next->micro_ops_loaded += loadable;
            loadable = 0;
        }
    }
}

void delete_inst_from_queue(sim_inst_t *inst) {
    inst->previous->next = inst->next;
    inst->next->previous = inst->previous;
}

port_t *newPort(sim_inst_t *inst, port_t *next) {
    port_t *port = (port_t *) malloc(sizeof(port_t));
    port->inst = inst;
    port->next = next;
    port->num_cycles_in_port = 0;
    port->availiable = false;
    return port;
}


void freePort(port_t *port) {
    free(port);
}


void freeStation(station_t *station) {
    free(station->ports);
    free(station);
}

void inform_children_im_done(sim_inst_t *inst, int cycles_done) {
    for (size_t i = 0; i < inst->num_dep_children; i++) {
        if (inst->dep_children[i].cycles == cycles_done)
            inst->dep_children[i].child->fathers_todo--;
    }
}


void printStation(station_t *s) {
    printf("wait queue:\n");
    sim_inst_t *cur = s->wait_queue;
    while (cur) {
        printf("line: %i loaded: %i\n",
               cur->line, cur->micro_ops_loaded);
        cur = cur->next;
    }
    cur = s->station_queue;
    printf("station queue:\n");
    while (cur && cur->micro_ops_loaded > 0) {
        printf("line: %i loaded: %ii\n",
               cur->line, cur->micro_ops_loaded);
        cur = cur->next;
    }
    printf("done list:\n");
    for (size_t i = 0; i < s->done_insts->size; i++) {
        if (s->done_insts->arr[i])
            printf("%i, ", i);
    }
    printf("\n");
    for (size_t i = 0; i < s->num_ports; i++) {
        printf("port %s: ", i);
        printPort(s->ports[i]);
        printf("\n");
    }
}


void printPort(port_t *p) {
    printf("line: %i cycles in port: %i", p->inst->line, p->num_cycles_in_port);
    if (p->next) {
        printf(" -> ");
        printPort(p->next);
    }
}
