/*BEGIN_LEGAL

Copyright (c) 2016 Intel Corporation

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
/// @file xed-disas-macho.H
#if !defined(XED_DISAS_MACHO_H)
# define XED_DISAS_MACHO_H

# if defined(__APPLE__)
#include "..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include\xed\xed-interface.h"
#include "util.h"

void xed_disas_macho(xed_disas_info_t* fi, inst_list_t* instructions);
# endif
#endif
