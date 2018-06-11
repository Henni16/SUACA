cmake_minimum_required(VERSION 3.9)
project(src)

set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(../xed/kits/xed-install-base-2017-09-20-win-x86-64/include/xed)
LINK_DIRECTORIES(../xed/kits/xed-install-base-2017-09-20-win-x86-64/lib)


add_executable(bachelor
        avltree.c
        avltree.h
        dependency_analysis.c
        dependency_analysis.h
        xed.c
        xed.h
        graph.c
        graph.h
        headers.h
        inst_list.c
        inst_list.h
        xed-disas-macho.c
        xed-disas-macho.h
        main.c
        xed-disas-elf.h
        xed-disas-elf.c
        xed-disas-pecoff.cpp
        xed-disas-pecoff.h
        reg_map.c
        reg_map.h
        reservation_station.c
        reservation_station.h
        sim_inst.c
        sim_inst.h
        xed-examples-util.c
        xed-examples-util.h
        xed-symbol-table.c
        xed-symbol-table.h
        xmlParser.c
        xmlParser.h hashmap.c hashmap.h hashset.c hashset.h)


TARGET_LINK_LIBRARIES(bachelor xed)