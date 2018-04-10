#include "reservation_station.h"
#include "xmlParser.h"
#include "inst_list.h"
#include "graph.h"
#include "hashset.h"


station_t *create_initial_state(graph_t *dependencies, single_list_t *insts) {
    station_t *station = parse_station_file(STATION_FILE);
    bool fail = false;
    if (station == NULL) {
        return NULL;
    }
    hashset_t *set = create_hashset(insts);
    inst_info_t **table_info = parse_instruction_file(TABLE, ARCHITECTURE_NAME, station->num_ports, set);
    hashset_free(set);
    if (table_info == NULL) {
        return NULL;
    }
    sim_inst_t *cur;
    sim_inst_t *prev = NULL;
    sim_inst_t *all[insts->size];
    //build all sim insts
    for (int i = 0; i < insts->size; i++) {
        int index = xed_decoded_inst_get_iform_enum(&insts->array[i]);
        inst_info_t *info = table_info[index];
        if (info == NULL) {
            fail = true;
            printf("Unsupported instruction found in line: %i  instruction was: %s\n", i + 1,
                   xed_iform_enum_t2str(index));
            continue;
            //return NULL;
        }
        //needs to be incremented because we create a new reference to this struct (we'll copy for now)
        //info->micro_ops->numrefs++;
        cur = newSimInst(i, info->micro_ops, info->num_micro_ops, dependencies->nodes[i]->num_fathers,
                         get_max_latency(info->latencies, index), dependencies->nodes[i]->num_successors,
                         station->num_ports);
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
    if (fail) {
        free_info_array(table_info);
        freeStation(station);
        return NULL;
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
    station->done_insts = newSimInstList(insts->size);
    return station;
}

void perform_cycle(station_t *station) {
    load_instruction_into_station(station);
    put_executables_into_ports(station);
    //execute_instructions_in_ports(station);
    for (int i = 0; i < station->num_ports; ++i) {
        station->ports[i] = NULL;
    }
}



void put_executables_into_ports(station_t *station) {
    sim_inst_t *cur = station->station_queue;
    while (cur != NULL && cur->micro_ops_loaded > 0) {
        if (!all_fathers_done(cur)) {
            cur->cycles_delayed++;
        } else {
            bool fits_single;
            bool fits = true;
            port_ops_t *po = cur->micro_ops;
            hashset_t *would_like_to_use = new_hashset(station->num_ports);
            while (po) {
                fits_single = true;
                for (int i = 0; i < station->num_ports && po->loaded_ops; ++i) {
                    if (po->usable_ports[i] && !station->ports[i]) {
                        station->ports[i] = cur;
                        cur->used_ports[i]++;
                        po->loaded_ops--;
                        station->size--;
                    } else if (po->usable_ports[i]) {
                        fits_single = false;
                    }
                }
                if (!po->loaded_ops) {
                    po = po->next;
                    continue;
                }
                if (!fits_single) {
                    fits = false;
                    for (int i = 0; i < station->num_ports; ++i) {
                        if (po->usable_ports[i] && station->ports[i]->line != cur->line) {
                            insert_into_hashset(would_like_to_use, i);
                        }
                    }
                }
                po = po->next;
            }
            if (fits) {
                if (++cur->executed_cycles == cur->latency) {
                    delete_inst_from_queue(cur, station);
                    add_to_sim_list(station->done_insts, cur);
                }
                inform_children_im_done(cur, cur->executed_cycles);
            } else {
                for (int i = 0; i < station->num_ports; ++i) {
                    if (hashset_contains(would_like_to_use, i)) {
                        station->ports[i]->delayed_cycles++;
                    }
                }
                cur->cycles_delayed++;
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
    station->size += loadable;
    while (loadable > 0 && next) {
        //all instruct of next can be loaded
        if (get_loadable_micro_ops(next) <= loadable) {
            loadable -= get_loadable_micro_ops(next);
            load_num_micro_ops(next, get_loadable_micro_ops(next));
            next->micro_ops_loaded = next->num_micro_ops;
            station->wait_queue = next->next;
            next = next->next;
        }
            //instruction can't be fully loaded
        else {
            next->micro_ops_loaded += loadable;
            load_num_micro_ops(next, loadable);
            loadable = 0;
        }
    }
}

void delete_inst_from_queue(sim_inst_t *inst, station_t *station) {
    if (!inst->next && !inst->previous) {
        station->station_queue = NULL;
    } else if (!inst->previous) {
        inst->next->previous = NULL;
        station->station_queue = inst->next;
    } else if (!inst->next) {
        inst->previous->next = NULL;
    } else {
        inst->previous->next = inst->next;
        inst->next->previous = inst->previous;
    }
}



void freeStation(station_t *station) {
    free_sim_inst_list(station->done_insts);
    /*for (int i = 0; i < station->num_ports; ++i) {
        freePort(station->ports[i]);
    }*/
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
        printf("line: %i loaded: %i\n",
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
        printf("port %i: ", i);
        //printPort(s->ports[i]);
        printf("\n");
    }
    printf("\n\n================================================\n\n\n");
}


