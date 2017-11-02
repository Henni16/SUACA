#include "disas.h"
#include "dependency_analysis.h"

int main(int argc, char *argv[]) {
  inst_list_t* instructions = build_inst_list(argc, argv);
  if (instructions == NULL) {
    printf("Please give me a file to work on\n");
    return 0;
  }
  print_list(instructions);
  for (size_t i = 0; i < instructions->numLists; i++) {
    reg_map_t* map = compute_dependencies(instructions->lists[i]);
    print_map(map);
    free_map(map);
    if (i+1 < instructions->numLists)
      printf("\n\n================================================\n\n\n");
  }
  free_list(instructions);
  return 0;
}
