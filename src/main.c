#include "xed.h"
#include "dependency_analysis.h"
#include "xmlParser.h"
#include <time.h>
#include "reservation_station.h"
#include "hashmap.h"
#include "inst_list.h"
#include "graph.h"

int build_cfg;
int build_dep_graph;
int print_help;
int branch = 0;
bool print_setup = false;
bool perfomance = false;
bool no_deps = false;
bool perfect_frontend = false;
bool infinite_ports = false;
int num_iterations = 0;
int detail_line = -1;
char *setup_arch;
int setup_loop = 0;
char *invalid_flag;
char *file_name;
char *arch_name;
station_t *station;
station_t *frontend_test;
int frontend_cycles;
station_t *port_test;
int port_cycles;
station_t *dep_test;
int dep_cycles;
inst_info_t **table_info;

void clp(int argc, char *argv[]);

void graphs_and_map(single_list_t *list, int index, int branch);

void help();

int main(int argc, char *argv[]) {
    clock_t start = clock();
    clp(argc, argv);
    if (print_help) {
        help();
        return 0;
    }
    if (setup_loop && setup_arch) {
        create_setup_file(CONFIG_LOC, setup_arch, setup_loop);
        return 0;
    } else {
        bool success = parse_setup_file(CONFIG_LOC, &setup_arch, &setup_loop);
        if (!success) {
            printf("Couldn't find suaca.config at location %s\nAbort!\n", CONFIG_LOC);
            return 1;
        }
        if (print_setup) {
            printf("Default architecture: %s\nDefault number of iterations: %i\n", setup_arch, setup_loop);
            return 0;
        }
        if (!arch_name) {
            arch_name = setup_arch;
        }
        if (num_iterations == 0) {
            num_iterations = setup_loop;
        }
    }
    if (invalid_flag != NULL) {
        printf("Invalid flag: %s\nValid ", invalid_flag);
        help();
        return 0;
    }
    if (file_name == NULL) {
        printf("Please give me a file to work on\n Use --help to see options");
        return 0;
    }

    inst_list_t *instructions = build_inst_list(file_name);
    print_list(instructions);

    for (size_t i = 0; i < instructions->numLists; i++) {
        if (branch) {
            printf("Left branch analysis:\n\n");
            graphs_and_map(instructions->lists[i], i, 0);
            printf("\n\nRight branch analysis:\n\n");
            graphs_and_map(instructions->lists[i], i, 1);
        } else
            graphs_and_map(instructions->lists[i], i, -1);
        free_info_array(table_info);
        if (i + 1 < instructions->numLists)
            printf("\n\n================================================\n\n\n");
    }
    free_list(instructions);
    free(arch_name);
    printf("time: %f", (clock() - start) / (double) CLOCKS_PER_SEC);
    return 0;
}

void help() {
    printf("options:\n");
    printf(" -cfg:            build controlflowgraph\n");
    printf(" -dg:             build dependencygraph\n");
    printf(" -p:              run in \"performance mode\"\n");
    printf(" --arch [x]:      [x] is architecture the analysis is based on\n");
    printf(" --detail [x]:    detailed delay info for line [x]\n");
    printf(" --loop [x]:      run the analysis in a loop of [x]\n");
    printf(" --setup [x] [y]: set [x]/[y] as the default architecture/number of iterations\n");
    printf(" --print-default: prints the default values for architecture/number of iterations\n");
}

int perform_simulation(station_t *station, single_list_t *list, bool print) {
    if (!station) {
        printf("Couldn't create station!\n");
        return 0;
    }
    int total_num_microops = compute_total_num_microops(station) * num_iterations;
    int num_cycles = 0;
    // perform computations until both queues are empty and no instruction is be executed (to_exec)
    while (station->wait_queue || station->station_queue || station->to_exec) {
        perform_cycle(station);
        num_cycles++;
    }
    if (detail_line == -1 && print && !perfomance) {
        print_sim_inst_list(station->done_insts, list, station->num_ports, arch_name, num_iterations, num_cycles,
                            total_num_microops, frontend_cycles, port_cycles, dep_cycles);
    } else if (print && detail_line != -1)
        print_sim_inst_details(station->done_insts, list, detail_line, station->num_ports, num_iterations);
    else if (print) {
        print_sim_inst_list(station->done_insts, list, station->num_ports, arch_name, num_iterations, num_cycles,
                            total_num_microops, -1, -1, -1);
    }
    freeStation(station);
    return num_cycles;
}

void validate_table(single_list_t *insts, inst_info_t **table);

int parse_stuff(single_list_t *insts) {
    char station_file[strlen(arch_name) + strlen(STATION_LOC) + strlen(".arch") + 1];
    build_station_file_string(station_file, arch_name);
    station = parse_station_file(station_file, num_iterations, insts->single_loop_size);
    if (station == NULL) {
        printf("The station file for %s could not be found\n", arch_name);
        return 0;
    }
    if (perfect_frontend) {
        station->load_per_cycle = station->cap;
    }
    station->no_dependencies = no_deps;
    station->non_blocking_ports = infinite_ports;
    if (!perfomance) {
        frontend_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
        frontend_test->load_per_cycle = frontend_test->cap;
        port_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
        port_test->non_blocking_ports = true;
        dep_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
        dep_test->no_dependencies = true;
    }
    if (!table_info) {
        hashset_t *set = create_hashset(insts);
        printf("Parsing measurement file...\n");
        table_info = parse_instruction_file(TABLE, arch_name, station->num_ports, set);
        validate_table(insts, table_info);
        printf("Done parsing!\n");
        hashset_free(set);
        if (table_info == NULL) {
            return 0;
        }
    }
}

void validate_table(single_list_t *insts, inst_info_t **table) {
    inst_info_t *info;
    port_ops_t *po;
    int sum;
    for (int i = 0; i < insts->single_loop_size; ++i) {
        info = table[xed_decoded_inst_get_iform_enum(&insts->array[i])];
        if (!info) continue;
        po = info->micro_ops;
        sum = 0;
        while (po) {
            sum += po->numops;
            po = po->next;
        }
        if (sum != info->num_micro_ops) {
            printf("Warning! Given number of Uops for instruction %s was %i, but sum of Uops was %i!\n",
                   xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(&insts->array[i])), info->num_micro_ops, sum);
            info->num_micro_ops = sum;
        }
    }
}

void graphs_and_map(single_list_t *list, int index, int branch) {
    if (num_iterations > 1 && branch != 1) {
        if (!add_loop_instructions(list)) {
            printf("Aborting analysis!\n");
        }
    } else if (branch != 1) {
        list->single_loop_size = list->size;
    }
    graph_t *g = build_controlflowgraph(list);
    if (build_cfg)
        build_graphviz(g, list, "controlflow", index);
    graph_t *dg = build_dependencygraph_cfg(list, g);
    if (build_dep_graph)
        build_graphviz(dg, list, "dependency", index);
    if (!parse_stuff(list)) {
        return;
    }
    if (detail_line == -1 && !perfomance) {
        create_initial_state(dg, list, num_iterations, frontend_test, table_info, false, branch, g);
        frontend_cycles = perform_simulation(frontend_test, list, false);
        create_initial_state(dg, list, num_iterations, port_test, table_info, false, branch, g);
        port_cycles = perform_simulation(port_test, list, false);
        create_initial_state(dg, list, num_iterations, dep_test, table_info, false, branch, g);
        dep_cycles = perform_simulation(dep_test, list, false);
    }
    create_initial_state(dg, list, num_iterations, station, table_info, true, branch, g);
    free_graph(g);
    perform_simulation(station, list, true);
    free_graph(dg);
}


void clp(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-cfg")) {
            build_cfg = 1;
        } else if (!strcmp(argv[i], "-dg")) {
            build_dep_graph = 1;
        } else if (!strcmp(argv[i], "--help")) {
            print_help = 1;
        } else if (!strcmp(argv[i], "--arch")) {
            if (argc - 1 == i || *argv[i + 1] == '-') {
                printf("Missing argument after --arch!\nDefault architecture is selected\n\n");
            } else
                arch_name = argv[++i];
        } else if (!strcmp(argv[i], "--loop")) {
            if (argc - 1 == i || *argv[i + 1] == '-') {
                printf("Missing argument after --loop!\nDefault number of loop iterations is selected\n\n");
            } else
                num_iterations = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--detail")) {
            if (argc - 1 == i || *argv[i + 1] == '-') {
                printf("Missing argument after --detail!\nPlease select a line you want to have analyzed\n\n");
            } else
                detail_line = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--setup")) {
            if (argc - 2 == i || *argv[i + 1] == '-' || *argv[i + 2] == '-') {
                printf("--setup need two arguments!\n--setup [Default architecture] [default number of iterations]\n\n");
            } else {
                setup_arch = argv[++i];
                setup_loop = atoi(argv[++i]);
            }
        } else if (!strcmp(argv[i], "--print-default")) {
            print_setup = true;
        } else if (!strcmp(argv[i], "-p")) {
            perfomance = true;
        } else if (!strcmp(argv[i], "-b")) {
            branch = 1;
        } else if (!strcmp(argv[i], "-pf")) {
            perfect_frontend = true;
            perfomance = true;
        } else if (!strcmp(argv[i], "-ip")) {
            infinite_ports = true;
            perfomance = true;
        } else if (!strcmp(argv[i], "-nd")) {
            no_deps = true;
            perfomance = true;
        } else if (*argv[i] == '-') {
            invalid_flag = argv[i];
        } else {
            file_name = argv[i];
        }
    }
}
