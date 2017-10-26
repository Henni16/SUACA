#include "disas.h"
#include "dependency_analysis.h"

int main(int argc, char *argv[]) {
  inst_list_t* instructions = build_inst_list(argc, argv);
  if (instructions == NULL) {
    printf("Please give me a file to work on\n");
    return 0;
  }
  print_list(instructions);
  reg_map_t* map = compute_dependencies(instructions);
  print_map(map);
  free_map(map);
  free_list(instructions);
  return 0;
}
