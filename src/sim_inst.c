#include "sim_inst.h"

sim_inst_t* newSimInst(int line, bool* ports, int micro_ops, int fathers) {
  sim_inst_t* ret = (sim_inst_t*) malloc(sizeof(sim_inst_t));
  ret->num_micro_ops = micro_ops;
  ret->micro_ops_loaded = 0;
  ret->micro_ops_processed = 0;
  ret->ports = ports;
  ret->line = line;
  ret->fathers_done = 0;
  ret->num_fathers = fathers;
}


bool all_fathers_done(sim_inst_t* si) {
  return si->fathers_done == si->num_fathers;
}

void free_sim_inst(sim_inst_t* si) {
  free(si);
}
