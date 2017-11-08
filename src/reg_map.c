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
  return ret;
}


void add_to_map(reg_map_t* map, xed_reg_enum_t reg, int line, access_enum_t read_write) {
  assert(reg < map->size);
  access_t* elem = new_access_t(read_write, line);
  access_t* cur = map->map[compute_register(reg)];
  if (cur == NULL) {
    map->map[compute_register(reg)] = elem;
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

void swap(access_t* cur, access_t* prev) {
  if (prev != NULL) {
    prev->next = cur->next;
  }
  access_t* after = cur->next->next;
  cur->next->next = cur;
  cur->next = after;
}

void order_map(reg_map_t* map) {
  access_t* cur;
  access_t* prev;
  for (size_t i = 0; i < map->size; i++) {
    cur = map->map[i];
    if (cur == NULL) continue;
    prev = NULL;
    while (cur != NULL) {
      if (cur->next != NULL) {
        if (cur->line == cur->next->line) {
          if (cur->read_write == WRITE && cur->next->read_write == READ)
            swap(cur, prev);
          }
      }
      prev = cur;
      cur = cur->next;
    }
  }
}



void print_map(reg_map_t* map) {
  access_t* cur;
  for (size_t i = 0; i < map->size; i++) {
    cur = map->map[i];
    if (cur == NULL) continue;
    printf("\nRegister: %s\n", xed_reg_enum_t2str(i));
    while (cur != NULL) {
      printf("Used in line: %i with %s access\n", cur->line+1,
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


xed_reg_enum_t compute_register(xed_reg_enum_t reg){
  switch (reg) {
    case XED_REG_EAX:
    case XED_REG_AX:
    case XED_REG_AH:
    case XED_REG_AL:
      return XED_REG_RAX;
    case XED_REG_ECX:
    case XED_REG_CX:
    case XED_REG_CH:
    case XED_REG_CL:
      return XED_REG_RCX;
    case XED_REG_EDX:
    case XED_REG_DX:
    case XED_REG_DH:
    case XED_REG_DL:
      return XED_REG_RDX;
    case XED_REG_EBX:
    case XED_REG_BX:
    case XED_REG_BH:
    case XED_REG_BL:
      return XED_REG_RBX;
    case XED_REG_ESP:
    case XED_REG_SP:
      return XED_REG_RSP;
    case XED_REG_EBP:
    case XED_REG_BP:
      return XED_REG_RBP;
    case XED_REG_ESI:
    case XED_REG_SI:
      return XED_REG_RSI;
    case XED_REG_EDI:
    case XED_REG_DI:
      return XED_REG_RDI;
    default:
      return reg;
  }
}
