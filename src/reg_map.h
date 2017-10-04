#if !defined(REG_MAP_H)
#define REG_MAP_H

#include <stdio.h>
#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"


typedef enum {
  WRITE,
  READ
} access_enum_t;

typedef struct access_s{
  access_enum_t read_write;
  int line;
  struct access_s* next;
} access_t;

typedef struct {
  access_t** map;
  int size;
} reg_map_t;

reg_map_t* newMap(int size);

void add_to_map(reg_map_t* map, int reg, int line, access_enum_t read_write);

void free_map(reg_map_t* map);

void print_map(reg_map_t* map);


#endif
