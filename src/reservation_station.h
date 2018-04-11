#ifndef RESERVATION_STATION_H
#define RESERVATION_STATION_H

//TODO set table file here
#define TABLE "../../tables/intel.xml"
#define STATION_FILE ""
#define ARCHITECTURE_NAME "SNB"

#include <stdbool.h>
#include "sim_inst.h"


typedef struct execute_list_s {
    sim_inst_t *elem;
    struct execute_list_s *next;
} execute_list_t;

typedef struct station_s {
    int size;
    int cap;
    int load_per_cycle;
    int num_ports;
    //we store a pointer to the instruction that currently blocks the port
    //for computation of delayed cycles
    sim_inst_t **ports;
    sim_inst_t *wait_queue;
    sim_inst_t *station_queue;
    sim_inst_list_t *done_insts;
    execute_list_t *to_exec;
} station_t;


/*
  parses file to create station
*/
station_t *create_initial_state(graph_t *dependencies, single_list_t *insts);


/*
  performs one clock cycle
*/
void perform_cycle(station_t *station);

/*
  performs the instruction load at the beginning of a cycle
*/
void load_instruction_into_station(station_t *station);

/*
  performs a greedy algorithm to put as many load_instructions
  into the ports as possible and "executes" them
*/
void put_executables_into_ports(station_t *station);

/*
 * increases the execution counter of all instructions in the ports
 * informs the children instructions
 */
void execute_instructions_in_ports(station_t *station);


void delete_inst_from_queue(sim_inst_t *inst, station_t *station);

void freeStation(station_t *station);

void inform_children_im_done(sim_inst_t *inst, int cycles_done);

void printStation(station_t *s);

void execute_list_add(execute_list_t **list, sim_inst_t *to_add);

void execute_list_clear(execute_list_t **list);



#endif
