#ifndef DEBUG
void dummy(char const *arg, ...) { (void) arg; }
#define printf dummy
#define puts (void)
#endif

#define _____ "---"

static char const * const mnemonic[256] = {
	/*0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F */
	"BRK", "ORA", _____, _____, _____, "ORA", "ASL", _____, "PHP", "ORA", "ASL", _____, _____, "ORA", "ASL", _____, /* 0 */
	"BPL", "ORA", _____, _____, _____, "ORA", "ASL", _____, "CLC", "ORA", _____, _____, _____, "ORA", "ASL", _____, /* 1 */
	"JSR", "AND", _____, _____, "BIT", "AND", "ROL", _____, "PLP", "AND", "ROL", _____, "BIT", "AND", "ROL", _____, /* 2 */
	"BMI", "AND", _____, _____, _____, "AND", "ROL", _____, "SEC", "AND", _____, _____, _____, "AND", "ROL", _____, /* 3 */
	"RTI", "EOR", _____, _____, _____, "EOR", "LSR", _____, "PHA", "EOR", "LSR", _____, "JMP", "EOR", "LSR", _____, /* 4 */
	"BVC", "EOR", _____, _____, _____, "EOR", "LSR", _____, "CLI", "EOR", _____, _____, _____, "EOR", "LSR", _____, /* 5 */
	"RTS", "ADC", _____, _____, _____, "ADC", "ROR", _____, "PLA", "ADC", "ROR", _____, "JMP", "ADC", "ROR", _____, /* 6 */
	"BVS", "ADC", _____, _____, _____, "ADC", "ROR", _____, "SEI", "ADC", _____, _____, _____, "ADC", "ROR", _____, /* 7 */
	_____, "STA", _____, _____, "STY", "STA", "STX", _____, "DEY", _____, "TXA", _____, "STY", "STA", "STX", _____, /* 8 */
	"BCC", "STA", _____, _____, "STY", "STA", "STX", _____, "TYA", "STA", "TXS", _____, _____, "STA", _____, _____, /* 9 */
	"LDY", "LDA", "LDX", _____, "LDY", "LDA", "LDX", _____, "TAY", "LDA", "TAX", _____, "LDY", "LDA", "LDX", _____, /* A */
	"BCS", "LDA", _____, _____, "LDY", "LDA", "LDX", _____, "CLV", "LDA", "TSX", _____, "LDY", "LDA", "LDX", _____, /* B */
	"CPY", "CMP", _____, _____, "CPY", "CMP", "DEC", _____, "INY", "CMP", "DEX", _____, "CPY", "CMP", "DEC", _____, /* C */
	"BNE", "CMP", _____, _____, _____, "CMP", "DEC", _____, "CLD", "CMP", _____, _____, _____, "CMP", "DEC", _____, /* D */
	"CPX", "SBC", _____, _____, "CPX", "SBC", "INC", _____, "INX", "SBC", "NOP", _____, "CPX", "SBC", "INC", _____, /* E */
	"BEQ", "SBC", _____, _____, _____, "SBC", "INC", _____, "SED", "SBC", _____, _____, _____, "SBC", "INC", _____  /* F */
};

static char const * const addressing[256] = {
	/*0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F */
	"Sta", "Pre", _____, _____, _____, "Zpg", "Zpg", _____, "Sta", "Imm", "Acc", _____, _____, "Abs", "Abs", _____, /* 0 */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____, /* 1 */
	"Abs", "Pre", _____, _____, "Zpg", "Zpg", "Zpg", _____, "Sta", "Imm", "Acc", _____, "Abs", "Abs", "Abs", _____, /* 2 */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____, /* 3 */
	"Sta", "Pre", _____, _____, _____, "Zpg", "Zpg", _____, "Sta", "Imm", "Acc", _____, "Abs", "Abs", "Abs", _____, /* 4 */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____, /* 5 */
	"Sta", "Pre", _____, _____, _____, "Zpg", "Zpg", _____, "Sta", "Imm", "Acc", _____, "Abi", "Abs", "Abs", _____, /* 6 */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____, /* 7 */
	_____, "Pre", _____, _____, "Zpg", "Zpg", "Zpg", _____, "Imp", _____, "Imp", _____, "Abs", "Abs", "Abs", _____, /* 8 */
	"Pcr", "Pos", _____, _____, "Zpx", "Zpx", "Zpy", _____, "Imp", "Aby", "Imp", _____, _____, "Abx", _____, _____, /* 9 */
	"Imm", "Pre", "Imm", _____, "Zpg", "Zpg", "Zpg", _____, "Imp", "Imm", "Imp", _____, "Abs", "Abs", "Abs", _____, /* A */
	"Pcr", "Pos", _____, _____, "Zpx", "Zpx", "Zpy", _____, "Imp", "Aby", "Imp", _____, "Abx", "Abx", "Aby", _____, /* B */
	"Imm", "Pre", _____, _____, "Zpg", "Zpg", "Zpg", _____, "Imp", "Imm", "Imp", _____, "Abs", "Abs", "Abs", _____, /* C */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____, /* D */
	"Imm", "Pre", _____, _____, "Zpg", "Zpg", "Zpg", _____, "Imp", "Imm", "Imp", _____, "Abs", "Abs", "Abs", _____, /* E */
	"Pcr", "Pos", _____, _____, _____, "Zpx", "Zpx", _____, "Imp", "Aby", _____, _____, _____, "Abx", "Abx", _____  /* F */
};

