#ifndef DISAS_H
#define DISAS_H


#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "pecoff.h"
#include "macho.h"

inst_list_t* build_inst_list(int argc, char** argv);

#endif
