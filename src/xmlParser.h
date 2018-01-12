#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "headers.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MY_BUFF_SIZE 255

typedef struct inst_info_s {
  int latency;
  int num_ports;
  bool* usable_ports;
  int num_micro_ops;
} inst_info_t;

typedef struct attribute_value_s {
  char* attribute;
  char* value;
} attribute_value_t;

int main(void);

inst_info_t** parse_xml_file(char* file_name);

void skip_cur_element(FILE* f);

void split_attribute(char* attribute, attribute_value_t* a);


#endif
