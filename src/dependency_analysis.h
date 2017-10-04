#ifndef DEPENDENCY_ANALYSIS_H
#define DEPENDENCY_ANALYSIS_H

#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"
#include <stdlib.h>
#include "reg_map.h"
#include "disas.h"
#include "inst_list.h"

reg_map_t* compute_dependencies(inst_list_t* instructions);


#endif
