#include "reservation_station.h"




void load_instruction_into_station(station_t* station) {
  int loaded = 0;
  int loadable;
  if (station->cap - station->size < station->load_per_cycle)
    loadable = station->cap - station->size;
  else
    loadable = station->load_per_cycle;
  sim_inst_t* next = station->station_queue_end->next;
  while (loadable > 0) {
    //all instruct of next can be loaded
    if (get_loadable_micro_ops(next) <= loadable) {
      next->micro_ops_loaded = next->num_micro_ops;
      station->station_queue_end = next;
      loadable -= get_loadable_micro_ops(next);
      next = next->next;
    }
    //instruction can't be fully loaded
    else {
      next->micro_ops_loaded += loadable;
      loadable = 0;
    }
  }
}
