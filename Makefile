funestus: core.c steps.c debug.h
	gcc $< -Wall -Wextra -Winline -Og -std=c11 -march=native -g -o $@ -DDEBUG

