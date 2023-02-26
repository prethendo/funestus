#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "debug.h"

extern void ppu_write(int ppu_register, uint8_t data);
extern uint8_t ppu_read(int ppu_register);

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
} transient;

static enum {
	NONE = 0x0000,
	NMI = 0xFFFA,
	RESET = 0xFFFC,
	IRQ = 0xFFFE
} interrupt_vector = RESET;

static uint8_t ram[0x800]; // 2k
extern uint8_t *prg;

static uint8_t read_memory(uint16_t address) {
	if (address > 0x7FFF) {
		uint16_t const prg_mask = 0x3FFF; // 1x 16k PRG bank
		uint8_t data = prg[address & prg_mask]; // mapper_read(address);
		printf("  memory_read  %04X -> PRG %04X -> %02X\n", address, address & prg_mask, data);
		return data;
	}
	if (address < 0x2000) {
		uint8_t data = ram[address & 0x07FF];
		printf("  memory_read  %04X -> RAM %03X -> %02X\n", address, address & 0x07FF, data);
		return data;
	}
	if (address < 0x4000) {
		uint8_t data = ppu_read(address & 0x0007);
		printf("  memory_read  %04X -> \033[1;34mPPU\033[0m %X -> %02X\n", address, address & 0x0007, data);
		return data;
	}
	if (address == 0x4016 || address == 0x4017) {
		printf("  read_memory  %04X -> \033[1;45mCTRL\033[0m %04X -> %02X\n", address, address & 0x3FFF, prg[address & 0x3FFF]);
		return 0x00;
	}
	printf(" \033[1;41m memory_read  %04X ERR \033[0m\n", address);
	exit(EXIT_FAILURE);
	return 0x00;
}

static void write_memory(uint16_t address, uint8_t data) {
	if (address < 0x2000) {
		ram[address & 0x07FF] = data;
		printf("  memory_write %04X -> RAM %03X -> %02X\n", address, address & 0x07FF, data);
		return;
	}
	if (address < 0x4000) {
		ppu_write(address & 0x0007, data);
		printf("  memory_write %04X -> \033[1;34mPPU\033[0m %X -> %02X\n", address, address & 0x0007, data);
		return;
	}
	if (address == 0x4014) {
		printf("  memory_write %04X -> \033[1;35mOAMDMA\033[0m ---> %02X\n", address, data);
		// cpu-ppu dma
		return;
	}
	if (address >= 0x4000 && address <= 0x4017) {
		printf("  write_memory %04X -> \033[1;45mCTRL\033[0m %04X -> %02X\n", address, address & 0x000F, data);
		return;
	}
	printf(" \033[1;41m memory_write %04X -> %02X ERR\033[0m\n", address, data);
	exit(EXIT_FAILURE);
	return;
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
	getchar();
	exit(EXIT_FAILURE);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
static instruction const set[256] = {
	// Illegal/unimplemented opcodes
	[0 ... 255] = { terminate },

	// LSR_accumulator
	[0x4A] = { fetch_and_waste, shift_right_reg_a },

	// SEI_implied
	[0x78] = { fetch_and_waste, set_flag_i },
	// CLC_implied
	[0x18] = { fetch_and_waste, clear_flag_c },
	// CLD_implied
	[0xD8] = { fetch_and_waste, clear_flag_d },
	// INY_implied
	[0xC8] = { fetch_and_waste, increment_reg_y },
	// DEX_implied
	[0xCA] = { fetch_and_waste, decrement_reg_x },
	// DEY_implied
	[0x88] = { fetch_and_waste, decrement_reg_y },
	// TAX_implied
	[0xAA] = { fetch_and_waste, transfer_reg_a_to_reg_x },
	// TAY_implied
	[0xA8] = { fetch_and_waste, transfer_reg_a_to_reg_y },
	// TXA_implied
	[0x8A] = { fetch_and_waste, transfer_reg_x_to_reg_a },
	// TXS_implied
	[0x9A] = { fetch_and_waste, transfer_reg_x_to_reg_s },
	// TYA_implied
	[0x98] = { fetch_and_waste, transfer_reg_y_to_reg_a },

	// LDA_immediate
	[0xA9] = { fetch_param_data, put_data_into_reg_a },
	// LDX_immediate
	[0xA2] = { fetch_param_data, put_data_into_reg_x },
	// LDY_immediate
	[0xA0] = { fetch_param_data, put_data_into_reg_y },
	// ADC_immediate
	[0x69] = { fetch_param_data, add_with_carry },
	// AND_immediate
	[0x29] = { fetch_param_data, bitwise_and },
	// EOR_immediate
	[0x49] = { fetch_param_data, bitwise_xor },
	// CMP_immediate
	[0xC9] = { fetch_param_data, compare_reg_a },

	// BEQ_relative
	[0xF0] = { fetch_param_data, skip_on_flag_z_clear, branch_same_page, branch_any_page },
	// BNE_relative
	[0xD0] = { fetch_param_data, skip_on_flag_z_set, branch_same_page, branch_any_page },
	// BPL_relative
	[0x10] = { fetch_param_data, skip_on_flag_n_set, branch_same_page, branch_any_page },

	// STA_zeropage W
	[0x85] = { fetch_param_address_zp, store_reg_a, fetch_opcode },
	// STY_zeropage W
	[0x84] = { fetch_param_address_zp, store_reg_y, fetch_opcode },
	// LDA_zeropage R
	[0xA5] = { fetch_param_address_zp, load_data, put_data_into_reg_a },
	// LDX_zeropage R
	[0xA6] = { fetch_param_address_zp, load_data, put_data_into_reg_x },
	// AND_zeropage R
	[0x25] = { fetch_param_address_zp, load_data, bitwise_and },
	// EOR_zeropage R
	[0x45] = { fetch_param_address_zp, load_data, bitwise_xor },
	// ADC_zeropage R
	[0x65] = { fetch_param_address_zp, load_data, add_with_carry },
	// DEC_zeropage M
	[0xC6] = { fetch_param_address_zp, load_data, decrement_data, store_data, fetch_opcode },
	// ROR_zeropage M
	[0x66] = { fetch_param_address_zp, load_data, rotate_right_data, store_data, fetch_opcode },

	// STA_absolute W
	[0x8D] = { fetch_param_address_lo, fetch_param_address_hi, store_reg_a, fetch_opcode },
	// LDA_absolute R
	[0xAD] = { fetch_param_address_lo, fetch_param_address_hi, load_data, put_data_into_reg_a },
	// JSR_absolute
	[0x20] = { fetch_param_address_lo, fetch_and_waste, push_pch, push_pcl, fetch_param_address_hi, branch_any_page },
	// JMP_absolute
	[0x4C] = { fetch_param_address_lo, fetch_param_address_hi, branch_any_page },

	// STA_indirectY
	[0x91] = { fetch_param_address_zp, load_address_lo, load_address_hi, add_reg_y_to_address, store_reg_a, fetch_opcode },
	// PHA_stack
	[0x48] = { fetch_and_waste, push_reg_a, fetch_opcode },
	// PLA_stack
	[0x68] = { fetch_and_waste, fetch_and_waste, pull_data, put_data_into_reg_a },
	// RTS_stack
	[0x60] = { fetch_param_data, fetch_and_waste, pull_pcl, pull_pch, fetch_param_data, fetch_opcode },
	// BRK_stack
	[0x00] = { fetch_and_waste, push_pch, push_pcl, push_status, load_interrupt_vector_lo, load_interrupt_vector_hi, fetch_opcode }
};
#pragma GCC diagnostic pop

static instruction_step const *current_step = set[0x00];

void cpu_exec(void) {
	printf(">> A %02X, X %02X, Y %02X, S %02X, P %02X, PC %04X, %c%c.%c%c%c%c%c #%06llu ",
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

void cpu_interrupt(void) {
	interrupt_vector = NMI;
}

