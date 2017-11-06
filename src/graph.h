#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#define TMP_BUF_LEN (1024*4)

typedef struct node_s{
  int num_successors;
  int* successors;
} node_t;


typedef node_t** graph_t;

node_t* newNode();

graph_t newGraph(int size);

void free_graph(graph_t graph, int graph_size);

void add_graph_dependency(int source_line, int destination_line, graph_t graph);

/*
  print graphviz representation of graph
  requires single_list_t for the node names
  assumes that list and graph have same size
*/
void build_graphviz(graph_t graph, single_list_t* list, char* name);


#endif
