#include <stdio.h>
#include <stdint.h>

int pixel = 0;
int scanline = 0;

extern void display_frame_buffer(uint8_t *internal_frame_buffer);
static uint8_t frame_buffer[256 * 240];
extern uint8_t *chr;

void ppu_write(uint16_t address, uint8_t data) {
	printf("PPU write: %04X -> %02X\n", address, data);
}

/* https://www.nesdev.org/wiki/PPU_pattern_tables
DCBA98 76543210
---------------
0HRRRR CCCCPTTT
|||||| |||||+++- T: Fine Y offset, the row number within a tile
|||||| ||||+---- P: Bit plane (0: "lower"; 1: "upper")
|||||| ++++----- C: Tile column
||++++---------- R: Tile row
|+-------------- H: Half of pattern table (0: "left"; 1: "right")
+--------------- 0: Pattern table is at $0000-$1FFF */

void generate_buffer(uint8_t scanline) {
	if (scanline > 127) return;
	int buffer_entry = scanline * 256;
	int pattern_table_tile = (scanline / 8) * 512;
	int pixel_line = scanline % 8;
	for (int tile = 0; tile < 32; tile++) {
		int pattern_table_entry = pattern_table_tile + pixel_line;
		for (int pixel = 0; pixel < 8; pixel++) {
			int a = chr[pattern_table_entry] & (0x80 >> pixel);
			int b = chr[pattern_table_entry + 8] & (0x80 >> pixel);
			frame_buffer[buffer_entry++] = (a ? 1 : 0) + (b ? 2 : 0);
		}
		pattern_table_tile += 16;
	}
}

void run_visible_scanline(void) {
	if (pixel == 255)
		generate_buffer(scanline);
	if (pixel++ == 340)
		pixel = 0, scanline++;
}

void run_post_render_scanline(void) {
	if (pixel == 0)
		display_frame_buffer(frame_buffer);
	if (pixel++ == 340)
		scanline++, pixel = 0;
}

void run_vblank(void) {
	if (pixel++ == 340)
		pixel = 0, scanline++;
}

void run_pre_render_scanline(void) {
	if (pixel++ == 340)
		scanline = pixel = 0;
}

void ppu_exec(void) {
	if (scanline < 240)
		run_visible_scanline();
	else if (scanline == 240)
		run_post_render_scanline();
	else if (scanline < 261)
		run_vblank();
	else // scanline == 261
		run_pre_render_scanline();
}

