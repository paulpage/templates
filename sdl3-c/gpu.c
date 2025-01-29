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

typedef struct Vec2 {
    float x, y;
} Vec2;

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
    SDL_GPUShader *vertex_shader = load_shader(gpu, "shaders/2d.vert", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader *fragment_shader = load_shader(gpu, "shaders/2d.frag", SDL_GPU_SHADERSTAGE_FRAGMENT);

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
    SDL_GPUColorTargetBlendState blend_state = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true,
    };
    SDL_GPUColorTargetDescription color_target_info = {0};
    color_target_info.format = SDL_GetGPUSwapchainTextureFormat(gpu, window);
    color_target_info.blend_state = blend_state;

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target_info;
    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;

    vertex_buffer_info.slot = 0;
    vertex_buffer_info.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    vertex_buffer_info.instance_step_rate = 4;
    vertex_buffer_info.pitch = sizeof(VertInput);

    // dst_rect
    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = 0;

    // dst_rect
    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = sizeof(float) * 4;

    // colors[0]
    vertex_attributes[2].buffer_slot = 0;
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[2].location = 2;
    vertex_attributes[2].offset = sizeof(float) * 8;

    // colors[1]
    vertex_attributes[3].buffer_slot = 0;
    vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[3].location = 3;
    vertex_attributes[3].offset = sizeof(float) * 12;

    // colors[2]
    vertex_attributes[4].buffer_slot = 0;
    vertex_attributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[4].location = 4;
    vertex_attributes[4].offset = sizeof(float) * 16;

    // colors[3]
    vertex_attributes[5].buffer_slot = 0;
    vertex_attributes[5].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[5].location = 5;
    vertex_attributes[5].offset = sizeof(float) * 20;

    // corner_radius
    vertex_attributes[6].buffer_slot = 0;
    vertex_attributes[6].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    vertex_attributes[6].location = 6;
    vertex_attributes[6].offset = sizeof(float) * 24;

    // edge_softness
    vertex_attributes[7].buffer_slot = 0;
    vertex_attributes[7].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    vertex_attributes[7].location = 7;
    vertex_attributes[7].offset = sizeof(float) * 25;

    // border_thickness
    vertex_attributes[8].buffer_slot = 0;
    vertex_attributes[8].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    vertex_attributes[8].location = 8;
    vertex_attributes[8].offset = sizeof(float) * 26;

    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_info;
    pipeline_info.vertex_input_state.num_vertex_attributes = 9;
    pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;

    pipeline_info.props = 0;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    ASSERT_CREATED(pipeline);

    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);







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

        SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu);
        ASSERT_CREATED(cmdbuf);

        SDL_GPUTexture *swapchain_texture = NULL;
        ASSERT_CALL(SDL_AcquireGPUSwapchainTexture(cmdbuf, window, &swapchain_texture, NULL, NULL));

        if (swapchain_texture) {
            SDL_GPUColorTargetInfo color_target_info = {
                .texture = swapchain_texture,
                .clear_color = (SDL_FColor){0.0f, 0.5f, 0.0f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            };

            SDL_GPUBufferBinding vertex_binding = {
                .buffer = buf_vertex,
                .offset = 0,
            };

            Vec2 screen_size = {800.0f, 600.0f};
            SDL_PushGPUVertexUniformData(cmdbuf, 0, &screen_size, sizeof(Vec2));
            SDL_PushGPUFragmentUniformData(cmdbuf, 0, &screen_size, sizeof(Vec2));

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
            SDL_DrawGPUPrimitives(render_pass, 8, 2, 0, 0);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(cmdbuf);
    }

    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
SDL_Quit();
    return 0;
}
