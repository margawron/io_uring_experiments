CC = clang++
CFLAGS = -Wall
LDLIBS = -luring
OBJ = main.cpp


install: main.cpp
	$(CC) -o program.out $^ $(CFLAGS) $(LDLIBS)

run:
	./program.out