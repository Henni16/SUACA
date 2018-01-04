#include "reservation_station.h"



void perform_cycle(station_t* station) {
  //TODO before or after selection of executions??
  load_instruction_into_station(station);
  put_executables_into_ports(station);
  execute_instructions_in_ports(station);
}


void execute_instructions_in_ports(station_t* station) {
  port_t* port;
  port_t* prev = NULL;
  for (size_t i = 0; i < station->num_ports; i++) {
    port = station->ports[i];
    while (port != NULL) {
      port->availiable = true;
      if (++port->num_cycles_in_port == port->inst->latency) {
        if (prev != NULL)
          prev->next = port->next;
        port->inst->micro_ops_processed++;
        if (instruction_done(port->inst)) {
          inform_children_im_done(port->inst);
          delete_inst_from_queue(port->inst);
          add_to_sim_list(station->done_insts, port->inst);
          freePort(port);
        }
      }
      else {
        prev = port;
      }
      port = port->next;
    }
  }
}

void put_executables_into_ports(station_t* station) {
  sim_inst_t* cur = station->sation_queue;
  while (cur != NULL && cur->micro_ops_loaded > 0) {
    //would like to process micro_op but it isn't loaded
    if (cur->micro_ops_loaded < cur->being_processed) {
      //all remaining micro_ops are being processed
      if (cur->being_processed > cur->num_micro_ops) continue;
      //we reached the end of the station and not everything is loaded
      else break;
    }
    //there currently is a micro_op in a port
    if (cur->being_processed > cur->micro_ops_processed) continue;
    if (!all_fathers_done(cur)) {
      cur->cycles_delayed++;
    }
    else {
      bool fits = false;
      for (size_t i = 0; i < station->num_ports && !fits; i++) {
        if (cur->usable_ports[i]) {
          if (station->ports[i]->availiable) {
            station->ports[i] = newPort(cur, station->ports[i]);
            fits = true;
          }
        }
      }
      if (!fits) {
        cur->cycles_delayed++;
        for (size_t i = 0; i < station->num_ports; i++) {
          station->ports[i]->inst->delayed_cycles++;
        }
      }
    }
    cur = cur->next;
  }
}

void load_instruction_into_station(station_t* station) {
  int loadable;
  if (station->cap - station->size < station->load_per_cycle)
    loadable = station->cap - station->size;
  else
    loadable = station->load_per_cycle;
  sim_inst_t* next = station->wait_queue;
  while (loadable > 0) {
    //all instruct of next can be loaded
    if (get_loadable_micro_ops(next) <= loadable) {
      loadable -= get_loadable_micro_ops(next);
      next->micro_ops_loaded = next->num_micro_ops;
      station->wait_queue = next->next;
      next = next->next;
    }
    //instruction can't be fully loaded
    else {
      next->micro_ops_loaded += loadable;
      loadable = 0;
    }
  }
}

void delete_inst_from_queue(sim_inst_t* inst) {
  inst->previous->next = inst->next;
  inst->next->previous = inst->previous;
}

port_t* newPort(sim_inst_t* inst, port_t* next) {
  port_t* port = (port_t*) malloc(sizeof(port_t));
  port->inst = inst;
  port->next = next;
  port->num_cycles_in_port = 0;
  port->availiable = false;
  return port;
}


void freePort(port_t* port) {
  free(port);
}


void freeStation(station_t* station) {
  free(station->ports);
  free(station);
}

void inform_children_im_done(sim_inst_t* inst) {
  for (size_t i = 0; i < inst->num_dep_children; i++) {
    inst->dep_children[i]->fathers_todo--;
  }
}


void printStation(station_t* s) {
  printf("wait queue:\n");
  sim_inst_t* cur = s->wait_queue;
  while (cur) {
    printf("line: %i loaded: %i processed: %i\n",
            cur->line, cur->micro_ops_loaded, cur->micro_ops_processed);
    cur = cur->next;
  }
  cur = s->station_queue;
  printf("station queue:\n");
  while (cur && cur->micro_ops_loaded>0) {
    printf("line: %i loaded: %i processed: %i\n",
            cur->line, cur->micro_ops_loaded, cur->micro_ops_processed);
    cur = cur->next;
  }
  printf("done list:\n");
  for (size_t i = 0; i < s->done_insts->size; i++) {
    if (s->done_insts->arr[i])
      printf("%i, ", i);
  }
  printf("\n");
  for (size_t i = 0; i < s->num_ports; i++) {
    printf("port %s: ", i);
    printPort(s->ports[i]);
    printf("\n");
  }
}


void printPort(port_t* p) {
  printf("line: %i cycles in port: %i", p->inst->line, p->num_cycles_in_port);
  if (p->next) {
    printf(" -> ");
    printPort(p->next);
  }
}
