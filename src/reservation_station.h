#ifndef RESERVATION_STATION_H
#define RESERVATION_STATION_H

//TODO set table file here
#define TABLE "../../tables/result_reduced.xml"
//#define TABLE "../../tables/intel.xml"
#define STATION_LOC "../../tables/"
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
    // initial number of instruction of a single iteration
    int num_insts;
    int num_iterations;
    //we store a pointer to the instruction that currently blocks the port
    //for computation of delayed cycles
    sim_inst_t **ports;
    sim_inst_t *wait_queue;
    sim_inst_t *station_queue;
    sim_inst_list_t *done_insts;
    execute_list_t *to_exec;
    bool non_blocking_ports;
    int *port_usage;
} station_t;

typedef struct inst_info_s inst_info_t;

/*
  creates the sim_insts and the dependencies for the given station and instruction array
*/
void create_initial_state(graph_t *dependencies, single_list_t *insts, int num_iterations, station_t* station, inst_info_t **table_info, bool print_unsupported);


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

void execute_list_add(execute_list_t **list, sim_inst_t *to_add);

void execute_list_clear(execute_list_t **list);

void build_station_file_string(char *dest, const char *arch_name);

/*
 * computes the total number of microops in one iteration
 */
int compute_total_num_microops(station_t *station);

void insert_sorted(int *arr, int *len, int val, station_t *station);



#endif
