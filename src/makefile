CC=gcc
CFLAGS=-I../xed/kits/xed-install-base-2017-09-20-win-x86-64/include/ -lstdc++ -O3
DEPS = headers.h xed-examples-util.h xed-disas-pecoff.h inst_list.h xed.h reg_map.h dependency_analysis.h xed-disas-macho.h avltree.h xed-symbol-table.h graph.h sim_inst.h reservation_station.h xmlParser.h hashmap.h hashset.h xed-disas-elf.h
LIBS = ..\xed\kits\xed-install-base-2017-09-20-win-x86-64\lib\xed.lib
OBJ = main.o xed.o xed-examples-util.o xed-disas-pecoff.o inst_list.o reg_map.o dependency_analysis.o xed-disas-macho.o avltree.o xed-symbol-table.o graph.o sim_inst.o reservation_station.o xmlParser.o hashmap.o hashset.o xed-disas-elf.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

suaca: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY : clean
clean:
	rm *.o
