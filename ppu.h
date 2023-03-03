#ifndef HEADER_PPU
#define HEADER_PPU

void ppu_exec(void);
void ppu_write(int ppu_register, uint8_t data);
uint8_t ppu_read(int ppu_register);

#endif

