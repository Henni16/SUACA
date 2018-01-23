#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "headers.h"
#include "reservation_station.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MY_BUFF_SIZE 255
#define XML_DEBUG 0

typedef struct latency_reg_s {
    int latency;
    //just for parsing
    int id;
    int numregs;
    int cap;
    xed_reg_enum_t *reg;
    struct latency_reg_s *next;
} latency_reg_t;

typedef struct inst_info_s {
    latency_reg_t *latencies;
    bool *usable_ports;
    int num_micro_ops;
} inst_info_t;

typedef struct attribute_value_s {
    char *attribute;
    char *value;
    char *rest;
} attribute_value_t;


inst_info_t **parse_instruction_file(char *file_name, char *architecture_name, int num_ports);

void parse_single_instruction(inst_info_t **info, FILE *file, char *architecture_name, int num_ports);

/*
 * reads item until it's finished
 * returns true if item is also closed
 */
bool search_end_of_item(FILE *file);

void parse_operands(FILE *file, inst_info_t *info);

void parse_registers(char *line, latency_reg_t *latreg);

/*
 * return true if measurement was found for architecture
 */
bool parse_architecture(FILE *file, inst_info_t *info, xed_iform_enum_t iform);

/*
 * sets the latency for the operand of the id
 */
void set_cycles(inst_info_t *info, int cycles, int id);

void parse_ports(char* buff, inst_info_t *info);

void skip_cur_element(FILE *f);

void split_attribute(char *attribute, attribute_value_t *a);

station_t *parse_station_file(char *file_name);

int get_max_latency(latency_reg_t *l);

/*
 * searches for the latency of a specific register
 * returns the maximum latency if register couldn't be found
 */
int get_latency_for_register(latency_reg_t *l, xed_reg_enum_t reg);

latency_reg_t *newLatReg();

inst_info_t *newInstInfo(int num_ports);

void add_reg_to_lat_reg(xed_reg_enum_t reg, latency_reg_t *latreg);

void extract_registers(FILE *file, latency_reg_t *latreg);

void free_info_array(inst_info_t **array);

void free_info(inst_info_t *info);

#endif
