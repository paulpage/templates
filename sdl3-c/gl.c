#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdbool.h>

#define ASSERT_CALL(call) \
    do { \
        if (!(call)) { \
            SDL_Log("Error in %s: %s", #call, SDL_GetError()); \
            SDL_Quit(); \
            return 1; \
        } \
    } while (0)

#define ASSERT_CREATED(obj) do { if ((obj) == NULL) { SDL_Log("Error: %s is null", #obj); SDL_Quit(); return 1; }} while (0)

int main(void) {
    ASSERT_CALL(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_OPENGL);
    ASSERT_CREATED(window);

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    ASSERT_CREATED(glcontext);

    bool quit = false;
    SDL_Event event;
    while (!quit) {
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
            }
        }

        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
