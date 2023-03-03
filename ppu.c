#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "loader.h"
#include "core.h"
#include "cpu.h"

static int pixel = 0;
static int scanline = 0;

static uint8_t vram[2048]; // 0x800
static uint8_t frame_buffer[256 * 240];

static enum { FIRST, SECOND } write_order;
static uint16_t ppu_address;

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
			uint16_t column: 4;
			uint16_t row: 4;
			uint16_t half_of_chr: 1;
			uint16_t zero: 3;
		};
		uint16_t full;
	};
} tile;

static struct {
	bool nmi_enabled; // Generate an NMI at the start of the vblank interval
	enum { HORIZONTAL = 1, VERTICAL = 32 } address_increment;
} ctrl;

static struct {
	bool vblank; // Vertical blank has started - Set at dot 1 of line 241 / Cleared after reading $2002 and at dot 1 of the pre-render scanline.
} status;

uint8_t ppu_read(int ppu_register) {
	// PPUSTATUS
	if (ppu_register == 2) {
		uint8_t vblank = status.vblank;
		printf("PPU read:  %04X -> %02X\n", ppu_register, vblank);
		status.vblank = false;
		write_order = FIRST;
		if (vblank)
			return 0x80;
	}
	return 0x00;
}

void ppu_write(int ppu_register, uint8_t data) {
	printf("PPU write: %04X -> %02X\n", ppu_register, data);
	// PPUCTRL
	if (ppu_register == 0) {
		ctrl.nmi_enabled = (data & 0x80);
		tile.half_of_chr = (data & 0x10) >> 4;
		//sprite_pattern_table_address = (data & 0x08) >> 3;
		ctrl.address_increment = (data & 0x04) ? VERTICAL : HORIZONTAL;
		//base_nametable_address = (data & 0x03);
		return;
	}
	// PPUADDR
	if (ppu_register == 6) {
		if (write_order == FIRST)
			ppu_address = data << 8;
		else // write_order == SECOND
			ppu_address |= data;
		write_order = !write_order;
		return;
	}
	// PPUDATA
	if (ppu_register == 7) {
		vram[ppu_address & 0x07FF] = data; // vram has size 2k == 0x800
		ppu_address += ctrl.address_increment;
		return;
	}
}

static void draw_pixel(void) {
	// this calculation finds the tile index inside the nametable, based on the scanline and the current pixel
	int tile_index = scanline / 8 * 32 + (pixel % 256 / 8);
	int tile_pos = vram[tile_index]; // position in the pattern table (chr)

	tile.row = (tile_pos & 0xF0) >> 4; // upper nibble of tile_pos
	tile.column = tile_pos & 0x0F; // lower nibble of tile_pos

	tile.fine_y_offset = scanline % 8;

	tile.bit_plane = 0;
	int a = chr[tile.full] & (0x80 >> pixel % 8);

	tile.bit_plane = 1;
	int b = chr[tile.full] & (0x80 >> pixel % 8);

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
	if (scanline == 241 && pixel == 1) { // send a signal to CPU
		if (ctrl.nmi_enabled)
			cpu_interrupt();
		status.vblank = true;
	}
	if (pixel++ == 340)
		scanline++, pixel = 0;
}

static void run_pre_render_scanline(void) {
	if (pixel == 1)
		status.vblank = false;
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

