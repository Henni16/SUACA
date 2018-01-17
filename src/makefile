CC=gcc
CFLAGS=-I..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include -lstdc++ -O3
DEPS = headers.h util.h pecoff.h inst_list.h disas.h reg_map.h dependency_analysis.h macho.h avltree.h xed-symbol-table.h graph.h sim_inst.h reservation_station.h xmlParser.h
LIBS = ..\xed\kits\xed-install-base-2017-09-20-win-x86-64\lib\xed.lib
OBJ = main.o disas.o util.o pecoff.o inst_list.o reg_map.o dependency_analysis.o macho.o avltree.o xed-symbol-table.o graph.o sim_inst.o reservation_station.o xmlParser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bachelor: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY : clean
clean:
	rm *.o
