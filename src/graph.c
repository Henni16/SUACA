#include "graph.h"

node_t* newNode() {
  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->successors = NULL;
  node->num_successors = 0;
  return node;
}


graph_t* newGraph(int size) {
  node_t** g = (node_t**) malloc(size * sizeof(node_t*));
  for (size_t i = 0; i < size; i++) {
    g[i] = newNode();
  }
  graph_t* graph = (graph_t*) malloc(sizeof(graph_t));
  graph->nodes = g;
  graph->size = size;
  return graph;
}



void add_graph_dependency(int source_line, int destination_line, graph_t* graph){
  //check if last line is reached
  if (destination_line >= graph->size || destination_line < 0) return;

  node_t* source = graph->nodes[source_line];
  source->num_successors++;
  if (source->num_successors > 1) {
    source->successors = (int*) realloc(source->successors,
                                        source->num_successors * sizeof(int));
  }
  else {
    source->successors = (int*) malloc(sizeof(int));
  }
  source->successors[source->num_successors-1] = destination_line;
}


void free_graph(graph_t* graph) {
  for (size_t i = 0; i < graph->size; i++) {
    if (graph->nodes[i]->num_successors > 0)
      free(graph->nodes[i]->successors);
    free(graph->nodes[i]);
  }
  free(graph->nodes);
  free(graph);
}

void build_graphviz(graph_t* graph, single_list_t* list, char* name){
  char buf[0x100];
  snprintf(buf, sizeof(buf), "%s.graphviz", name);
  FILE *f = fopen(buf, "w");
  fprintf(f, "digraph %s{\n", name);
  for (size_t i = 0; i < list->size; i++) {
    char buf[TMP_BUF_LEN];
    disassemble(buf, TMP_BUF_LEN, &list->array[i], list->printinfo[i]);
    fprintf(f, "%i [label=\"%i: %s\", style=filled]\n", i, i+1, buf);
  }
  for (size_t i = 0; i < list->size; i++) {
    node_t* cur = graph->nodes[i];
    for (size_t j = 0; j < cur->num_successors; j++) {
      fprintf(f, "%i -> %i\n", i, cur->successors[j]);
    }
  }
  fprintf(f, "}");
  fclose(f);
}
