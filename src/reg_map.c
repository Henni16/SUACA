#include "reg_map.h"
#include <stdlib.h>
#include <assert.h>


access_map_t* newMap(int size) {
  access_map_t* ret = (access_map_t*) malloc(sizeof(access_map_t));
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


void add_to_map(access_map_t* map, int reg, int line, access_enum_t read_write) {
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


void free_map(access_map_t* map) {
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
