cmake_minimum_required(VERSION 3.9)
project(src)

set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(../xed/kits/xed-install-base-2017-09-20-win-x86-64/include/xed)
LINK_DIRECTORIES(../xed/kits/xed-install-base-2017-09-20-win-x86-64/lib)


add_executable(src
        avltree.c
        avltree.h
        dependency_analysis.c
        dependency_analysis.h
        disas.c
        disas.h
        graph.c
        graph.h
        headers.h
        inst_list.c
        inst_list.h
        macho.c
        macho.h
        main.c
        pecoff.cpp
        pecoff.h
        reg_map.c
        reg_map.h
        reservation_station.c
        reservation_station.h
        sim_inst.c
        sim_inst.h
        util.c
        util.h
        xed-symbol-table.c
        xed-symbol-table.h
        xmlParser.c
        xmlParser.h)




TARGET_LINK_LIBRARIES(src xed)