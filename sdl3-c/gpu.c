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
            exit(1); \
        } \
    } while (0)

#define ASSERT_CREATED(obj) do { if ((obj) == NULL) { SDL_Log("Error: %s is null", #obj); SDL_Quit(); exit(1); }} while (0)

typedef struct Rect {
    float x, y, w, h;
} Rect;

typedef struct Color {
    float r, g, b, a;
} Color;

typedef struct VertInput {
    Rect dst_rect;
    Rect src_rect;
    Color colors[4];
    float corner_radius;
    float edge_softness;
    float border_thickness;
} VertInput;

SDL_GPUShader *load_shader(SDL_GPUDevice *gpu, char *filename, SDL_GPUShaderStage stage) {
    size_t len;
    unsigned char *data = read_file(filename, &len);
    SDL_GPUShaderCreateInfo info = {
        .code_size = len,
        .code = data,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0, // TODO probably change
        .num_uniform_buffers = 1,
    };

    SDL_GPUShader *shader = SDL_CreateGPUShader(gpu, &info);
    ASSERT_CREATED(shader);
    return shader;
}

int main(int argc, char *argv[]) {

    float scroll_offset = 0;
    bool mouse_down = false;

    int width = 800;
    int height = 600;

    ASSERT_CALL(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = SDL_CreateWindow("Playground", width, height, 0);
    ASSERT_CREATED(window);

    // GPU Init
    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    ASSERT_CREATED(gpu);
    ASSERT_CALL(SDL_ClaimWindowForGPUDevice(gpu, window));

    // Shaders
    SDL_GPUShader *vert = load_shader(gpu, "shaders/2d.vert", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader *frag = load_shader(gpu, "shaders/2d.frag", SDL_GPU_SHADERSTAGE_FRAGMENT);

    // Buffer data
    VertInput vertices[2] = {
        (VertInput){
            .dst_rect = (Rect){10.0f, 20.0f, 100.0f, 200.0f},
            .src_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f},
            .colors = {
                (Color){1.0f, 1.0f, 0.0f, 1.0f},
                (Color){1.0f, 1.0f, 1.0f, 1.0f},
                (Color){1.0f, 0.0f, 0.0f, 1.0f},
                (Color){0.0f, 1.0f, 0.0f, 1.0f},
            },
            .corner_radius = 5.0f,
            .edge_softness = 1.0f,
            .border_thickness = 2.0f,
        },
        (VertInput){
            .dst_rect = (Rect){30.0f, 40.0f, 100.0f, 200.0f},
            .src_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f},
            .colors = {
                (Color){1.0f, 1.0f, 0.0f, 1.0f},
                (Color){1.0f, 1.0f, 1.0f, 1.0f},
                (Color){1.0f, 0.0f, 0.0f, 1.0f},
                (Color){0.0f, 1.0f, 0.0f, 1.0f},
            },
            .corner_radius = 5.0f,
            .edge_softness = 1.0f,
            .border_thickness = 2.0f,
        },
    };

    unsigned int buf_size = 2 * sizeof(VertInput);

    // Buffers
    SDL_GPUBufferCreateInfo buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = buf_size,
        .props = 0,
    };
    SDL_GPUBuffer *buf_vertex = SDL_CreateGPUBuffer(gpu, &buffer_info);
    ASSERT_CREATED(buf_vertex);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = buf_size,
        .props = 0,
    };
    SDL_GPUTransferBuffer *buf_transfer = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_info);

    void *map = SDL_MapGPUTransferBuffer(gpu, buf_transfer, false);
    memcpy(&vertices, map, buf_size);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation buf_location = {
        .transfer_buffer = buf_transfer,
        .offset = 0,
    };
    SDL_GPUBufferRegion dst_region = (SDL_GPUBufferRegion){
        .buffer = buf_vertex,
            .offset = 0,
            .size = buf_size,
    };
    SDL_UploadToGPUBuffer(copy_pass, &buf_location, &dst_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(gpu, buf_transfer);

    // Pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};
    SDL_GPUVertexBufferDescription vertex_buffer_info = {0};
    SDL_GPUVertexAttribute vertex_attributes[9] = {0};


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

    }

    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
SDL_Quit();
    return 0;
}
