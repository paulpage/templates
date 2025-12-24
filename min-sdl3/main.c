#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

size_t file_length(FILE *f) {
	long len, pos;
	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, pos, SEEK_SET);
	return (size_t) len;
}

unsigned char *read_file(const char *filename, size_t *plen) {
    FILE *f = fopen(filename, "rb");
    size_t len = file_length(f);
    unsigned char *buffer = (unsigned char*)malloc(len);
    len = fread(buffer, 1, len, f);
    if (plen) *plen = len;
    fclose(f);
    return buffer;
}

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
    Rect rect;
    Vec4 color;
} VertInput;

typedef struct AppState {
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUTransferBuffer *vertex_data_transfer_buffer;
    SDL_GPUBuffer *vertex_data_buffer;
} AppState;
AppState state = {0};

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
        /* .format = SDL_GPU_SHADERFORMAT_SPIRV, */
        .format = SDL_GPU_SHADERFORMAT_DXIL,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers,
    };

    SDL_GPUShader *shader = SDL_CreateGPUShader(state.gpu, &info);
    ASSERT_CREATED(shader);
    return shader;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(state.gpu);
    ASSERT_CREATED(cmdbuf);

    SDL_GPUTexture *swapchain_texture = NULL;
    ASSERT_CALL(SDL_AcquireGPUSwapchainTexture(cmdbuf, state.window, &swapchain_texture, NULL, NULL));

    if (swapchain_texture) {
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

        SDL_BindGPUGraphicsPipeline(render_pass, state.pipeline);

        SDL_BindGPUVertexStorageBuffers(render_pass, 0, &state.vertex_data_buffer, 1);
        SDL_PushGPUVertexUniformData(cmdbuf, 0, &(Vec2){800.0f, 600.0f}, sizeof(Vec2));
        SDL_PushGPUFragmentUniformData(cmdbuf, 0, &(Vec2){800.0f, 600.0f}, sizeof(Vec2));

        SDL_DrawGPUPrimitives(render_pass, 100 * 6, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);
    return 0;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WINDOW_EXPOSED:
            printf("EXPOSED\n");
            break;
    }
    return 0;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	SDL_ReleaseGPUGraphicsPipeline(state.gpu, state.pipeline);
	SDL_ReleaseGPUTransferBuffer(state.gpu, state.vertex_data_transfer_buffer);
	SDL_ReleaseGPUBuffer(state.gpu, state.vertex_data_buffer);
    SDL_DestroyGPUDevice(state.gpu);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
/* int main(int argc, char *argv[]) { */

    // Init
    ASSERT_CALL(SDL_Init(SDL_INIT_VIDEO));
    state.window = SDL_CreateWindow("Example", 800, 600, SDL_WINDOW_RESIZABLE);
    ASSERT_CREATED(state.window);
    /* SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL); */
    state.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, NULL);
    ASSERT_CREATED(state.gpu);
    ASSERT_CALL(SDL_ClaimWindowForGPUDevice(state.gpu, state.window));

    // Shaders
    SDL_GPUShader *vertex_shader = load_shader(state.gpu, "shader.vert.dxil", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 1, 1);
    SDL_GPUShader *fragment_shader = load_shader(state.gpu, "shader.frag.dxil", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 1);

    // Pipeline
    state.pipeline = SDL_CreateGPUGraphicsPipeline(
		state.gpu,
		&(SDL_GPUGraphicsPipelineCreateInfo){
			.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
				.num_color_targets = 1,
				.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
					.format = SDL_GetGPUSwapchainTextureFormat(state.gpu, state.window),
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
    ASSERT_CREATED(state.pipeline);

    SDL_ReleaseGPUShader(state.gpu, vertex_shader);
    SDL_ReleaseGPUShader(state.gpu, fragment_shader);

    // Buffer data
    VertInput store[100] = {0};
    for (int i = 0; i < 100; i++) {
        int x = (float)(i / 10);
        int y = (float)(i % 10);
        VertInput vert = {
            .rect = (Rect){x * 80.0f + 10.0f, y * 80.0f + 10.0f, 60.0f, 60.0f},
            .color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
        };
        store[i] = vert;
    }

    SDL_GPUTransferBuffer *vertex_data_transfer_buffer = SDL_CreateGPUTransferBuffer(
        state.gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = 100 * sizeof(VertInput),
        }
    );

    state.vertex_data_buffer = SDL_CreateGPUBuffer(
        state.gpu,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = 100 * sizeof(VertInput),
        }
    );

    VertInput *data_ptr = SDL_MapGPUTransferBuffer(state.gpu, vertex_data_transfer_buffer, true);

    for (int i = 0; i < 100; i++) {
        data_ptr[i] = store[i];
    }

    SDL_UnmapGPUTransferBuffer(state.gpu, vertex_data_transfer_buffer);

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(state.gpu);
    ASSERT_CREATED(cmdbuf);

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmdbuf);
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = vertex_data_transfer_buffer,
            .offset = 0,
        },
        &(SDL_GPUBufferRegion) {
            .buffer = state.vertex_data_buffer,
            .offset = 0,
            .size = 100 * sizeof(VertInput),
        },
        true
    );
    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(cmdbuf);

    return 0;
}
