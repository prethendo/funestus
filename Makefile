CC_ARGS_ = -c -Wall -Wextra -Og -std=c11 -march=native -g -DDEBUG
CC_ARGS = -c -Wall -Wextra -O3 -std=c11 -march=native -s

funestus: core.o loader.o cpu.o ppu.o
	gcc -o $@ $^ -lSDL2 -lSDL2main

core.o: core.c
	gcc $(CC_ARGS) -o $@ $<

loader.o: loader.c
	gcc $(CC_ARGS) -o $@ $<

cpu.o: cpu.c steps.c debug.h
	gcc $(CC_ARGS) -o $@ $<

ppu.o: ppu.c
	gcc $(CC_ARGS) -o $@ $<

