#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#define TMP_BUF_LEN (1024*4)

typedef struct node_s{
  int num_successors;
  int* successors;
  int* fathers;
  int num_fathers;
} node_t;


typedef struct {
  node_t** nodes;
  int size;
}graph_t;

node_t* newNode();

graph_t* newGraph(int size);

void free_graph(graph_t* graph);

int is_root(node_t* node);

node_t* get_node(graph_t* graph, int line);

void add_graph_dependency(int source_line, int destination_line, graph_t* graph);

int is_successor(int from, int to, graph_t* graph);

int is_successor_seen(int from, int to, graph_t* graph, int* seen);


/*
  print graphviz representation of graph
  requires single_list_t for the node names
  assumes that list and graph have same size
*/
void build_graphviz(graph_t* graph, single_list_t* list, char* name, int index);


#endif