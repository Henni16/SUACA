#include "disas.h"

int main(int argc, char *argv[]) {
  inst_list_t* instructions = build_inst_list(argc, argv);
  if (instructions == NULL) {
    printf("Please give me a file to work on\n");
    return 0;
  }
  free_list(instructions);
  return 0;
}
