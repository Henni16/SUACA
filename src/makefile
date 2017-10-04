CC=gcc
CFLAGS=-I..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include -lstdc++
DEPS = util.h pecoff.h inst_list.h disas.h reg_map.h dependency_analysis.h
LIBS = ..\xed\kits\xed-install-base-2017-09-20-win-x86-64\lib\xed.lib
OBJ = main.o disas.o util.o pecoff.o inst_list.o reg_map.o dependency_analysis.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

disas: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
