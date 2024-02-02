#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

int main(int argc, char *argv[]) {
    int width = 800;
    int height = 600;

    if (SDL_Init(SDL_INIT_VIDEO != 0)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Playground", width, height, 0);
    if (window == NULL) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                printf("wheel %f %f\n", event.wheel.x, event.wheel.y);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                size_t n = 0;
                char *data = (char*)SDL_GetClipboardData("image/png", &n);
                printf("data: %ld\n", n);
        }

        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    return 0;
}
