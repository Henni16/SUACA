#include "reg_map.h"
#include <stdlib.h>
#include <assert.h>


char* access_enum_t2str(access_enum_t e);

reg_map_t* newMap(int size) {
  reg_map_t* ret = (reg_map_t*) malloc(sizeof(reg_map_t));
  ret->map = (access_t**) calloc(size, sizeof(access_t*));
  ret->size = size;
  return ret;
}


access_t* new_access_t(access_enum_t e, int line) {
  access_t* ret = (access_t*) malloc(sizeof(access_t));
  ret->line = line;
  ret->next = NULL;
  ret->read_write = e;
}


void add_to_map(reg_map_t* map, int reg, int line, access_enum_t read_write) {
  assert(reg < map->size);
  access_t* elem = new_access_t(read_write, line);
  access_t* cur = map->map[reg];
  if (cur == NULL) {
    map->map[reg] = elem;
  }
  else {
    while (cur->next != NULL) cur = cur->next;
    cur->next = elem;
  }
}


void free_map(reg_map_t* map) {
  access_t* cur;
  access_t* prev;
  for (size_t i = 0; i < map->size; i++) {
    cur = map->map[i];
    if (cur == NULL) continue;
    while (cur->next != NULL) {
      prev = cur;
      cur = cur->next;
      free(prev);
    }
    free(cur);
  }
  free(map);
}



void print_map(reg_map_t* map) {
  printf("\n\n Print Map!\n\n");
  access_t* cur;
  access_t* prev;
  for (size_t i = 0; i < map->size; i++) {
    cur = map->map[i];
    if (cur == NULL) continue;
    printf("\nRegister: %s\n", xed_reg_enum_t2str(i));
    while (cur != NULL) {
      printf("Used in line: %i with %s access\n", cur->line,
        access_enum_t2str(cur->read_write));
      cur = cur->next;
    }
  }
}

char* access_enum_t2str(access_enum_t e) {
  switch (e) {
    case WRITE: return "write";
    case READ: return "read";
  }
}
