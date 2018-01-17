#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "headers.h"
#include "reservation_station.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MY_BUFF_SIZE 255

typedef struct latency_reg_s {
  int latency;
    xed_reg_enum_t reg;
    struct latency_reg_s *next;
} latency_reg_t;

typedef struct inst_info_s {
    latency_reg_t *latencies;
  bool* usable_ports;
  int num_micro_ops;
} inst_info_t;

typedef struct attribute_value_s {
  char* attribute;
  char* value;
} attribute_value_t;

int main(void);

inst_info_t **parse_instruction_file(char *file_name);

void skip_cur_element(FILE* f);

void split_attribute(char* attribute, attribute_value_t* a);

station_t *parse_station_file(char *file_name);

int get_max_latency(latency_reg_t *l);

int get_latency_for_register(latency_reg_t *l, xed_reg_enum_t reg);

#endif
