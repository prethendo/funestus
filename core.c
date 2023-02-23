#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "SDL2/SDL.h"

extern void cpu_exec(void);
extern void ppu_exec(void);
extern bool load_rom(char const *file_name);

static uint32_t FRAME_BUFFER_READY; // SDL2 event

static uint32_t frame_buffer[256 * 240];
static uint32_t nes_color[256];

static struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Palette *palette;
} sdl;

static int loop_emulation(void *arg) {
	(void) arg;

	while (true) {
		ppu_exec();
		cpu_exec();
		ppu_exec();
		ppu_exec();
	}
	return 0;
}

void display_frame_buffer(uint8_t const *internal_frame_buffer) {
	for (int i = 0; i < 256 * 240; i++) {
		//uint8_t color = internal_frame_buffer[i] * 21;
		uint8_t color = 0x22;
		switch (internal_frame_buffer[i]) {
			case 0: color = 0x20; break;
			case 1: color = 0x10; break;
			case 2: color = 0x00; break;
			case 3: color = 0x3F; break;
		}
		frame_buffer[i] = nes_color[color];
	}
	SDL_Event event;
	event.type = FRAME_BUFFER_READY;
	SDL_PushEvent(&event);
}

static bool initialize_sdl(void) {
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO); // SDL_LOG_PRIORITY_VERBOSE
	SDL_version version;
	SDL_GetVersion(&version);
	printf("Using SDL version %d.%d.%d\n", version.major, version.minor, version.patch);

	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		puts("Endianness is SDL_BIG_ENDIAN: abort");
		return false;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return false;

	sdl.window = SDL_CreateWindow("funestus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 768, 720, SDL_WINDOW_SHOWN);
	if (!sdl.window)
		return false;

	sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
	if (!sdl.renderer)
		return false;

	if (SDL_SetRenderDrawColor(sdl.renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE) != 0)
		return false;
	if (SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE) != 0)
		puts(SDL_GetError());


	SDL_PixelFormatEnum pixel_format_enum = SDL_GetWindowPixelFormat(sdl.window);
	if (pixel_format_enum == SDL_PIXELFORMAT_UNKNOWN)
		return false;
	puts(SDL_GetPixelFormatName(pixel_format_enum));

	sdl.texture = SDL_CreateTexture(sdl.renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, 256, 240);
	if (!sdl.texture)
		return false;

	// http://drag.wootest.net/misc/palgen.html
	uint8_t const A = SDL_ALPHA_OPAQUE;
	SDL_Color colors[64] = {
		{ 0x46, 0x46, 0x46, A }, { 0x00, 0x06, 0x5A, A }, { 0x00, 0x06, 0x78, A }, { 0x02, 0x06, 0x73, A },
		{ 0x35, 0x03, 0x4C, A }, { 0x57, 0x00, 0x0E, A }, { 0x5A, 0x00, 0x00, A }, { 0x41, 0x00, 0x00, A },
		{ 0x12, 0x02, 0x00, A }, { 0x00, 0x14, 0x00, A }, { 0x00, 0x1E, 0x00, A }, { 0x00, 0x1E, 0x00, A },
		{ 0x00, 0x15, 0x21, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A },
		{ 0x9D, 0x9D, 0x9D, A }, { 0x00, 0x4A, 0xB9, A }, { 0x05, 0x30, 0xE1, A }, { 0x57, 0x18, 0xDA, A },
		{ 0x9F, 0x07, 0xA7, A }, { 0xCC, 0x02, 0x55, A }, { 0xCF, 0x0B, 0x00, A }, { 0xA4, 0x23, 0x00, A },
		{ 0x5C, 0x3F, 0x00, A }, { 0x0B, 0x58, 0x00, A }, { 0x00, 0x66, 0x00, A }, { 0x00, 0x67, 0x13, A },
		{ 0x00, 0x5E, 0x6E, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A },
		{ 0xFE, 0xFF, 0xFF, A }, { 0x1F, 0x9E, 0xFF, A }, { 0x53, 0x76, 0xFF, A }, { 0x98, 0x65, 0xFF, A },
		{ 0xFC, 0x67, 0xFF, A }, { 0xFF, 0x6C, 0xB3, A }, { 0xFF, 0x74, 0x66, A }, { 0xFF, 0x80, 0x14, A },
		{ 0xC4, 0x9A, 0x00, A }, { 0x71, 0xB3, 0x00, A }, { 0x28, 0xC4, 0x21, A }, { 0x00, 0xC8, 0x74, A },
		{ 0x00, 0xBF, 0xD0, A }, { 0x2B, 0x2B, 0x2B, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A },
		{ 0xFE, 0xFF, 0xFF, A }, { 0x9E, 0xD5, 0xFF, A }, { 0xAF, 0xC0, 0xFF, A }, { 0xD0, 0xB8, 0xFF, A },
		{ 0xFE, 0xBF, 0xFF, A }, { 0xFF, 0xC0, 0xE0, A }, { 0xFF, 0xC3, 0xBD, A }, { 0xFF, 0xCA, 0x9C, A },
		{ 0xE7, 0xD5, 0x8B, A }, { 0xC5, 0xDF, 0x8E, A }, { 0xA6, 0xE6, 0xA3, A }, { 0x94, 0xE8, 0xC5, A },
		{ 0x92, 0xE4, 0xEB, A }, { 0xA7, 0xA7, 0xA7, A }, { 0x00, 0x00, 0x00, A }, { 0x00, 0x00, 0x00, A }
	};

	SDL_PixelFormat *pixel_format = SDL_AllocFormat(pixel_format_enum);
	if (!pixel_format)
		return false;
	for (int i = 0; i < 64; i++)
		nes_color[i] = SDL_MapRGBA(pixel_format, colors[i].r, colors[i].g, colors[i].b, colors[i].a);
	SDL_FreeFormat(pixel_format);

	sdl.palette = SDL_AllocPalette(64);
	if (!sdl.palette)
		return false;
	if (SDL_SetPaletteColors(sdl.palette, colors, 0, 64) != 0)
		return false;

	FRAME_BUFFER_READY = SDL_RegisterEvents(1);
	if (FRAME_BUFFER_READY == (uint32_t) -1)
		return false;

	SDL_Thread *thread = SDL_CreateThread(loop_emulation, NULL, (void *) NULL);
	if (!thread)
		return false;
	SDL_DetachThread(thread);

	return true;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		puts("ROM filename is missing!");
		return EXIT_FAILURE;
	}
	if (!load_rom(argv[1]))
		return EXIT_FAILURE;

	bool start_up = initialize_sdl();

	if (start_up) {
		puts("Emulation is afoot!\n");
		SDL_Event event;

		while (SDL_WaitEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
			if (event.type == FRAME_BUFFER_READY) {
				SDL_UpdateTexture(sdl.texture, NULL, frame_buffer, 256 * 4);
				SDL_RenderClear(sdl.renderer);
				SDL_RenderCopy(sdl.renderer, sdl.texture, NULL, NULL);
				SDL_RenderPresent(sdl.renderer);
			}
		} 
	}

	if (sdl.palette)
		SDL_FreePalette(sdl.palette);
	if (sdl.texture)
		SDL_DestroyTexture(sdl.texture);
	if (sdl.renderer)
		SDL_DestroyRenderer(sdl.renderer);
	if (sdl.window)
		SDL_DestroyWindow(sdl.window);
	if (strlen(SDL_GetError()))
		printf("SDL error: %s\n", SDL_GetError());
	SDL_Quit();

	puts("Finish!");
	if (!start_up)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

