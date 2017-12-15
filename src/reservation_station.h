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
  bool* ports;
  sim_inst_t* station_queue_end;
  sim_inst_t* station_queue;
  sim_inst_list_t* done_insts;
}station_t;


/*
  parses file to create station
*/
station_t* create_initial_state();


/*
  performs one clock cycle
  returns true if there is still work to do
          false if all instructions have been executed;
*/
bool perform_cycle(station_t* station);

/*
  performs the instruction load at the beginning of a cycle
*/
void load_instruction_into_station(station_t* station);


#endif
