#if !defined(PECOFF_H)
#define PECOFF_H

#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"
#include "util.h"


void xed_disas_pecoff(xed_disas_info_t* fi, inst_list_t* instructions);

#endif
