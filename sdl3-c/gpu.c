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

typedef union Vec4 {
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
} Vec4;

typedef struct Rect {
    float x, y, w, h;
} Rect;

typedef struct Vec2 {
    float x, y;
} Vec2;

typedef struct VertInput {
    Rect dst_rect;
    Rect src_rect;
    Vec4 border_color;
    Vec4 corner_radii;
    Vec4 colors[4];
    float edge_softness;
    float border_thickness;
    float _padding[2]; // std140 alignment
} VertInput;

SDL_GPUShader *load_shader(
    SDL_GPUDevice *gpu,
    char *filename,
    SDL_GPUShaderStage stage,
    int num_samplers,
    int num_storage_textures,
    int num_storage_buffers,
    int num_uniform_buffers
) {
    size_t len;
    unsigned char *data = read_file(filename, &len);
    SDL_GPUShaderCreateInfo info = {
        .code_size = len,
        .code = data,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers,
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
    SDL_GPUShader *vertex_shader = load_shader(gpu, "shaders/2d.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 1, 1);
    SDL_GPUShader *fragment_shader = load_shader(gpu, "shaders/2d.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 1);

    // Pipeline
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(
		gpu,
		&(SDL_GPUGraphicsPipelineCreateInfo){
			.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
				.num_color_targets = 1,
				.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
					.format = SDL_GetGPUSwapchainTextureFormat(gpu, window),
					.blend_state = {
						.enable_blend = true,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					}
				}}
			},
			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			.vertex_shader = vertex_shader,
			.fragment_shader = fragment_shader,
		}
	);
    ASSERT_CREATED(pipeline);

    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);

    // Textures
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(
        gpu,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .width = 1,
            .height = 1,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        }
    );

    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(
        gpu,
        &(SDL_GPUSamplerCreateInfo){
			.min_filter = SDL_GPU_FILTER_NEAREST,
			.mag_filter = SDL_GPU_FILTER_NEAREST,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
        }
    );

    SDL_GPUTransferBuffer *vertex_data_transfer_buffer = SDL_CreateGPUTransferBuffer(
        gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = 2 * sizeof(VertInput),
        }
    );

    SDL_GPUBuffer *vertex_data_buffer = SDL_CreateGPUBuffer(
        gpu,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = 2 * sizeof(VertInput),
        }
    );

    SDL_GPUTransferBuffer *texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
        gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = 1 * 1 * 4,
        }
    );

    Uint8 image_data[4] = {1, 1, 1, 1};
    Uint8 *texture_transfer_ptr = SDL_MapGPUTransferBuffer(
        gpu,
        texture_transfer_buffer,
        false
    );
    SDL_memcpy(texture_transfer_ptr, &image_data, 1 * 1 * 4);
    SDL_UnmapGPUTransferBuffer(gpu, texture_transfer_buffer);

    // Buffer data
    VertInput vertices[2] = {
        (VertInput){
            .dst_rect = (Rect){300.0f, 200.0f, 200.0f, 200.0f},
            .src_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f},
            .corner_radii = (Vec4){50.0f, 20.0f, 100.0f, 10.0f},
            .border_color = (Vec4){1.0, 1.0, 0.0, 1.0},
            .colors = {
                (Vec4){1.0f, 1.0f, 0.0f, 1.0f},
                (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
                (Vec4){1.0f, 0.0f, 0.0f, 1.0f},
                (Vec4){0.0f, 1.0f, 0.0f, 1.0f},
            },
            .edge_softness = 1.0f,
            .border_thickness = 20.0f,
        },
        (VertInput){
            .dst_rect = (Rect){10.0f, 10.0f, 200.0f, 200.0f},
            .src_rect = (Rect){0.0f, 0.0f, 1.0f, 1.0f},
            .corner_radii = {10.0f, 10.0f, 10.0f, 10.0f},
            .border_color = (Vec4){0.2, 0.2, 0.8, 1.0},
            .colors = {
                (Vec4){0.0f, 0.0f, 0.0f, 1.0f},
                (Vec4){0.0f, 0.0f, 0.0f, 1.0f},
                (Vec4){0.2f, 0.2f, 0.2f, 1.0f},
                (Vec4){0.2f, 0.2f, 0.2f, 1.0f},
            },
            .edge_softness = 1.0f,
            .border_thickness = 10.0f,
        },
    };

    unsigned int buf_size = 2 * sizeof(VertInput);

    // Upload texture
    SDL_GPUCommandBuffer *upload_cmd_buf = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);

    SDL_UploadToGPUTexture(
        copy_pass,
        &(SDL_GPUTextureTransferInfo) {
            .transfer_buffer = texture_transfer_buffer,
            .offset = 0,
        },
        &(SDL_GPUTextureRegion){
            .texture = texture,
            .w = 1,
            .h = 1,
            .d = 1,
        },
        false
    );

    // Main loop
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

            VertInput *data_ptr = SDL_MapGPUTransferBuffer(gpu, vertex_data_transfer_buffer, true);

            // modify data
            // data_ptr[i].dst_rect.x = ...
            for (int i = 0; i < 2; i++) {
                data_ptr[i] = vertices[i];
            }

            SDL_UnmapGPUTransferBuffer(gpu, vertex_data_transfer_buffer);

            SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmdbuf);
            SDL_UploadToGPUBuffer(
                copy_pass,
                &(SDL_GPUTransferBufferLocation) {
                    .transfer_buffer = vertex_data_transfer_buffer,
                    .offset = 0,
                },
                &(SDL_GPUBufferRegion) {
                    .buffer = vertex_data_buffer,
                    .offset = 0,
                    .size = 2 * sizeof(VertInput),
                },
                true
            );
            SDL_EndGPUCopyPass(copy_pass);


            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
                cmdbuf,
                &(SDL_GPUColorTargetInfo){
                    .texture = swapchain_texture,
                    .cycle = false,
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_STORE,
                    .clear_color = (SDL_FColor){0.0f, 0.5f, 0.0f, 1.0f},
                },
                1,
                NULL
            );

            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

            SDL_BindGPUFragmentSamplers(
                render_pass,
                0,
                &(SDL_GPUTextureSamplerBinding){
                    .texture = texture,
                    .sampler = sampler,
                },
                1
            );

            SDL_BindGPUVertexStorageBuffers(render_pass, 0, &vertex_data_buffer, 1);
            SDL_PushGPUVertexUniformData(cmdbuf, 0, &(Vec2){800.0f, 600.0f}, sizeof(Vec2));
            SDL_PushGPUFragmentUniformData(cmdbuf, 0, &(Vec2){800.0f, 600.0f}, sizeof(Vec2));

            SDL_DrawGPUPrimitives(render_pass, 2 * 6, 1, 0, 0);
            SDL_EndGPURenderPass(render_pass);

        }

        SDL_SubmitGPUCommandBuffer(cmdbuf);
    }

	SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
	SDL_ReleaseGPUSampler(gpu, sampler);
	SDL_ReleaseGPUTexture(gpu, texture);
	SDL_ReleaseGPUTransferBuffer(gpu, vertex_data_transfer_buffer);
	SDL_ReleaseGPUBuffer(gpu, vertex_data_buffer);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
