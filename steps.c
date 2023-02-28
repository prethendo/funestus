/********************************************************** Fetch **********************************************************/
static void fetch_opcode(void) {
	puts(__FUNCTION__);
	uint8_t next = read_memory(reg.pc);
	if (interrupt_vector) {
		next = 0x00;
		fprintf(stdout, "\n\033[1;42m CPU interrupt \033[0m\n");
	} else {
		reg.pc++;
	}
	current_step = set[next];
	printf("\nFETCH %02X \033[1;33m %s \033[0m %s\n", next, mnemonic[next], addressing[next]);
}

static void fetch_param_address_zp(void) {
	puts(__FUNCTION__);
	transient.address = read_memory(reg.pc++);
}

static void fetch_param_address_lo(void) {
	puts(__FUNCTION__);
	transient.address_lo = read_memory(reg.pc++);
}

static void fetch_param_address_hi(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(reg.pc++);
}

static void fetch_param_data(void) {
	puts(__FUNCTION__);
	transient.data = read_memory(reg.pc++);
}

static void fetch_and_waste(void) {
	puts(__FUNCTION__);
	read_memory(reg.pc);
}

static void fetch_param_address_hi_add_reg_x(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(reg.pc++);
	if (transient.address_lo + reg.x < 0x0100) { // add X if the sum results in an address in the same page
		transient.address_lo += reg.x;
		current_step++;
	}
}

static void fetch_param_address_hi_add_reg_y(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(reg.pc++);
	if (transient.address_lo + reg.y < 0x0100) { // add Y if the sum results in an address in the same page
		transient.address_lo += reg.y;
		current_step++;
	}
}

/***************************************************************************************************************************/
static void add_reg_x_to_address(void) {
	puts(__FUNCTION__);
	transient.address += reg.x;
	read_memory(reg.pc);
}

static void add_reg_x_to_address_lo(void) {
	puts(__FUNCTION__);
	transient.address_lo += reg.x;
	read_memory(reg.pc);
}

/***************************************************** Status setting ******************************************************/
static void set_flag_c(void) {
	puts(__FUNCTION__);
	flag.c = true;
	fetch_opcode();
}

static void set_flag_i(void) {
	puts(__FUNCTION__);
	flag.i = true;
	fetch_opcode();
}

static void clear_flag_c(void) {
	puts(__FUNCTION__);
	flag.c = false;
	fetch_opcode();
}

static void clear_flag_d(void) {
	puts(__FUNCTION__);
	flag.d = false;
	fetch_opcode();
}

/******************************************************* Arithmetic ********************************************************/
static void add_with_carry(void) {
	puts(__FUNCTION__);
	uint16_t sum = reg.a + transient.data + flag.c;
	flag.c = (sum & 0x0100);
	// overflow: if both operands are positive, the result must be positive (same if both are negative)
	flag.v = ~(reg.a ^ transient.data) & (reg.a ^ sum) & 0x80;
	reg.a = sum;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void subtract_with_carry(void) {
	puts(__FUNCTION__);
	transient.data ^= 0xFF;
	add_with_carry();
}

static void increment_reg_x(void) {
	puts(__FUNCTION__);
	reg.x++;
	update_flags_nz(reg.x);
	fetch_opcode();
}

static void increment_reg_y(void) {
	puts(__FUNCTION__);
	reg.y++;
	update_flags_nz(reg.y);
	fetch_opcode();
}

static void increment_data(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, transient.data);
	transient.data++;
}

static void decrement_reg_x(void) {
	puts(__FUNCTION__);
	reg.x--;
	update_flags_nz(reg.x);
	fetch_opcode();
}

static void decrement_reg_y(void) {
	puts(__FUNCTION__);
	reg.y--;
	update_flags_nz(reg.y);
	fetch_opcode();
}

static void decrement_data(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, transient.data);
	transient.data--;
}

/********************************************************* Logical *********************************************************/
static void bitwise_and(void) {
	puts(__FUNCTION__);
	reg.a &= transient.data;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void bitwise_or(void) {
	puts(__FUNCTION__);
	reg.a |= transient.data;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void bitwise_xor(void) {
	puts(__FUNCTION__);
	reg.a ^= transient.data;
	update_flags_nz(reg.a);
	fetch_opcode();
}

/***************************************************************************************************************************/
static void bit_test(void) {
	puts(__FUNCTION__);
	flag.n = (transient.data & 0x80);
	flag.v = (transient.data & 0x40);
	flag.z = !(transient.data & reg.a);
	fetch_opcode();
}

/******************************************************* Comparison ********************************************************/
static void compare_reg_a(void) {
	puts(__FUNCTION__);
	flag.c = (reg.a >= transient.data);
	update_flags_nz(reg.a - transient.data);
	fetch_opcode();
}

static void compare_reg_x(void) {
	puts(__FUNCTION__);
	flag.c = (reg.x >= transient.data);
	update_flags_nz(reg.x - transient.data);
	fetch_opcode();
}

static void compare_reg_y(void) {
	puts(__FUNCTION__);
	flag.c = (reg.y >= transient.data);
	update_flags_nz(reg.y - transient.data);
	fetch_opcode();
}

/**************************************************** Register transfer ****************************************************/
static void transfer_reg_a_to_reg_x(void) {
	puts(__FUNCTION__);
	reg.x = reg.a;
	update_flags_nz(reg.x);
	fetch_opcode();
}

static void transfer_reg_a_to_reg_y(void) {
	puts(__FUNCTION__);
	reg.y = reg.a;
	update_flags_nz(reg.y);
	fetch_opcode();
}

static void transfer_reg_x_to_reg_a(void) {
	puts(__FUNCTION__);
	reg.a = reg.x;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void transfer_reg_x_to_reg_s(void) {
	puts(__FUNCTION__);
	reg.s = reg.x;
	fetch_opcode();
}

static void transfer_reg_y_to_reg_a(void) {
	puts(__FUNCTION__);
	reg.a = reg.y;
	update_flags_nz(reg.a);
	fetch_opcode();
}

/**************************************************** Bit manipulation *****************************************************/
static void shift_left_reg_a(void) {
	puts(__FUNCTION__);
	flag.c = (reg.a & 0x80);
	reg.a <<= 1;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void shift_left_data(void) {
	puts(__FUNCTION__);
	flag.c = (transient.data & 0x80);
	transient.data <<= 1;
	read_memory(reg.pc);
}

static void shift_right_reg_a(void) {
	puts(__FUNCTION__);
	flag.c = (reg.a & 0x01);
	reg.a >>= 1;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void shift_right_data(void) {
	puts(__FUNCTION__);
	flag.c = (transient.data & 0x01);
	transient.data >>= 1;
	read_memory(reg.pc);
}

static void rotate_right_reg_a(void) {
	puts(__FUNCTION__);
	bool carry = flag.c;
	flag.c = (reg.a & 0x01);
	reg.a >>= 1;
	if (carry)
		reg.a |= 0x80;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void rotate_right_data(void) {
	puts(__FUNCTION__);
	bool carry = flag.c;
	flag.c = (transient.data & 0x01);
	transient.data >>= 1;
	if (carry)
		transient.data |= 0x80;
	read_memory(reg.pc);
}

static void rotate_left_reg_a(void) {
	puts(__FUNCTION__);
	bool carry = flag.c;
	flag.c = (reg.a & 0x80);
	reg.a <<= 1;
	if (carry)
		reg.a |= 0x01;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void rotate_left_data(void) {
	puts(__FUNCTION__);
	bool carry = flag.c;
	flag.c = (transient.data & 0x80);
	transient.data <<= 1;
	if (carry)
		transient.data |= 0x01;
	read_memory(reg.pc);
}

/******************************************************** Branching ********************************************************/
static void skip_on_flag_z_clear(void) {
	puts(__FUNCTION__);
	if (!flag.z)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_n_clear(void) {
	puts(__FUNCTION__);
	if (!flag.n)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_c_clear(void) {
	puts(__FUNCTION__);
	if (!flag.c)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_v_clear(void) {
	puts(__FUNCTION__);
	if (!flag.v)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_z_set(void) {
	puts(__FUNCTION__);
	if (flag.z)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_n_set(void) {
	puts(__FUNCTION__);
	if (flag.n)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void skip_on_flag_c_set(void) {
	puts(__FUNCTION__);
	if (flag.c)
		fetch_opcode();
	else
		read_memory(reg.pc);
}

static void branch_same_page(void) {
	puts(__FUNCTION__);
	transient.address = reg.pc + (int8_t) transient.data;
	if (reg.pch == transient.address_hi) {
		reg.pc = transient.address;
		fetch_opcode();
	} else {
		reg.pcl = transient.address_lo;
	}
}

static void branch_any_page(void) {
	puts(__FUNCTION__);
	reg.pc = transient.address;
	fetch_opcode();
}

/******************************************************* Store/Load ********************************************************/
static void put_data_into_reg_a(void) {
	puts(__FUNCTION__);
	reg.a = transient.data;
	update_flags_nz(reg.a);
	fetch_opcode();
}

static void put_data_into_reg_x(void) {
	puts(__FUNCTION__);
	reg.x = transient.data;
	update_flags_nz(reg.x);
	fetch_opcode();
}

static void put_data_into_reg_y(void) {
	puts(__FUNCTION__);
	reg.y = transient.data;
	update_flags_nz(reg.y);
	fetch_opcode();
}

static void put_data_into_status(void) {
	puts(__FUNCTION__);
	ungroup_status_flags(transient.data);
	fetch_opcode();
}

static void store_reg_a(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, reg.a);
}

static void store_reg_x(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, reg.x);
}

static void store_reg_y(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, reg.y);
}

static void store_data(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, transient.data);
	update_flags_nz(transient.data);
}

static void load_data(void) {
	puts(__FUNCTION__);
	transient.data = read_memory(transient.address);
}

static void load_address_lo(void) {
	puts(__FUNCTION__);
	transient.data = read_memory(transient.address);
}

static void load_address_hi(void) {
	puts(__FUNCTION__);
	transient.address_lo += 1; // JMP_ind and STA_indY do not cross page boundaries here
	transient.address_hi = read_memory(transient.address);
	transient.address_lo = transient.data;
}

static void load_address_hi_add_reg_y(void) { // add only if the sum of address and reg.y remains in same memory page
	puts(__FUNCTION__);
	load_address_hi();
	if (transient.address_lo + reg.y < 0x0100) {
		transient.address_lo += reg.y;
		current_step++;
	}
}

/***************************************************************************************************************************/
static void add_reg_y_to_address(void) {
	puts(__FUNCTION__);
	transient.address += reg.y;
	read_memory(transient.address);
}

/********************************************************* Stack ***********************************************************/
static void push_reg_a(void) {
	puts(__FUNCTION__);
	write_memory(0x100 | reg.s--, reg.a);
}

static void push_pch(void) {
	puts(__FUNCTION__);
	if (interrupt_vector == RESET)
		read_memory(0x100 | reg.s--);
	else
		write_memory(0x100 | reg.s--, reg.pch);
}

static void push_pcl(void) {
	puts(__FUNCTION__);
	if (interrupt_vector == RESET)
		read_memory(0x100 | reg.s--);
	else
		write_memory(0x100 | reg.s--, reg.pcl);
}

static void pull_pch(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(0x100 | ++reg.s);
	reg.pc = transient.address;
}

static void pull_pcl(void) {
	puts(__FUNCTION__);
	transient.address_lo= read_memory(++reg.s | 0x100);
}

static void pull_data(void) {
	puts(__FUNCTION__);
	transient.data = read_memory(0x100 | ++reg.s);
}

static void push_status(void) {
	puts(__FUNCTION__);
	flag.b = interrupt_vector ? true : false;
	if (interrupt_vector == RESET)
		read_memory(0x100 | reg.s--);
	else
		write_memory(0x100 | reg.s--, group_status_flags());
	if (interrupt_vector) // when BRK it should be true as well -> unimplemented
		flag.i = true;
}

static void pull_status(void) {
	puts(__FUNCTION__);
	ungroup_status_flags(read_memory(++reg.s | 0x100));
}

static void load_interrupt_vector_lo(void) {
	puts(__FUNCTION__);
	transient.address_lo = read_memory(interrupt_vector);
}

static void load_interrupt_vector_hi(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(interrupt_vector + 1);
	reg.pc = transient.address;
	interrupt_vector = NONE;
}

