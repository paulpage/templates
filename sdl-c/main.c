#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>

int main(int argc, char *argv[]) {
    int width = 800;
    int height = 600;

    if (SDL_Init(SDL_INIT_VIDEO != 0)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Playground", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    if (window == NULL) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0); SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_MOUSEWHEEL:
                    printf("wheel %f %f\n", event.wheel.preciseX, event.wheel.preciseY);
                    rect1.x += event.wheel.preciseX * 10;
                    rect1.y += event.wheel.preciseY * 10;
                    break;
                case SDL_MOUSEMOTION:
                    rect2.x = event.motion.x;
                    rect2.y = event.motion.y;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_q) {
                        show = true;
                    }
                    break;
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_q) {
                        show = false;
                    }
                    break;
            }
        }

        rect1.y += 0.01;

        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRectF(renderer, &rect1);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRectF(renderer, &rect2);

        if (show) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
            SDL_RenderFillRectF(renderer, &rect3);
        }

        SDL_RenderPresent(renderer);
    }

    return 0;
}
