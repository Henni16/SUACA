#ifndef SIM_INST_H
#define SIM_INST_H

#include "graph.h"
#include <stdbool.h>

struct reg_sim_inst_s;

//forward declaration
typedef struct port_ops_s port_ops_t;

typedef struct sim_inst_s {
    int line;
    int latency;
    int num_micro_ops;
    int micro_ops_loaded;
    port_ops_t *micro_ops;
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
    struct sim_inst_s *next;
    struct sim_inst_s *previous;
    //nodes that rely on this one to be finished
    int num_dep_children;
    struct reg_sim_inst_s *dep_children;
    //number of cycles that were used on each port
    int* used_ports;
    // number of cycles this instruction has been executed
    int executed_cycles;
    // number of microops that have been put into a port
    int executed_microops;
} sim_inst_t;

typedef struct reg_sim_inst_s {
    sim_inst_t *child;
    int cycles;
} reg_sim_inst_t;

typedef struct sim_inst_list_s {
    sim_inst_t **arr;
    int size;
} sim_inst_list_t;

sim_inst_t *newSimInst(int line, port_ops_t *micro_ops, int num_micro_ops, int num_fathers,
                       int latency, int num_children, int numports);

bool all_fathers_done(sim_inst_t *si);

void free_sim_inst(sim_inst_t *si);

int get_loadable_micro_ops(sim_inst_t *inst);

void load_num_micro_ops(sim_inst_t *inst, int num_ops);

sim_inst_list_t *newSimInstList(int length);

void add_to_sim_list(sim_inst_list_t *list, sim_inst_t *elem);

void free_sim_inst_list(sim_inst_list_t *list);

void print_sim_inst_list(sim_inst_list_t *list, single_list_t *inst_list, int num_ports);


#endif
