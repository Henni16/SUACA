#ifndef DISAS_H
#define DISAS_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "headers.h"
#include "xed-examples-util.h"
#include "xed-disas-pecoff.h"
#include "xed-disas-macho.h"
#include "xed-disas-elf.h"

inst_list_t* build_inst_list(char* file_name);

#endif
