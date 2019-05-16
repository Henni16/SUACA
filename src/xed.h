/*
This file has been edited.

BEGIN_LEGAL
Copyright (c) 2016-2017 Intel Corporation
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
      http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
END_LEGAL */

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
