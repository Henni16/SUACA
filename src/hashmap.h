#ifndef HASHMAP_H
#define HASHMAP_H

#include "headers.h"

typedef struct collision_list_s {
    char *name;
    xed_iform_enum_t *iforms;
    int count;
    struct collision_list_s *next;
} collision_list_t;

typedef struct hashmap_s {
    collision_list_t **array;
    int cap;
} hashmap_t;


hashmap_t *hashmap_init();

collision_list_t *hashmap_lookup(hashmap_t *map, const char *str);

void cut_last_underscore(const char *str, char *dest);

unsigned long hash(const char *str);

collision_list_t *collision_list_new(const char *name, xed_iform_enum_t iform);

void add_to_hashmap(hashmap_t *map, const char *name, xed_iform_enum_t iform);

void hashmap_free(hashmap_t *map);

#endif //HASHMAP_H
