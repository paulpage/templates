#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#include "stb_truetype.h"

#define PJP_IMPLEMENTATION
#include "pjp.h"

#define ASSERT_CALL(call) \
    do { \
        if (!(call)) { \
            SDL_Log("Error in %s: %s", #call, SDL_GetError()); \
            SDL_Quit(); \
            return 1; \
        } \
    } while (0)

#define ASSERT_CREATED(obj) do { if ((obj) == NULL) { SDL_Log("Error: %s is null", #obj); SDL_Quit(); return 1; }} while (0)

#define FONT_SIZE 24.0f
#define ATLAS_WIDTH 512
#define ATLAS_HEIGHT 512

typedef struct {
    SDL_Texture* texture;
    stbtt_packedchar char_data[96];
    float scale;
} Font;

Font load_font(SDL_Renderer* renderer, const char* font_path) {
    Font font = {0};
    size_t font_size = 0;
    unsigned char *font_buffer = read_file(font_path, &font_size);
    printf("font file size: %zd\n", font_size);

    unsigned char *atlas_data = malloc(ATLAS_WIDTH * ATLAS_HEIGHT);

    stbtt_pack_context pack_context = {0};

    stbtt_pack_range pack_range = {0};
    pack_range.font_size = FONT_SIZE;
    pack_range.first_unicode_codepoint_in_range = 32;
    pack_range.num_chars = 96;
    pack_range.chardata_for_range = font.char_data;

    stbtt_PackBegin(&pack_context, atlas_data, ATLAS_WIDTH, ATLAS_HEIGHT, 0, 1, NULL);
    stbtt_PackFontRanges(&pack_context, font_buffer, 0, &pack_range, 1);

    stbtt_PackEnd(&pack_context);

	Uint32* pixels = malloc(ATLAS_WIDTH * ATLAS_HEIGHT * sizeof(Uint32));
	const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
	for(int i = 0; i < ATLAS_WIDTH * ATLAS_HEIGHT; i++) {
		pixels[i] = SDL_MapRGBA(format, NULL, 0xff, 0xff, 0xff, atlas_data[i]);
	}

    font.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, ATLAS_WIDTH, ATLAS_HEIGHT);
    SDL_SetTextureBlendMode(font.texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(font.texture, NULL, pixels, ATLAS_WIDTH * sizeof(Uint32));

	free(pixels);

    free(atlas_data);
    free(font_buffer);

    return font;
}

void draw_text(SDL_Renderer* renderer, Font* font, const char *text, float x, float y) {
    Uint8 r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(font->texture, r, g, b);
	SDL_SetTextureAlphaMod(font->texture, a);

    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad quad;
            stbtt_GetPackedQuad(font->char_data, ATLAS_WIDTH, ATLAS_HEIGHT, *text - 32, &x, &y, &quad, 1);
            
            SDL_FRect src_rect = {
                quad.s0 * ATLAS_WIDTH,
                quad.t0 * ATLAS_HEIGHT,
                (quad.s1 - quad.s0) * ATLAS_WIDTH,
                (quad.t1 - quad.t0) * ATLAS_HEIGHT
            };
            
            SDL_FRect dst_rect = {
                quad.x0,
                quad.y0,
                quad.x1 - quad.x0,
                quad.y1 - quad.y0
            };

            SDL_RenderTexture(renderer, font->texture, &src_rect, &dst_rect);
        }
        ++text;
    }
}

int main(int argc, char *argv[]) {

    size_t file_size = 0, line_count = 0;
    char **buf = read_file_lines("render.c", &file_size, &line_count);
    printf("file_size: %zd\nline_count: %zd\n", file_size, line_count);

    float scroll_offset = 0;
    bool mouse_down = false;

    int width = 800;
    int height = 600;

    ASSERT_CALL(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = SDL_CreateWindow("Playground", width, height, 0);
    ASSERT_CREATED(window);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    ASSERT_CREATED(renderer);

    Font font = load_font(renderer, "../res/fonts/vera/Vera.ttf");

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        /*SDL_WaitEvent(&event);*/
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_Q) {
                        quit = true;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = true;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = false;
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (mouse_down) {
                        scroll_offset += event.motion.yrel;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    scroll_offset += event.wheel.y * 20.0f;
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 100, 100, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < line_count; i++) {
            draw_text(renderer, &font, buf[i], 20, 20 + i * 20);
        }


        /* SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); */
        /* SDL_FRect rect = {300.0f, 200.0f, 200.0f, 200.0f}; */
        /* SDL_RenderFillRect(renderer, &rect); */



        /* SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); */
        /* for (int i = 0; i < line_count; i++) { */
        /*     float offset = scroll_offset; */
        /*     draw_text(renderer, &font, buf[i], 0, i * 20 + offset); */
        /* } */

        /* SDL_FRect src_rect = { */
        /*     0.0f, 0.0f, ATLAS_WIDTH, ATLAS_HEIGHT, */
        /* }; */
        /* SDL_FRect dst_rect = { */
        /*     100.0f, 100.0f, 400.0f, 400.0f, */
        /* }; */
        /* SDL_RenderTexture(renderer, font.texture, &src_rect, &dst_rect); */

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
