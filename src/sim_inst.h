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
  int* used_ports;
  int fathers_todo;
  /*
    number of cycles the instruction had to wait
    for another instrunction to finish
  */
  int cycles_delayed;
  /*
    number of cycles the instruction caused another
    instruction to be delayed
  */
  int delayed_cycles;
  struct sim_inst_s* next;
} sim_inst_t;

typedef struct sim_inst_list_s {
  sim_inst_t** arr;
} sim_inst_list_t;

sim_inst_t* newSimInst(int line, bool* ports, int micro_ops, int num_fathers,
                       int num_ports);

bool all_fathers_done(sim_inst_t* si);

void free_sim_inst(sim_inst_t* si);

int get_loadable_micro_ops(sim_inst_t* inst);

sim_inst_list_t* newSimInstList(int length);

void add_to_sim_list(sim_inst_list_t* list, sim_inst_t* elem);

void free_sim_inst_list(sim_inst_list_t* list);


#endif
