#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

uint8_t *prg;
uint8_t *chr;

// basic crc32 calculation with inverted polynomial and without lookup table
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

static void unload_rom() {
	if (prg)
		free(prg);
	if (chr)
		free(chr);
	printf("ROM unloaded\n");
}

bool load_rom(char const *file_name) {
	uint8_t *header = read_file_chunk(file_name, 0, 16);
	if (!header)
		return false;
	free(header); // for now, just discard the header

	atexit(unload_rom);

	prg = read_file_chunk(file_name, 16, 16384); // 16k == 0x4000
	if (!prg)
		return false;

	chr = read_file_chunk(file_name, 16 + 16384, 8192); // 8k == 0x2000
	if (!chr)
		return false;

	uint8_t *full_rom = read_file_chunk(file_name, 16, 24576); // 24k == 0x6000
	if (full_rom) {
		printf("ROM CRC32: %08X\n", calculate_crc32(full_rom, 24576));
		free(full_rom);
	}

	return true;
}

