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
		// cpu-ppu dma
		printf("  memory_write %04X -> \033[1;35mOAMDMA\033[0m ---> %02X\n", address, data);
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

inline static void ungroup_status_flags(uint8_t p) {
	flag.n = p & 0x80;
	flag.v = p & 0x40;
	// flag.b is not updated
	flag.d = p & 0x08;
	flag.i = p & 0x04;
	flag.z = p & 0x02;
	flag.c = p & 0x01;
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

	// ASL_accumulator
	[0x0A] = { fetch_and_waste, shift_left_reg_a },
	// LSR_accumulator
	[0x4A] = { fetch_and_waste, shift_right_reg_a },
	// ROL_accumulator
	[0x2A] = { fetch_and_waste, rotate_left_reg_a },
	// ROR_accumulator
	[0x6A] = { fetch_and_waste, rotate_right_reg_a },

	// SEC_implied
	[0x38] = { fetch_and_waste, set_flag_c },
	// SEI_implied
	[0x78] = { fetch_and_waste, set_flag_i },
	// CLC_implied
	[0x18] = { fetch_and_waste, clear_flag_c },
	// CLD_implied
	[0xD8] = { fetch_and_waste, clear_flag_d },
	// INX_implied
	[0xE8] = { fetch_and_waste, increment_reg_x },
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
	// NOP_implied
	[0xEA] = { fetch_and_waste, fetch_opcode },

	// LDA_immediate
	[0xA9] = { fetch_param_data, put_data_into_reg_a },
	// LDX_immediate
	[0xA2] = { fetch_param_data, put_data_into_reg_x },
	// LDY_immediate
	[0xA0] = { fetch_param_data, put_data_into_reg_y },
	// ADC_immediate
	[0x69] = { fetch_param_data, add_with_carry },
	// SBC_immediate
	[0xE9] = { fetch_param_data, subtract_with_carry },
	// AND_immediate
	[0x29] = { fetch_param_data, bitwise_and },
	// ORA_immediate
	[0x09] = { fetch_param_data, bitwise_or },
	// EOR_immediate
	[0x49] = { fetch_param_data, bitwise_xor },
	// CMP_immediate
	[0xC9] = { fetch_param_data, compare_reg_a },
	// CPX_immediate
	[0xE0] = { fetch_param_data, compare_reg_x },
	// CPY_immediate
	[0xC0] = { fetch_param_data, compare_reg_y },

	// BEQ_relative
	[0xF0] = { fetch_param_data, skip_on_flag_z_clear, branch_same_page, branch_any_page },
	// BMI_relative
	[0x30] = { fetch_param_data, skip_on_flag_n_clear, branch_same_page, branch_any_page },
	// BCS_relative
	[0xB0] = { fetch_param_data, skip_on_flag_c_clear, branch_same_page, branch_any_page },
	// BVS_relative
	[0x70] = { fetch_param_data, skip_on_flag_v_clear, branch_same_page, branch_any_page },
	// BNE_relative
	[0xD0] = { fetch_param_data, skip_on_flag_z_set, branch_same_page, branch_any_page },
	// BPL_relative
	[0x10] = { fetch_param_data, skip_on_flag_n_set, branch_same_page, branch_any_page },
	// BCC_relative
	[0x90] = { fetch_param_data, skip_on_flag_c_set, branch_same_page, branch_any_page },

	// STA_zeropage W
	[0x85] = { fetch_param_address_zp, store_reg_a, fetch_opcode },
	// STX_zeropage W
	[0x86] = { fetch_param_address_zp, store_reg_x, fetch_opcode },
	// STY_zeropage W
	[0x84] = { fetch_param_address_zp, store_reg_y, fetch_opcode },
	// LDA_zeropage R
	[0xA5] = { fetch_param_address_zp, load_data, put_data_into_reg_a },
	// LDX_zeropage R
	[0xA6] = { fetch_param_address_zp, load_data, put_data_into_reg_x },
	// LDY_zeropage R
	[0xA4] = { fetch_param_address_zp, load_data, put_data_into_reg_y },
	// CMP_zeropage R
	[0xC5] = { fetch_param_address_zp, load_data, compare_reg_a },
	// CPX_zeropage R
	[0xE4] = { fetch_param_address_zp, load_data, compare_reg_x },
	// BIT_zeropage R
	[0x24] = { fetch_param_address_zp, load_data, bit_test },
	// AND_zeropage R
	[0x25] = { fetch_param_address_zp, load_data, bitwise_and },
	// ORA_zeropage R
	[0x05] = { fetch_param_address_zp, load_data, bitwise_or },
	// EOR_zeropage R
	[0x45] = { fetch_param_address_zp, load_data, bitwise_xor },
	// ADC_zeropage R
	[0x65] = { fetch_param_address_zp, load_data, add_with_carry },
	// SBC_zeropage R
	[0xE5] = { fetch_param_address_zp, load_data, subtract_with_carry },
	// INC_zeropage M
	[0xE6] = { fetch_param_address_zp, load_data, increment_data, store_data, fetch_opcode },
	// DEC_zeropage M
	[0xC6] = { fetch_param_address_zp, load_data, decrement_data, store_data, fetch_opcode },
	// ASL_zeropage M
	[0x06] = { fetch_param_address_zp, load_data, shift_left_data, store_data, fetch_opcode },
	// LSR_zeropage M
	[0x46] = { fetch_param_address_zp, load_data, shift_right_data, store_data, fetch_opcode },
	// ROL_zeropage M
	[0x26] = { fetch_param_address_zp, load_data, rotate_left_data, store_data, fetch_opcode },
	// ROR_zeropage M
	[0x66] = { fetch_param_address_zp, load_data, rotate_right_data, store_data, fetch_opcode },

	// STA_zeropageX W
	[0x95] = { fetch_param_address_zp, add_reg_x_to_address_lo, store_reg_a, fetch_opcode },
	// STY_zeropageX W
	[0x94] = { fetch_param_address_zp, add_reg_x_to_address_lo, store_reg_y, fetch_opcode },
	// LDA_zeropageX R
	[0xB5] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, put_data_into_reg_a },
	// LDY_zeropageX R
	[0xB4] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, put_data_into_reg_y },
	// CMP_zeropageX R
	[0xD5] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, compare_reg_a },
	// AND_zeropageX R
	[0x35] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, bitwise_and },
	// ADC_zeropageX R
	[0x75] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, add_with_carry },
	// SBC_zeropageX R
	[0xF5] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, subtract_with_carry },
	// INC_zeropageX M
	[0xF6] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, increment_data, store_data, fetch_opcode },
	// DEC_zeropageX M
	[0xD6] = { fetch_param_address_zp, add_reg_x_to_address_lo, load_data, decrement_data, store_data, fetch_opcode },

	// STA_absolute W
	[0x8D] = { fetch_param_address_lo, fetch_param_address_hi, store_reg_a, fetch_opcode },
	// STX_absolute W
	[0x8E] = { fetch_param_address_lo, fetch_param_address_hi, store_reg_x, fetch_opcode },
	// STY_absolute W
	[0x8C] = { fetch_param_address_lo, fetch_param_address_hi, store_reg_y, fetch_opcode },
	// LDA_absolute R
	[0xAD] = { fetch_param_address_lo, fetch_param_address_hi, load_data, put_data_into_reg_a },
	// LDX_absolute R
	[0xAE] = { fetch_param_address_lo, fetch_param_address_hi, load_data, put_data_into_reg_x },
	// LDY_absolute R
	[0xAC] = { fetch_param_address_lo, fetch_param_address_hi, load_data, put_data_into_reg_y },
	// CMP_absolute R
	[0xCD] = { fetch_param_address_lo, fetch_param_address_hi, load_data, compare_reg_a },
	// ORA_absolute R
	[0x0D] = { fetch_param_address_lo, fetch_param_address_hi, load_data, bitwise_or },
	// EOR_absolute R
	[0x4D] = { fetch_param_address_lo, fetch_param_address_hi, load_data, bitwise_xor },
	// ADC_absolute R
	[0x6D] = { fetch_param_address_lo, fetch_param_address_hi, load_data, add_with_carry },
	// SBC_absolute R
	[0xED] = { fetch_param_address_lo, fetch_param_address_hi, load_data, subtract_with_carry },
	// INC_absolute M
	[0xEE] = { fetch_param_address_lo, fetch_param_address_hi, load_data, increment_data, store_data, fetch_opcode },
	// DEC_absolute M
	[0xCE] = { fetch_param_address_lo, fetch_param_address_hi, load_data, decrement_data, store_data, fetch_opcode },
	// ROR_absolute M
	[0x6E] = { fetch_param_address_lo, fetch_param_address_hi, load_data, rotate_right_data, store_data, fetch_opcode },
	// JSR_absolute
	[0x20] = { fetch_param_address_lo, fetch_and_waste, push_pch, push_pcl, fetch_param_address_hi, branch_any_page },
	// JMP_absolute
	[0x4C] = { fetch_param_address_lo, fetch_param_address_hi, branch_any_page },

	// STA_absoluteX W
	[0x9D] = { fetch_param_address_lo, fetch_param_address_hi, add_reg_x_to_address, store_reg_a, fetch_opcode },
	// LDA_absoluteX R
	[0xBD] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, put_data_into_reg_a },
	// LDY_absoluteX R
	[0xBC] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, put_data_into_reg_y },
	// AND_absoluteX R
	[0x3D] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, bitwise_and },
	// ORA_absoluteX R
	[0x1D] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, bitwise_or },
	// CMP_absoluteX R
	[0xDD] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, compare_reg_a },
	// ADC_absoluteX R
	[0x7D] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, add_with_carry },
	// SBC_absoluteX R
	[0xFD] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_x, add_reg_x_to_address, load_data, subtract_with_carry },
	// INC_absoluteX W
	[0xFE] = { fetch_param_address_lo, fetch_param_address_hi, add_reg_x_to_address, load_data, increment_data, store_data, fetch_opcode },
	// DEC_absoluteX W
	[0xDE] = { fetch_param_address_lo, fetch_param_address_hi, add_reg_x_to_address, load_data, decrement_data, store_data, fetch_opcode },

	// STA_absoluteY W
	[0x99] = { fetch_param_address_lo, fetch_param_address_hi, add_reg_y_to_address, store_reg_a, fetch_opcode },
	// LDA_absoluteY R
	[0xB9] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_y, add_reg_y_to_address, load_data, put_data_into_reg_a },
	// ORA_absoluteY R
	[0x19] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_y, add_reg_y_to_address, load_data, bitwise_or },
	// CMP_absoluteY R
	[0xD9] = { fetch_param_address_lo, fetch_param_address_hi_add_reg_y, add_reg_y_to_address, load_data, compare_reg_a },

	// STA_indirectY W
	[0x91] = { fetch_param_address_zp, load_address_lo, load_address_hi, add_reg_y_to_address, store_reg_a, fetch_opcode },
	// LDA_indirectY R
	[0xB1] = { fetch_param_address_zp, load_address_lo, load_address_hi_add_reg_y, add_reg_y_to_address, load_data, put_data_into_reg_a },
	// ORA_indirectY R
	[0x11] = { fetch_param_address_zp, load_address_lo, load_address_hi_add_reg_y, add_reg_y_to_address, load_data, bitwise_or },
	// CMP_indirectY R
	[0xD1] = { fetch_param_address_zp, load_address_lo, load_address_hi_add_reg_y, add_reg_y_to_address, load_data, compare_reg_a },
	// ADC_indirectY R
	[0x71] = { fetch_param_address_zp, load_address_lo, load_address_hi_add_reg_y, add_reg_y_to_address, load_data, add_with_carry },
	// SBC_indirectY R
	[0xF1] = { fetch_param_address_zp, load_address_lo, load_address_hi_add_reg_y, add_reg_y_to_address, load_data, subtract_with_carry },

	// JMP_indirect
	[0x6C] = { fetch_param_address_lo, fetch_param_address_hi, load_address_lo, load_address_hi, branch_any_page },

	// PHA_stack
	[0x48] = { fetch_and_waste, push_reg_a, fetch_opcode },
	// PHP_stack
	[0x08] = { fetch_and_waste, push_status, fetch_opcode },
	// PLA_stack
	[0x68] = { fetch_and_waste, fetch_and_waste, pull_data, put_data_into_reg_a },
	// PLP_stack
	[0x28] = { fetch_and_waste, fetch_and_waste, pull_data, put_data_into_status },
	// RTS_stack
	[0x60] = { fetch_param_data, fetch_and_waste, pull_pcl, pull_pch, fetch_param_data, fetch_opcode },
	// RTI_stack
	[0x40] = { fetch_param_data, fetch_and_waste, pull_status, pull_pcl, pull_pch, fetch_opcode },
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

