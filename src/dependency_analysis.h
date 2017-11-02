#ifndef DEPENDENCY_ANALYSIS_H
#define DEPENDENCY_ANALYSIS_H

#include <stdlib.h>
#include "headers.h"
#include "reg_map.h"
#include "disas.h"
#include "inst_list.h"

reg_map_t* compute_dependencies(single_list_t* instructions);


#endif
