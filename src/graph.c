#include "graph.h"
#include "inst_list.h"

node_t *newNode() {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->successors = NULL;
    node->fathers = NULL;
    node->num_fathers = 0;
    node->num_successors = 0;
    return node;
}


int is_root(node_t *node) {
    return !node->num_fathers;
}

node_t *get_node(graph_t *graph, int line) {
    return graph->nodes[line];
}

graph_t *newGraph(int size) {
    node_t **g = (node_t **) malloc(size * sizeof(node_t *));
    for (size_t i = 0; i < size; i++) {
        g[i] = newNode();
    }
    graph_t *graph = (graph_t *) malloc(sizeof(graph_t));
    graph->nodes = g;
    graph->size = size;
    return graph;
}


void add_graph_dependency(int source_line, int destination_line, graph_t *graph, xed_reg_enum_t dep_reg) {
    //check if last line is reached
    if (destination_line >= graph->size || destination_line < 0) return;

    node_t *source = graph->nodes[source_line];
    node_t *dest = graph->nodes[destination_line];
    source->num_successors++;
    dest->num_fathers++;
    if (source->num_successors > 1) {
        source->successors = (int_reg_tuple_t *) realloc(source->successors,
                                                         source->num_successors * sizeof(int_reg_tuple_t));
    } else {
        source->successors = (int_reg_tuple_t *) malloc(sizeof(int_reg_tuple_t));
    }
    if (dest->num_fathers > 1) {
        dest->fathers = (int_reg_tuple_t *) realloc(dest->fathers,
                                                    dest->num_fathers * sizeof(int_reg_tuple_t));
    } else {
        dest->fathers = (int_reg_tuple_t *) malloc(sizeof(int_reg_tuple_t));
    }
    source->successors[source->num_successors - 1] = (int_reg_tuple_t) {destination_line, dep_reg};
    dest->fathers[dest->num_fathers - 1] = (int_reg_tuple_t) {source_line, dep_reg};
}


void free_graph(graph_t *graph) {
    for (size_t i = 0; i < graph->size; i++) {
        if (graph->nodes[i]->num_successors > 0)
            free(graph->nodes[i]->successors);
        if (graph->nodes[i]->num_fathers > 0)
            free(graph->nodes[i]->fathers);
        free(graph->nodes[i]);
    }
    free(graph->nodes);
    free(graph);
}

void build_graphviz(graph_t *graph, single_list_t *list, char *name, int index) {
    char buf[0x100];
    snprintf(buf, sizeof(buf), "%s_%i.dot", name, index);
    FILE *f = fopen(buf, "w");
    fprintf(f, "digraph %s{\n", name);
    for (size_t i = 0; i < (list->single_loop_size > -1 ? list->single_loop_size : list->size); i++) {
        char buf[TMP_BUF_LEN];
        disassemble(buf, TMP_BUF_LEN, &list->array[i], list->printinfo[i]);
        fprintf(f, "%i [label=\"%i: %s\", style=filled]\n", i, i, buf);
    }
    for (size_t i = 0; i < (list->single_loop_size > 1 ? list->single_loop_size : list->size); i++) {
        node_t *cur = graph->nodes[i];
        for (size_t j = 0; j < cur->num_successors; j++) {
            if (cur->successors[j].reg != XED_REG_INVALID) {
                if (cur->successors[j].line >= list->single_loop_size) {
                    fprintf(f, "%i -> %i [ label = \"%s\" color = \"red\"]\n", i,
                            cur->successors[j].line - list->single_loop_size,
                            xed_reg_enum_t2str(cur->successors[j].reg));
                } else {
                    fprintf(f, "%i -> %i [ label = \"%s\" ]\n", i, cur->successors[j].line,
                            xed_reg_enum_t2str(cur->successors[j].reg));
                }
            } else {
                if (cur->successors[j].line >= list->single_loop_size) {
                    fprintf(f, "%i -> %i [ color = \"red\"]\n", i, cur->successors[j].line - list->single_loop_size);
                } else {
                    fprintf(f, "%i -> %i\n", i, cur->successors[j].line);
                }
            }

        }
    }
    fprintf(f, "}");
    fclose(f);
}


int is_successor(int from, int to, graph_t *graph) {
    int seen[graph->size];
    for (size_t i = 0; i < graph->size; i++) seen[i] = 0;
    return is_successor_seen(from, to, graph, seen);
}

int is_successor_seen(int from, int to, graph_t *graph, int *seen) {
    int cur_succ;
    for (size_t i = 0; i < graph->nodes[from]->num_successors; i++) {
        cur_succ = graph->nodes[from]->successors[i].line;
        if (cur_succ == to) return 1;
        if (!seen[cur_succ]) {
            seen[cur_succ] = 1;
            if (is_successor_seen(cur_succ, to, graph, seen)) return 1;
        }
    }
    return 0;
}


int get_num_children(int line, graph_t *graph) {
    return graph->nodes[line]->num_successors;
}
