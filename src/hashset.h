
#ifndef SRC_HASHSET_H
#define SRC_HASHSET_H


#include "inst_list.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct hashset_s {
    int *elems;
    int size;
} hashset_t;

hashset_t *create_hashset(single_list_t *to_hash);

void hashset_free(hashset_t *set);

bool hashset_contains(hashset_t *set, int lookup);

#endif //SRC_HASHSET_H
