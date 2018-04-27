#include "reservation_station.h"
#include "inst_list.h"
#include "graph.h"
#include "hashset.h"
#include "xmlParser.h"


void create_initial_state(graph_t *dependencies, single_list_t *insts, int num_iterations, station_t *station,
                          inst_info_t **table_info, bool print_unsupported) {
    sim_inst_t *cur;
    sim_inst_t *prev = NULL;
    sim_inst_t *all[station->num_insts * num_iterations];
    station->done_insts = newSimInstList(station->num_insts);
    //build all sim insts
    for (int i = 0; i < station->num_insts * num_iterations; i++) {
        int mod_i = i % station->num_insts;
        // for every iteration after the first we have to consider the second part of the dep_graph
        // to find the fathers
        int father_i = i < station->num_insts ? mod_i : mod_i + station->num_insts;
        int index = xed_decoded_inst_get_iform_enum(&insts->array[mod_i]);
        inst_info_t *info = table_info[index];
        if (info == NULL) {
            if (!station->done_insts->arr[mod_i] && print_unsupported)
                printf("Unsupported instruction found in line: %i  instruction was: %s\n", i + 1,
                       xed_iform_enum_t2str(index));
            all[i] = newSimInst(mod_i, NULL, 0, 0, 0, 0, 0, 0);
            all[i]->unsupported = true;
            station->done_insts->arr[mod_i] = all[i];
            continue;
        }
        //needs to be incremented because we create a new reference to this struct (we'll copy for now)
        //info->micro_ops->numrefs++;
        cur = newSimInst(mod_i, info->micro_ops, info->num_micro_ops,
                         dependencies->nodes[father_i]->num_fathers,
                         get_max_latency(info->latencies), dependencies->nodes[mod_i]->num_successors,
                         station->num_ports, station->num_insts);
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
    for (int k = 0; k < num_iterations; ++k) {
        for (int j = 0; j < station->num_insts; ++j) {
            int index = xed_decoded_inst_get_iform_enum(&insts->array[j]);
            inst_info_t *info = table_info[index];
            cur = all[j + (k * station->num_insts)];
            int to_sub = 0;
            for (int i = 0; i < cur->num_dep_children; ++i) {
                int_reg_tuple_t intreg = dependencies->nodes[j]->successors[i];
                if ((intreg.line + (k * station->num_insts)) >= num_iterations * station->num_insts) {
                    to_sub++;
                    continue;
                }
                if (all[(intreg.line + (k * station->num_insts))]->unsupported) {
                    to_sub++;
                    continue;
                }
                cur->dep_children[i] = (reg_sim_inst_t) {all[intreg.line + (k * station->num_insts)],
                                                         get_latency_for_register(info->latencies, intreg.reg)};
            }
            cur->num_dep_children -= to_sub;
            int added = k > 0 ? (k - 1) * station->num_insts : 0;
            int second_j = k > 0 ? j + station->num_insts : j;
            to_sub = 0;
            for (int i = 0; i < cur->fathers_todo; ++i) {
                if (all[dependencies->nodes[second_j]->fathers[i].line + added]->unsupported) {
                    to_sub++;
                    continue;
                }
                cur->fathers[i] = all[dependencies->nodes[second_j]->fathers[i].line + added];
            }
            cur->fathers_todo -= to_sub;
        }
    }
    station->to_exec = NULL;
}

void perform_cycle(station_t *station) {
    load_instruction_into_station(station);
    put_executables_into_ports(station);
    execute_instructions_in_ports(station);
    for (int i = 0; i < station->num_ports; ++i) {
        station->ports[i] = NULL;
    }
}


void put_executables_into_ports(station_t *station) {
    sim_inst_t *cur = station->station_queue;
    while (cur != NULL && cur->micro_ops_loaded > 0) {
        if (!all_fathers_done(cur)) {
            cur->cycles_delayed++;
            for (int i = 0; i < cur->fathers_todo; ++i) {
                cur->fathers[i]->delayed_cycles++;
                cur->fathers[i]->dep_delays[cur->line].delay_caused++;
                cur->dep_delays[cur->fathers[i]->line].delay_suffered++;
            }
        } else {
            bool fits_single;
            bool fits = true;
            port_ops_t *po = cur->micro_ops;
            hashset_t *would_like_to_use = new_hashset(station->num_ports);
            int will_use[station->num_ports];
            while (po) {
                fits_single = true;
                // instruction is not fully loaded
                if (po->numops) {
                    fits = false;
                }
                int arr_len = 0;
                // check which ports are possible
                for (int i = 0; i < station->num_ports; ++i) {
                    if (po->usable_ports[i] && !station->ports[i]) {
                        insert_sorted(will_use, &arr_len, i, station);
                    } else if (po->usable_ports[i]) {
                        fits_single = false;
                    }
                }
                // assign the microops to the least used ports
                for (int i = 0; i < arr_len && po->loaded_ops; ++i) {
                    if (!station->non_blocking_ports) {
                        station->ports[will_use[i]] = cur;
                        cur->used_ports[will_use[i]]++;
                        station->port_usage[will_use[i]]++;
                    }
                    po->loaded_ops--;
                    station->size--;
                }
                if (!po->loaded_ops) {
                    po = po->next;
                    continue;
                }
                if (!fits_single) {
                    fits = false;
                    for (int i = 0; i < station->num_ports; ++i) {
                        if (po->usable_ports[i] && station->ports[i]->id != cur->id) {
                            insert_into_hashset(would_like_to_use, i);
                        }
                    }
                }
                po = po->next;
            }
            if (fits) {
                execute_list_add(&station->to_exec, cur);
            } else {
                for (int i = 0; i < station->num_ports; ++i) {
                    if (hashset_contains(would_like_to_use, i)) {
                        station->ports[i]->delayed_cycles++;
                        cur->port_delays[i].delay_suffered++;
                        station->ports[i]->port_delays[i].delay_caused++;
                    }
                }
                cur->cycles_delayed++;
            }
            hashset_free(would_like_to_use);
        }
        cur = cur->next;
    }
}


void execute_instructions_in_ports(station_t *station) {
    execute_list_t *list = station->to_exec;
    sim_inst_t *cur;
    while (list) {
        cur = list->elem;
        if (++cur->executed_cycles == cur->latency) {
            delete_inst_from_queue(cur, station);
            add_to_sim_list(station->done_insts, cur, station->num_ports, station->num_insts);
        }
        inform_children_im_done(cur, cur->executed_cycles);
        list = list->next;
    }
    execute_list_clear(&station->to_exec);
}

void load_instruction_into_station(station_t *station) {
    int loadable;
    if (station->cap - station->size < station->load_per_cycle)
        loadable = station->cap - station->size;
    else
        loadable = station->load_per_cycle;
    sim_inst_t *next = station->wait_queue;
    while (loadable > 0 && next) {
        //all instruct of next can be loaded
        if (get_loadable_micro_ops(next) <= loadable) {
            loadable -= get_loadable_micro_ops(next);
            station->size += get_loadable_micro_ops(next);
            load_num_micro_ops(next, get_loadable_micro_ops(next));
            next->micro_ops_loaded = next->num_micro_ops;
            station->wait_queue = next->next;
            next = next->next;
        }
            //instruction can't be fully loaded
        else {
            next->micro_ops_loaded += loadable;
            station->size += loadable;
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
    if (station->done_insts)
        free_sim_inst_list(station->done_insts);
    free(station->ports);
    free(station->port_usage);
    free(station);
}

void inform_children_im_done(sim_inst_t *inst, int cycles_done) {
    for (size_t i = 0; i < inst->num_dep_children; i++) {
        if (inst->dep_children[i].cycles == cycles_done) {
            clear_father_from_list(inst->dep_children[i].child, inst->id);
        }
    }
}


void execute_list_add(execute_list_t **list, sim_inst_t *to_add) {
    execute_list_t *new = malloc(sizeof(execute_list_t));
    new->elem = to_add;
    new->next = NULL;
    if (!*list) {
        *list = new;
    } else {
        while ((*list)->next) {
            list = &(*list)->next;
        }
        (*list)->next = new;
    }
}

void execute_list_clear(execute_list_t **list) {
    execute_list_t *next;
    execute_list_t *cur = *list;
    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
    *list = NULL;
}


void build_station_file_string(char *dest, const char *arch_name) {
    int st_loc_len = strlen(STATION_LOC);
    int a_len = strlen(arch_name);
    for (int i = 0; i < st_loc_len; ++i) {
        dest[i] = STATION_LOC[i];
    }
    for (int i = st_loc_len; i < st_loc_len + a_len; ++i) {
        dest[i] = arch_name[i - st_loc_len];
    }
    char *a = ".arch";
    for (int i = st_loc_len + a_len; i < st_loc_len + a_len + strlen(a); ++i) {
        dest[i] = a[i - st_loc_len - a_len];
    }
    dest[st_loc_len + a_len + strlen(a)] = '\0';
}


int compute_total_num_microops(station_t *station) {
    sim_inst_t *cur = station->station_queue;
    int num = 0;
    do {
        num += cur->num_micro_ops;
        cur = cur->next;
    } while (cur && cur->line > 0);
    return num;
}


void insert_sorted(int *arr, int *len, int val, station_t *station) {
    int tmp;
    for (int i = 0; i < *len; ++i) {
        if (station->port_usage[arr[i]] > station->port_usage[val]) {
            tmp = val;
            val = arr[i];
            arr[i] = tmp;
        }
    }
    if (*len < station->num_ports) {
        arr[(*len)++] = val;
    }
}