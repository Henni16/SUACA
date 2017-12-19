#ifndef RESERVATION_STATION_H
#define RESERVATION_STATION_H

//TODO set table file here
#define TABLE ""

#include <stdbool.h>
#include "sim_inst.h"

typedef struct station_s {
  int size;
  int cap;
  int load_per_cycle;
  int num_ports;
  port_t** ports;
  sim_inst_t* wait_queue;
  sim_inst_t* station_queue;
  sim_inst_list_t* done_insts;
} station_t;

typedef struct port_s {
  sim_inst_t* inst;
  int num_cycles_in_port;
  bool availiable;
  struct port_s* next;
} port_t;


/*
  parses file to create station
*/
station_t* create_initial_state(graph_t* dependencies);


/*
  performs one clock cycle
*/
void perform_cycle(station_t* station);

/*
  performs the instruction load at the beginning of a cycle
*/
void load_instruction_into_station(station_t* station);

/*
  performs a greedy algorithm to put as many load_instructions
  into the ports as possible
*/
void put_executables_into_ports(station_t* station);

/*
  performs the execution of all instructions that are in the ports
*/
void execute_instructions_in_ports(station_t* station);

void delete_inst_from_queue(sim_inst_t* inst);

void freeStation(station_t* station);

port_t* newPort(sim_inst_t* inst, port_t* next);

void freePort(port_t* port);

void inform_children_im_done(sim_inst_t* inst);


#endif