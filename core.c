#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "debug.h"

static struct {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t s;
	union {
		struct {
			uint8_t pcl;
			uint8_t pch;
		};
		uint16_t pc;
	};
} reg = { .a = 0xAA, .pcl = 0xFF };

static struct {
	bool n;
	bool v;
	bool b;
	bool d;
	bool i;
	bool z;
	bool c;
} flag = { .z = true };

static struct {
    union {
        struct {
            uint8_t address_lo;
            uint8_t address_hi;
        };
        uint16_t address;
    };
	uint8_t data;
    bool write;
	enum { READ, WRITE } rw;
} bus = { .address_lo = 0xFF };

static uint8_t transient_data;
static uint8_t ram[0x800]; // 2k
static uint8_t *rom;

inline static void access_memory(void) {
	if (!bus.write) {
		if (bus.address > 0x7FFF) {
			uint16_t const prg_mask = 0x3FFF; // 1x 16k PRG bank
			bus.data = rom[bus.address & prg_mask]; // mapper_read(bus.address);
			printf("  memory_read  %04X -> PRG %04X -> %02X\n", bus.address, bus.address & prg_mask, bus.data);
			return;
		}
		if (bus.address < 0x2000) {
			bus.data = ram[bus.address & 0x07FF];
			printf("  memory_read  %04X -> RAM %04X -> %02X\n", bus.address, bus.address & 0x07FF, bus.data);
			return;
		}
		if (bus.address < 0x4000) {
			bus.data = 0x00; // ppu_read(bus.address & 0x0007)
			printf("  memory_read  %04X -> \033[1;34mPPU\033[0m %X -> %02X\n", bus.address, bus.address & 0x0007, bus.data);
			return;
		}
		printf(" \033[1;41m memory_read  %04X ERR \033[0m\n", bus.address);
	} else {
		bus.write = false;
		if (bus.address < 0x2000) {
			ram[bus.address & 0x07FF] = bus.data;
			printf("  memory_write %04X -> RAM %04X -> %02X\n", bus.address, bus.address & 0x07FF, bus.data);
			return;
		}
		if (bus.address < 0x4000) {
			// ppu_write(bus.address & 0x0007, data);
			printf("  memory_write %04X -> \033[1;34mPPU\033[0m %X -> %02X\n", bus.address, bus.address & 0x0007, bus.data);
			return;
		}
		printf(" \033[1;41m memory_write %04X -> %02X ERR\033[0m\n", bus.address, bus.data);
	}
}

typedef void (*instruction_step)(void);
typedef instruction_step instruction[7];

static instruction const set[256];
static instruction_step const *current_step;
static unsigned long long step_counter = 0;

inline static void update_flags_nz(uint8_t reg) {
	flag.n = (reg & 0x80);
	flag.z = !reg;
}

inline static uint8_t group_status_flags(void) {
	uint8_t p = 0x20;
	if (flag.n) p |= 0x80;
	if (flag.v) p |= 0x40;
	if (flag.b) p |= 0x10;
	if (flag.d) p |= 0x08;
	if (flag.i) p |= 0x04;
	if (flag.z) p |= 0x02;
	if (flag.c) p |= 0x01;
	return p;
}

#include "steps.c"

static void terminate(void) {
	puts(__FUNCTION__);
	for (int i = 0; i < 256; i++)
		if ((current_step - 1) == set[i])
			fprintf(stdout, "ILLEGAL/UNIMPLEMENTED OPCODE \033[1;33m %02X \033[0m\n", i);
	free(rom);
	exit(EXIT_FAILURE);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
static instruction const set[256] = {
	// Illegal/unimplemented opcodes
	[0 ... 255] = { terminate },
	// SEI_implied
	[0x78] = { set_flag_i, decode_opcode },
	// CLD_implied
	[0xD8] = { clear_flag_d, decode_opcode },
	// TXS_implied
	[0x9A] = { transfer_reg_x_to_reg_s, decode_opcode },
	// LDA_immediate
	[0xA9] = { put_data_into_reg_a, decode_opcode },
	// LDX_immediate
	[0xA2] = { put_data_into_reg_x, decode_opcode },
	// AND_immediate
	[0x29] = { bitwise_and, decode_opcode },
	// BEQ_relative
	[0xF0] = { check_if_flag_z_is_set, branch_same_page, branch_any_page, decode_opcode },
	// STA_absolute
	[0x8D] = { load_absolute_address, store_reg_a_at_absolute_address, wrap_up_absolute_store, decode_opcode },
	// LDA_absolute
	[0xAD] = { load_absolute_address, load_data_from_absolute_address, put_data_into_reg_a, decode_opcode },
	// BRK_stack
	[0x00] = { push_pch, push_pcl, push_flags, load_interrupt_vector_lo, load_interrupt_vector_hi, put_interrupt_address_into_pc, decode_opcode }
};
#pragma GCC diagnostic pop

static instruction_step const *current_step = set[0x00];

void cpu_exec(void) {
	access_memory();
	printf(">> A %02X, X %02X, Y %02X, S %02X, P %02X, PC %04X, %c%cx%c%c%c%c%c #%03llu\t",
		reg.a, reg.x, reg.y, reg.s, group_status_flags(), reg.pc,
		flag.n ? 'n' : '.',
		flag.v ? 'v' : '.',
		flag.b ? 'b' : '.',
		flag.d ? 'd' : '.',
		flag.i ? 'i' : '.',
		flag.z ? 'z' : '.',
		flag.c ? 'c' : '.',
		step_counter
	);
	step_counter++;
	(*current_step++)();
}

static uint32_t calculate_crc32(uint8_t const *data, size_t length) {
	uint32_t crc = 0xFFFFFFFF;

	for (size_t i = 0; i < length; i++) {
		crc ^= data[i];

		for (int i = 0; i < 8; i++) {
			if (crc & 1)
				crc = ((crc >> 1) ^ 0xEDB88320);
			else
				crc >>= 1;
		}
	}
	return ~crc;
}

static uint8_t *read_file_chunk(char const *filename, long offset, long requested_length) {
	FILE *file_stream;

	// GCC nested function
	uint8_t *abort_reading(char const *msg) {
		fprintf(stdout, "<%s> %s! %s", filename, msg, errno ? strerror(errno) : "\n");
		if (file_stream)
			fclose(file_stream);
		return NULL;
	}

	file_stream = fopen(filename, "rb");
	if (file_stream == NULL)
		return abort_reading("Error opening ROM file");

	int file_seek = fseek(file_stream, 0, SEEK_END);
	if (file_seek != 0)
		return abort_reading("Error while seeking in file stream");

	long reported_length = ftell(file_stream);
	if (reported_length == -1L)
		return abort_reading("Error while obtaining file position indicator");

	if (reported_length < requested_length + offset)
		return abort_reading("Unexpected file size");

	file_seek = fseek(file_stream, offset, SEEK_SET);
	if (file_seek != 0)
		return abort_reading("Error while seeking in file stream");

	uint8_t *read_data = malloc(requested_length);
	if (read_data == NULL)
		return abort_reading("Error on memory allocation");

	size_t read_length = fread(read_data, 1, requested_length, file_stream);
	if (read_length - requested_length != 0) {
		free(read_data);
		return abort_reading("Unexpected amount of data read");
	}

	fclose(file_stream);
	return read_data;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stdout, "ROM filename is missing!\n");
		return EXIT_FAILURE;
	}

	uint8_t *header = read_file_chunk(argv[1], 0, 16);
	if (!header)
		return EXIT_FAILURE;
	free(header);

	rom = read_file_chunk(argv[1], 16, 24576);
	if (!rom)
		return EXIT_FAILURE;

	fprintf(stdout, "Emulation is on!\n");
	fprintf(stdout, "crc32 %08X\n\n", calculate_crc32(rom, 24576));

	while (step_counter < 48)
		cpu_exec();

	free(rom);
	return EXIT_SUCCESS;
}

