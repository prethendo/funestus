/****************************************************** Status setting *****************************************************/
static void set_flag_i(void) {
	puts(__FUNCTION__);
	flag.i = true;
	bus.rw = READ;
}

static void clear_flag_d(void) {
	puts(__FUNCTION__);
	flag.d = false;
	bus.rw = READ;
}

/********************************************************* Logical *********************************************************/
static void bitwise_and(void) {
	puts(__FUNCTION__);
	reg.a &= bus.data;
	update_flags_nz(reg.a);
	bus.address = ++reg.pc;
	bus.rw = READ;
}

/**************************************************** Register transfer ****************************************************/
static void transfer_reg_x_to_reg_s(void) {
	puts(__FUNCTION__);
	reg.s = reg.x;
	bus.rw = READ;
}

/******************************************************* Store/Load ********************************************************/
static void put_data_into_reg_a(void) {
	puts(__FUNCTION__);
	reg.a = bus.data;
	update_flags_nz(reg.a);
	bus.address = ++reg.pc;
	bus.rw = READ;
}

static void put_data_into_reg_x(void) {
	puts(__FUNCTION__);
	reg.x = bus.data;
	update_flags_nz(reg.x);
	bus.address = ++reg.pc;
	bus.rw = READ;
}

static void store_reg_a_at_absolute_address(void) {
	puts(__FUNCTION__);
	bus.address_lo = transient_data;
	bus.address_hi = bus.data;
	bus.data = reg.a;
	bus.rw = WRITE;
	reg.pc++;
}

static void load_data_from_absolute_address(void) {
	puts(__FUNCTION__);
	bus.address_lo = transient_data;
	bus.address_hi = bus.data;
	bus.rw = READ;
}

static void wrap_up_absolute_store(void) {
	puts(__FUNCTION__);
	bus.address = reg.pc;
	bus.rw = READ;
}

static void load_absolute_address(void) {
	puts(__FUNCTION__);
	transient_data = bus.data;
	bus.address = ++reg.pc;
	bus.rw = READ;
}

static void load_interrupt_vector_lo(void) {
	puts(__FUNCTION__);
	bus.address = 0xFFFC; // reset vector
	flag.i = true;
	bus.rw = READ;
}

static void load_interrupt_vector_hi(void) {
	puts(__FUNCTION__);
	bus.address = 0xFFFD; // reset vector
	transient_data = bus.data;
	flag.b = true;
	bus.rw = READ;
}

static void put_interrupt_address_into_pc(void) {
	puts(__FUNCTION__);
	bus.address_lo = transient_data;
	bus.address_hi = bus.data;
	reg.pc = bus.address;
	bus.rw = READ;
}

/******************************************************** Branching ********************************************************/
static void check_if_flag_z_is_set(void) {
	puts(__FUNCTION__);
	transient_data = bus.data;
	bus.address = ++reg.pc;
	bus.rw = READ;
	if (!flag.z)
		current_step += 2;
}

static void branch_same_page(void) {
	puts(__FUNCTION__);
	uint16_t new_address = reg.pc + (int8_t) transient_data;
	uint8_t new_address_hi = new_address >> 8;
	uint8_t new_address_lo = new_address & 0x00FF;

	reg.pcl = new_address_lo;
	bus.address = reg.pc;
	bus.rw = READ;

	if (reg.pch == new_address_hi) // same page
		current_step++;
	else
		transient_data = new_address_hi;
}

static void branch_any_page(void) {
	puts(__FUNCTION__);
	reg.pch = transient_data;
	bus.address = reg.pc;
	bus.rw = READ;
}

/********************************************************** Stack **********************************************************/
static void push_pch(void) {
	puts(__FUNCTION__);
	bus.address_lo = reg.s--;
	bus.address_hi = 0x01;
	bus.data = reg.pch;
	bus.rw = READ; // only on reset
}

static void push_pcl(void) {
	puts(__FUNCTION__);
	bus.address_lo = reg.s--;
	bus.address_hi = 0x01;
	bus.data = reg.pcl;
	bus.rw = READ; // only on reset
}

static void push_flags(void) {
	puts(__FUNCTION__);
	bus.address_lo = reg.s--;
	bus.address_hi = 0x01;
	bus.data = group_status_flags();
	bus.rw = READ; // only on reset
}

/********************************************************** Fetch **********************************************************/
static void decode_opcode(void) {
	puts(__FUNCTION__);
	current_step = set[bus.data];
	bus.address = ++reg.pc;
	printf("\nDECODE %02X \033[1;33m %s \033[0m %s\n", bus.data, mnemonic[bus.data], addressing[bus.data]);
}

