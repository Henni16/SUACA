#include "hashset.h"
#include "inst_list.h"


//we will use linear probing
void insert_into_hashset(hashset_t *set, int to_insert) {
    int i = to_insert % set->size;
    while (set->elems[i] != -1 && set->elems[i] != to_insert) {
        i = (i + 1) % set->size;
    }
    set->elems[i] = to_insert;
}


hashset_t *new_hashset(int size) {
    hashset_t *set = malloc(sizeof(hashset_t));
    set->size = size * 2;
    set->elems = malloc(set->size * sizeof(int));
    for (int i = 0; i < set->size; ++i) {
        set->elems[i] = -1;
    }
    return set;
}

hashset_t *create_hashset(single_list_t *to_hash) {
    hashset_t *set = malloc(sizeof(hashset_t));
    //to_hash->size is upper bound of needed space, we double that for hashing purposes
    set->size = to_hash->size * 2;
    set->elems = malloc(set->size * sizeof(int));
    for (int i = 0; i < set->size; ++i) {
        set->elems[i] = -1;
    }
    for (int i = 0; i < to_hash->size; ++i) {
        insert_into_hashset(set, xed_decoded_inst_get_iform_enum(&to_hash->array[i]));
    }
    return set;
}



bool hashset_contains(hashset_t *set, int lookup) {
    int i = lookup % set->size;
    while (set->elems[i] != -1 && set->elems[i] != lookup) {
        i = (i + 1) % set->size;
    }
    return set->elems[i] == lookup;
}


void hashset_free(hashset_t *set) {
    free(set->elems);
    free(set);
}