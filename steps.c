/********************************************************** Fetch **********************************************************/
static void fetch_opcode(void) {
	puts(__FUNCTION__);
	uint8_t data = read_memory(reg.pc++);
	if (interrupt_vector) {
		current_step = set[0x00];
		puts("Interrupt requested!");
		getchar();
	} else {
		current_step = set[data];
	}
	printf("\nFETCH %02X \033[1;33m %s \033[0m %s\n", data, mnemonic[data], addressing[data]);
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

/***************************************************** Status setting ******************************************************/
static void set_flag_i(void) {
	puts(__FUNCTION__);
	flag.i = true;
	fetch_opcode();
}

static void clear_flag_d(void) {
	puts(__FUNCTION__);
	flag.d = false;
	fetch_opcode();
}

/********************************************************* Logical *********************************************************/
static void bitwise_and(void) {
	puts(__FUNCTION__);
	reg.a &= transient.data;
	update_flags_nz(reg.a);
	fetch_opcode();
}

/**************************************************** Register transfer ****************************************************/
static void transfer_reg_x_to_reg_s(void) {
	puts(__FUNCTION__);
	reg.s = reg.x;
	fetch_opcode();
}

/******************************************************** Branching ********************************************************/
static void check_flag_z_set(void) {
	puts(__FUNCTION__);
	if (!flag.z)
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

static void store_reg_a(void) {
	puts(__FUNCTION__);
	write_memory(transient.address, reg.a);
}

static void load_data(void) {
	puts(__FUNCTION__);
	transient.data = read_memory(transient.address);
}

/********************************************************* Stack ***********************************************************/
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

static void push_status(void) {
	puts(__FUNCTION__);
	if (interrupt_vector == RESET)
		read_memory(0x100 | reg.s--);
	else
		write_memory(0x100 | reg.s--, group_status_flags());
	if (interrupt_vector)
		flag.i = true;
}

static void load_interrupt_vector_lo(void) {
	puts(__FUNCTION__);
	transient.address_lo = read_memory(interrupt_vector);
	flag.b = true;
}

static void load_interrupt_vector_hi(void) {
	puts(__FUNCTION__);
	transient.address_hi = read_memory(interrupt_vector + 1);
	reg.pc = transient.address;
	interrupt_vector = NONE;
}

