#include "reservation_station.h"
#include "inst_list.h"
#include "graph.h"
#include "hashset.h"
#include "xmlParser.h"


bool *compute_needed_instructions(graph_t *cfg, int branch, single_list_t *list) {
    bool *needed = calloc(cfg->size, sizeof(bool));
    if (branch == -1) {
        for (int i = 0; i < cfg->size; ++i) {
            needed[i] = true;
        }
        return needed;
    }
    bool branched = false;
    int cur_line = 0;
    node_t *cur = cfg->nodes[cur_line];
    while (cur) {
        if (!branched && cur->num_successors > 1) {
            branched = true;
            cur_line = cur->successors[branch].line;
        } else {
            if (!is_branch_instruction(&list->array[cur_line]))
                needed[cur_line] = true;
            if (cur->num_successors)
                cur_line = cur->successors[0].line;
            else
                break;
        }
        cur = cfg->nodes[cur_line];
    }
    return needed;
}

void create_initial_state(graph_t *dependencies, single_list_t *insts, int num_iterations, station_t *station,
                          inst_info_t **table_info, bool print_unsupported, int branch, graph_t *cfg) {
    reset_global_id();
    sim_inst_t *cur;
    sim_inst_t *prev = NULL;
    sim_inst_t **all =  malloc(station->num_insts * num_iterations * sizeof(sim_inst_t*));
    station->done_insts = newSimInstList(station->num_insts);
    bool *needed = compute_needed_instructions(cfg, branch, insts);
    //build all sim insts
    for (int i = 0; i < station->num_insts * num_iterations; i++) {
        int mod_i = i % station->num_insts;
        if (!needed[mod_i]) {
            all[i] = newSimInst(mod_i, NULL, 0, 0, 0, 0, 0, 0, 0);
            all[i]->not_needed = true;
            station->done_insts->arr[mod_i] = all[i];
            continue;
        }
        // for every iteration after the first we have to consider the second part of the dep_graph
        // to find the fathers
        int father_i = i < station->num_insts ? mod_i : mod_i + station->num_insts;
        int index = xed_decoded_inst_get_iform_enum(&insts->array[mod_i]);
        inst_info_t *info = table_info[index];
        if (info == NULL) {
            if (!station->done_insts->arr[mod_i] && print_unsupported)
                printf("Unsupported instruction found in line: %i  instruction was: %s\n", i,
                       xed_iform_enum_t2str(index));
            all[i] = newSimInst(mod_i, NULL, 0, 0, 0, 0, 0, 0, 0);
            all[i]->unsupported = true;
            station->done_insts->arr[mod_i] = all[i];
            continue;
        }
        //needs to be incremented because we create a new reference to this struct (we'll copy for now)
        //info->micro_ops->numrefs++;
        cur = newSimInst(mod_i, info->micro_ops, info->num_micro_ops,
                         dependencies->nodes[father_i]->num_fathers,
                         get_max_latency(info->latencies), dependencies->nodes[mod_i]->num_successors,
                         station->num_ports, station->num_insts, info->div_cycles);
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
                if (all[(intreg.line + (k * station->num_insts))]->unsupported || all[(intreg.line + (k * station->num_insts))]->not_needed) {
                    to_sub++;
                    continue;
                }
                cur->dep_children[i-to_sub] = (reg_sim_inst_t) {all[intreg.line + (k * station->num_insts)],
                                                         get_latency_for_register(info->latencies, intreg.reg)};
            }
            cur->num_dep_children -= to_sub;
            int added = k > 0 ? (k - 1) * station->num_insts : 0;
            int second_j = k > 0 ? j + station->num_insts : j;
            to_sub = 0;
            for (int i = 0; i < cur->fathers_todo; ++i) {
                if (all[dependencies->nodes[second_j]->fathers[i].line + added]->unsupported || all[dependencies->nodes[second_j]->fathers[i].line + added]->not_needed) {
                    to_sub++;
                    continue;
                }
                cur->fathers[i-to_sub] = all[dependencies->nodes[second_j]->fathers[i].line + added];
            }
            cur->fathers_todo -= to_sub;
        }
    }
    station->to_exec = NULL;
    free(needed);
    free(all);
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
    while (cur != NULL && (cur->micro_ops_loaded > 0 || !cur->num_micro_ops)) {
        if (!cur->num_micro_ops) {
            execute_list_add(&station->to_exec, cur);
            cur = cur->next;
            continue;
        }
        if (!station->no_dependencies && !all_fathers_done(cur)) {
            cur->cycles_delayed++;
            for (int i = 0; i < cur->fathers_todo; ++i) {
                cur->fathers[i]->delayed_cycles++;
                cur->fathers[i]->dep_delays[cur->line].delay_caused++;
                cur->dep_delays[cur->fathers[i]->line].delay_suffered++;
            }
        } else {
            bool fits_single;
            bool fits = true;
            bool blame_div = false;
            port_ops_t *po = cur->micro_ops;
            hashset_t *would_like_to_use = new_hashset(station->num_ports);
            int will_use[station->num_ports];
            while (po) {
                fits_single = true;
                // instruction is not fully loaded
                if (po->numops) {
                    fits = false;
                }
                // this part is already in the port pipeline
                if (!po->loaded_ops) {
                    po = po->next;
                    continue;
                }
                int arr_len = 0;
                hashset_t *blamable = new_hashset(station->num_ports);
                // check which ports are possible
                for (int i = 0; i < station->num_ports; ++i) {
                    if (po->usable_ports[i] && !station->ports[i]) {
                        // instruction wants to use the div pipe, but it is blocked
                        // !blame_div because we only consider a single uop as the div op
                        if (!i && cur->div_cycles && !cur->executed_div && station->div_port && !blame_div && station->div_port->id != cur->id) {
                            fits_single = false;
                            blame_div = true;
                            continue;
                        }
                        insert_sorted(will_use, &arr_len, i, station);
                    } else if (po->usable_ports[i] && cur->id != station->ports[i]->id) {
                        fits_single = false;
                        insert_into_hashset(blamable, i);
                    }
                }
                // assign the microops to the least used ports
                for (int i = 0; i < arr_len && po->loaded_ops; ++i) {
                    if (!station->non_blocking_ports) {
                        if (!will_use[i] && cur->div_cycles && !cur->executed_div && !station->div_port) {
                            station->div_port = cur;
                        }
                        station->ports[will_use[i]] = cur;
                    }
                    cur->used_ports[will_use[i]]++;
                    station->port_usage[will_use[i]]++;
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
                        if (hashset_contains(blamable, i)) {
                            insert_into_hashset(would_like_to_use, i);
                        }
                    }
                }
                hashset_free(blamable);
                po = po->next;
            }
            if (fits) {
                execute_list_add(&station->to_exec, cur);
            } else {
                // we need this because one instruction might block several ports and we want to count the number
                // of cycles an instruction caused a delay
                hashset_t *already_blamed = new_hashset(station->num_ports);
                for (int i = 0; i < station->num_ports; ++i) {
                    if (hashset_contains(would_like_to_use, i) && !hashset_contains(already_blamed, station->ports[i]->id)) {
                        station->ports[i]->delayed_cycles++;
                        cur->port_delays[i].delay_suffered++;
                        station->ports[i]->port_delays[i].delay_caused++;
                        insert_into_hashset(already_blamed, station->ports[i]->id);
                    }
                }
                if (blame_div) {
                    station->div_port->div_delayed_cycles++;
                    station->div_port->delayed_cycles++;
                    cur->div_cycles_delayed++;
                }
                if (blame_div || already_blamed->num) {
                    cur->cycles_delayed++;
                }
                hashset_free(already_blamed);
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
        if (cur->div_cycles && station->div_port && station->div_port->id == cur->id && ++cur->executed_div >= cur->div_cycles) {
            station->div_port = NULL;
        }
        inform_children_im_done(cur, ++cur->executed_cycles);
        if (cur->executed_cycles >= cur->latency) {
            delete_inst_from_queue(cur, station);
            add_to_sim_list(station->done_insts, cur, station->num_ports, station->num_insts);
        }
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
        if (inst->dep_children[i].cycles == cycles_done || !inst->dep_children[i].cycles) {
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