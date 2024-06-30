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
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    SDL_FRect rect1 = {0, 0, 100, 100};
    SDL_FRect rect2 = {0, 0, 100, 100};
    SDL_FRect rect3 = {0, 0, 100, 100};
    bool show = false;

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        /*SDL_WaitEvent(&event);*/
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    printf("wheel %f %f\n", event.wheel.x, event.wheel.y);
                    rect1.x += event.wheel.x * 10;
                    rect1.y += event.wheel.y * 10;
                    break;
                /*case SDL_EVENT_MOUSE_BUTTON_DOWN:*/
                /*    break;*/
                    /*size_t n = 0;*/
                    /*char *data = (char*)SDL_GetClipboardData("image/png", &n);*/
                    /*printf("data: %ld\n", n);*/
                case SDL_EVENT_MOUSE_MOTION:
                    rect2.x = event.motion.x;
                    rect2.y = event.motion.y;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_q) {
                        show = true;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    if (event.key.key == SDLK_q) {
                        show = false;
                    }
                    break;
            }
        }

        rect1.y += 0.01;

        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &rect1);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect2);

        if (show) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
            SDL_RenderFillRect(renderer, &rect2);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}
