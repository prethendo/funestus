#include <stdio.h>
#include <stdint.h>

static int pixel = 0;
static int scanline = 0;

static uint8_t vram[2048]; // 0x800
static uint8_t frame_buffer[256 * 240];

extern uint8_t *chr; // pattern tables
extern void cpu_interrupt(void);
extern void display_frame_buffer(uint8_t const *internal_frame_buffer);

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

static struct {
	union {
		struct {
			uint16_t fine_y_offset: 3;
			uint16_t bit_plane: 1;
			uint16_t tile_column: 4;
			uint16_t tile_row: 4;
			uint16_t half_of_chr: 1;
			uint16_t zero: 3;
		};
		uint16_t full;
	};
} ppu_address = { .zero = 0, .half_of_chr = 1 };

static void draw_pixel(void) {
	// this calculation finds the tile index inside the nametable, based on the scanline and the current pixel
	int tile_index = scanline / 8 * 32 + (pixel % 256 / 8);
	int tile_pos = vram[tile_index]; // position in the pattern table (chr)

	ppu_address.tile_row = (tile_pos & 0xF0) >> 4; // upper nibble of tile_pos
	ppu_address.tile_column = tile_pos & 0x0F; // lower nibble of tile_pos

	ppu_address.fine_y_offset = scanline % 8;

	ppu_address.bit_plane = 0;
	int a = chr[ppu_address.full] & (0x80 >> pixel % 8);

	ppu_address.bit_plane = 1;
	int b = chr[ppu_address.full] & (0x80 >> pixel % 8);

	frame_buffer[scanline * 256 + pixel] = (a ? 1 : 0) + (b ? 2 : 0);
}

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

static void run_visible_scanline(void) {
	//if (pixel == 255)
	//	generate_buffer(scanline);
	if (pixel < 256)
		draw_pixel();
	if (pixel++ == 340)
		scanline++, pixel = 0;
}

static void run_post_render_scanline(void) {
	if (pixel == 0)
		display_frame_buffer(frame_buffer);
	if (pixel++ == 340)
		scanline++, pixel = 0;
}

static void run_vblank(void) {
	if (scanline == 241 && pixel == 1) // send a signal to CPU
		cpu_interrupt();
	if (pixel++ == 340)
		scanline++, pixel = 0;
}

static void run_pre_render_scanline(void) {
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

