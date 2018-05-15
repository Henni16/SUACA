#include "disas.h"
#include "dependency_analysis.h"
#include "xmlParser.h"
#include <time.h>
#include "reservation_station.h"
#include "hashmap.h"
#include "inst_list.h"

int build_cfg;
int build_dep_graph;
int print_help;
int branch;
int num_iterations = 1;
int detail_line = -1;
char *invalid_flag;
char *file_name;
char *arch_name = "SNB";
station_t *station;
station_t *frontend_test;
int frontend_cycles;
station_t *port_test;
int port_cycles;
station_t *dep_test;
int dep_cycles;
inst_info_t **table_info;

void clp(int argc, char *argv[]);

void graphs_and_map(single_list_t *list, int index);

void help();

int main(int argc, char *argv[]) {
    clock_t start = clock();
    clp(argc, argv);
    if (print_help) {
        help();
        return 0;
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
        graphs_and_map(instructions->lists[i], i);
        if (i + 1 < instructions->numLists)
            printf("\n\n================================================\n\n\n");
    }
    free_list(instructions);
    printf("time: %f", (clock() - start) / (double) CLOCKS_PER_SEC);
    return 0;
}

void help() {
    printf("options:\n");
    printf(" -cfg:  build controlflowgraph\n");
    printf(" -dg:   build dependencygraph\n");
    printf(" --arch [x]: [x] is architecture the analysis is based on\n");
    printf(" --detail [x]: detailed delay info for line [x]\n");
    printf(" --loop [x]: run the analysis in a loop of [x]\n");
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
    if (detail_line == -1 && print) {
        print_sim_inst_list(station->done_insts, list, station->num_ports, arch_name, num_iterations, num_cycles,
                            total_num_microops, frontend_cycles, port_cycles, dep_cycles);
    } else if (print)
        print_sim_inst_details(station->done_insts, list, detail_line, station->num_ports, num_iterations);
    freeStation(station);
    return num_cycles;
}

int parse_stuff(single_list_t *insts) {
    char station_file[strlen(arch_name) + strlen(STATION_LOC) + strlen(".arch") + 1];
    build_station_file_string(station_file, arch_name);
    station = parse_station_file(station_file, num_iterations, insts->single_loop_size);
    if (station == NULL) {
        printf("The station file for %s could not be found\n", arch_name);
        return 0;
    }
    frontend_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
    frontend_test->load_per_cycle = frontend_test->cap;
    port_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
    port_test->non_blocking_ports = true;
    dep_test = parse_station_file(station_file, num_iterations, insts->single_loop_size);
    dep_test->load_per_cycle = dep_test->cap;
    dep_test->non_blocking_ports = true;
    hashset_t *set = create_hashset(insts);
    printf("Parsing measurement file...\n");
    table_info = parse_instruction_file(TABLE, arch_name, station->num_ports, set);
    printf("Done parsing!\n");
    hashset_free(set);
    if (table_info == NULL) {
        return 0;
    }

}

void graphs_and_map(single_list_t *list, int index) {
    if (num_iterations > 1) {
        if (!add_loop_instructions(list)) {
            printf("Aborting analysis!\n");
        }
    } else {
        list->single_loop_size = list->size;
    }
    graph_t *g = build_controlflowgraph(list);
    if (build_cfg)
        build_graphviz(g, list, "controlflow", index);
    graph_t *dg = build_dependencygraph_cfg(list, g);
    if (build_dep_graph)
        build_graphviz(dg, list, "dependency", index);
    free_graph(g);
    parse_stuff(list);
    if (detail_line == -1) {
        create_initial_state(dg, list, num_iterations, frontend_test, table_info, false);
        frontend_cycles = perform_simulation(frontend_test, list, false);
        create_initial_state(dg, list, num_iterations, port_test, table_info, false);
        port_cycles = perform_simulation(port_test, list, false);
        create_initial_state(dg, list, num_iterations, dep_test, table_info, false);
        dep_cycles = perform_simulation(dep_test, list, false);

    }
    create_initial_state(dg, list, num_iterations, station, table_info, true);
    perform_simulation(station, list, true);
    free_info_array(table_info);
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
        } else if (!strcmp(argv[i], "-b")) {
            branch = 1;
        } else if (*argv[i] == '-') {
            invalid_flag = argv[i];
        } else {
            file_name = argv[i];
        }
    }
}
