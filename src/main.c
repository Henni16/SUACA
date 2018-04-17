#include "disas.h"
#include "dependency_analysis.h"
#include "xmlParser.h"
#include <time.h>
#include "reservation_station.h"
#include "hashmap.h"

int build_cfg;
int build_dep_graph;
int print_map_flag;
int print_help;
int num_iterations;
int detail_line = -1;
char *invalid_flag;
char *file_name;
char *arch_name = "SNB";

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
    //print_list(instructions);

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
}

void graphs_and_map(single_list_t *list, int index) {
    graph_t *g = build_controlflowgraph(list);
    if (build_cfg)
        build_graphviz(g, list, "controlflow", index);
    graph_t *dg = build_dependencygraph_cfg(list, g);
    if (build_dep_graph)
        build_graphviz(dg, list, "dependency", index);
    free_graph(g);
    station_t* station = create_initial_state(dg, list, arch_name);
    free_graph(dg);
    if (station != NULL) {
        // perform computations until both queues are empty and no instruction is be executed (to_exec)
        while (station->wait_queue || station->station_queue || station->to_exec) {
            perform_cycle(station);
        }
        if (detail_line == -1)
            print_sim_inst_list(station->done_insts, list, station->num_ports, arch_name);
        else
            print_sim_inst_details(station->done_insts, list, detail_line, station->num_ports);
        freeStation(station);
    } else {
        printf("Couldn't create station!\n");
    }
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
        } else if (*argv[i] == '-') {
            invalid_flag = argv[i];
        } else {
            file_name = argv[i];
        }
    }
}
