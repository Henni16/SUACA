CC=gcc
CFLAGS=-I..\xed\kits\xed-install-base-2017-09-20-win-x86-64\include -lstdc++
DEPS = util.h pecoff.h
LIBS = ..\xed\kits\xed-install-base-2017-09-20-win-x86-64\lib\xed.lib
OBJ = disas.o util.o pecoff.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

disas: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
