#include "disas.h"
#include "dependency_analysis.h"

int build_cfg;
int build_dep_graph;
int print_map_flag;
int print_help;
char* invalid_flag;
char* file_name;

void clp(int argc, char *argv[]);

void graphs_and_map(single_list_t* list, int index);

void help();

int main(int argc, char *argv[]) {
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
    printf("Please give me a file to work on\n");
    return 0;
  }

  inst_list_t* instructions = build_inst_list(file_name);
  print_list(instructions);

  for (size_t i = 0; i < instructions->numLists; i++) {
    graphs_and_map(instructions->lists[i], i);
    if (i+1 < instructions->numLists && print_map)
      printf("\n\n================================================\n\n\n");
  }
  free_list(instructions);
  return 0;
}

void help() {
  printf("options:\n");
  printf(" -cfg:  build controlflowgraph\n");
  printf(" -dg:   build dependencygraph\n");
  printf(" -map:  print dependencymap\n");
}

void graphs_and_map(single_list_t* list, int index) {
  reg_map_t* map = compute_dependencies(list);
  graph_t* g = build_controlflowgraph(list);
  if (build_cfg)
    build_graphviz(g, list, "controlflow", index);
  graph_t* dg = build_dependencygraph(map, g);
  if (build_dep_graph)
    build_graphviz(dg, list, "dependency", index);
  if (print_map_flag) {
    print_map(map);
  }
  free_map(map);
  free_graph(g);
}


void clp(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-cfg")) {
            build_cfg = 1;
        }else if (!strcmp(argv[i], "-dg")) {
            build_dep_graph = 1;
        }else if (!strcmp(argv[i], "-map")) {
            print_map_flag = 1;
        }else if (!strcmp(argv[i], "-help")) {
            print_help = 1;
        }else if (*argv[i] == '-') {
            invalid_flag = argv[i];
        }else {
            file_name = argv[i];
        }
    }
}
