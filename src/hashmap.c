#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"


hashmap_t *hashmap_init() {
    hashmap_t *map = malloc(sizeof(hashmap_t));
    map->cap = 5000;
    map->array = calloc(map->cap, sizeof(collision_list_t *));
    for (int i = 0; i < xed_iform_enum_t_last(); ++i) {
        add_to_hashmap(map, xed_iform_enum_t2str(i), i);
    }
    return map;
}


void add_to_hashmap(hashmap_t *map, const char *name, xed_iform_enum_t iform) {
    char toadd[strlen(name)+1];
    cut_last_underscore(name, toadd);
    unsigned long hashval = hash(toadd) % map->cap;
    collision_list_t *list = map->array[hashval];
    if (list == NULL) {
        list = collision_list_new(toadd, iform);
        map->array[hashval] = list;
    } else {
        do {
            if (!strcmp(toadd, list->name)) {
                list->iforms[list->count++] = iform;
                break;
            } else if (list->next == NULL) {
                list->next = collision_list_new(toadd, iform);
                list = list->next;
            }
            list = list->next;
        } while (list != NULL);
    }
}


collision_list_t *hashmap_lookup(hashmap_t *map, const char *str) {
    unsigned long hashval = hash(str) % map->cap;
    collision_list_t *list = map->array[hashval];
    while (list != NULL) {
        if (!strcmp(str, list->name))
            return list;
        list = list->next;
    }
    return NULL;
}

void cut_last_underscore(const char *str, char *dest) {
    char *c = NULL;
    while (*(dest++) = *str++) {
        if (*str == '_')
            c = dest;
    }
    if (c)
        *c = 0;
}

unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


collision_list_t *collision_list_new(const char *name, xed_iform_enum_t iform) {
    collision_list_t *list = malloc(sizeof(collision_list_t));
    list->next = NULL;
    list->name = malloc(strlen(name)+1); //max len of iform string
    list->iforms = malloc(20 * sizeof(xed_iform_enum_t)); //maximum number of same enum is 18
    list->iforms[0] = iform;
    strcpy(list->name, name);
    list->count = 1;
    return list;
}

void hashmap_free(hashmap_t *map) {
    for (int i = 0; i < map->cap; ++i) {
        if (map->array[i] != NULL) {
            collision_list_t *c = map->array[i];
            collision_list_t *next;
            do {
                next = c->next;
                free(c->name);
                free(c->iforms);
                free(c);
                c = next;
            } while (c != NULL);
        }
    }
    free(map);
}
