#if !defined(REG_MAP_H)
#define REG_MAP_H


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
} access_map_t;

access_map_t* newMap(int size);

void add_to_map(access_map_t* map, int reg, int line, access_enum_t read_write);

void free_map(access_map_t* map);


#endif
