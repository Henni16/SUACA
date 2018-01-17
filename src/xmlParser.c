#include "xmlParser.h"
#include <limits.h>

inst_info_t **parse_instruction_file(char *file_name) {
  FILE* file = fopen(file_name, "r");
  if (file == NULL) {
    printf("Xml file not found\n");
    return NULL;
  }
  char buff[MY_BUFF_SIZE];
  attribute_value_t a;
  inst_info_t* info_array = malloc(XED_IFORM_LAST * sizeof(xed_iform_enum_t));
  while (fscanf(file, "%s", buff) != EOF) {
      if (!strcmp(buff + 1, "instruction")) {
      fscanf(file, "%s", buff);
          //split_attribute(buff, &a);
          //printf("\nattribute: %s\n", a.attribute);
          //printf("value: %s\n\n", a.value);
          skip_cur_element(file);
    }
    else
      printf("%s\n", buff);
  }
  free(info_array);
  fclose(file);
    return NULL;
}


void split_attribute(char* attribute, attribute_value_t* a) {
  a->attribute = strtok(attribute, "=>");
  a->value = strtok(NULL, "=>");
}

void skip_cur_element(FILE* f) {
  char cur;
  int level = 1;
  cur = fgetc(f);
  while (cur != EOF) {
    if (cur == '<') {
      cur = fgetc(f);
      if (cur == '/') {
        level--;
        char rteBuff[255];
        fscanf(f, "%s", rteBuff);
        if (!level) break;
      }
      else {
        level++;
      }
    }
    else if (cur == '/') {
      cur = fgetc(f);
      if (cur == '>') {
        level--;
        if (!level) break;
      }
    }
    cur = fgetc(f);
  }
}


station_t *parse_station_file(char *file_name) {
    station_t *s = malloc(sizeof(station_t));
    s->size = INT_MAX;
    s->cap = INT_MAX;
    s->load_per_cycle = 4;
    s->num_ports = 6;
    s->ports = malloc(s->num_ports * sizeof(port_t *));
    for (size_t i = 0; i < s->num_ports; i++) {
        //create empty port as recursion anchor
        s->ports[i] = newPort(newSimInst(-1, NULL, 0, 0, 0, 0), NULL);
    }
    s->wait_queue = NULL;
    s->station_queue = NULL;
    s->done_insts = NULL;
    return s;
}

int get_max_latency(latency_reg_t *l) {
    int max = -1;
    while (l != NULL) {
        if (l->latency > max)
            max = l->latency;
        l = l->next;
    }
    return max;
}


int get_latency_for_register(latency_reg_t *l, xed_reg_enum_t reg) {
    while (l != NULL) {
        if (l->reg == reg)
            return l->latency;
        l = l->next;
    }
}