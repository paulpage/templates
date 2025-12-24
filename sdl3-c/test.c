#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <windows.h>

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

#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <windows.h>

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
