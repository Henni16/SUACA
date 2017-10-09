#ifndef DISAS_H
#define DISAS_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "headers.h"
#include "util.h"
#include "pecoff.h"
#include "macho.h"

inst_list_t* build_inst_list(int argc, char** argv);

#endif
