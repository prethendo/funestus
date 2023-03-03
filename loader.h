#ifndef HEADER_LOADER
#define HEADER_LOADER

extern uint8_t *prg; // program code
extern uint8_t *chr; // pattern tables

bool load_rom(char const *file_name);

#endif

