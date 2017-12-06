#ifndef SIM_INST_H
#define SIM_INST_H

#include "graph.h"
#include <stdbool.h>

typedef struct sim_inst_s {
  int line;
  int num_micro_ops;
  int micro_ops_loaded;
  int micro_ops_processed;
  bool* usable_ports;
  int num_fathers;
  int fathers_done;
} sim_inst_t;

sim_inst_t* newSimInst(int line, bool* ports, int micro_ops, int num_fathers);

bool all_fathers_done(sim_inst_t* si);

void free_sim_inst(sim_inst_t* si);

#endif
