#[cfg(target_os = "emscripten")]
use emscripten_main_loop::{self, MainLoop, MainLoopEvent};

use core::ffi::c_int;
use std::ffi::CStr;
use std::ptr;
use std::mem;
use std::fs;
use std::path::PathBuf;
use gl::types::GLuint;

use sdl3_sys::everything::*;

struct App {
    use_wireframe: bool,
    use_small_viewport: bool,
    use_scissor_rect: bool,
    sdl: SdlContext,
    line_pipeline: *mut SDL_GPUGraphicsPipeline,
    fill_pipeline: *mut SDL_GPUGraphicsPipeline,
}

fn print_err()  {
    let err_ptr = unsafe { SDL_GetError() };
    if err_ptr.is_null() {
        println!("No SDL error available");
    } else {
        let message = unsafe {
            CStr::from_ptr(err_ptr).to_str().unwrap_or("Error contains invalid UTF-8")
        };
        println!("SDL Error: {}", message);
    }
}


struct SdlContext {
    window: *mut SDL_Window,
    gpu: *mut SDL_GPUDevice,
}

fn init_sdl(w: i32, h: i32) -> SdlContext {
    unsafe {
        if !SDL_Init(SDL_INIT_VIDEO) {
            print_err();
            panic!("SDL_Init failed");
        }

        let window = SDL_CreateWindow(c"My Window".as_ptr(), w as c_int, h as c_int, SDL_WINDOW_OPENGL);
        if window.is_null() {
            print_err();
            panic!("SDL_CreateWindow failed");
        }

        let supports_spirv = SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV, ptr::null());
        println!("suports_spirv: {}", supports_spirv);

        let gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, ptr::null());
        if gpu.is_null() {
            print_err();
            panic!("SDL_CreateGPUDevice failed");
        }

        if !SDL_ClaimWindowForGPUDevice(gpu, window) {
            print_err();
            panic!("SDL_ClaimWindowForGPUDevice failed");
        }

        let s: GLuint = 0;
        unsafe {
            gl::load_with(|s| {
                let cstr = CStr::from_bytes_with_nul_unchecked(s.as_bytes());
                SDL_GL_GetProcAddress(cstr.as_ptr()).unwrap() as *const _
            });
        }

        SdlContext {
            window,
            gpu,
        }
    }
}

fn load_shader(device: *mut SDL_GPUDevice, filename: &str, sampler_count: u32, uniform_buffer_count: u32, storage_buffer_count: u32, storage_texture_count: u32) -> *mut SDL_GPUShader {
    let mut path = PathBuf::from(filename);
    let stage = match path.extension().and_then(|ext| ext.to_str()) {
        Some("vert") => SDL_GPU_SHADERSTAGE_VERTEX,
        Some("frag") => SDL_GPU_SHADERSTAGE_FRAGMENT,
        _ => panic!("Invalid shader stage"),
    };
    path.set_extension("spv");
    let code = fs::read(&path)
        .expect(&format!("Failed to load file: {}", path.clone().display()));

    let shader_info = SDL_GPUShaderCreateInfo {
        code: code.as_ptr(),
        code_size: code.len(),
        entrypoint: c"main".as_ptr(),
        format: SDL_GPU_SHADERFORMAT_SPIRV,
        stage: stage,
        num_samplers: sampler_count,
        num_uniform_buffers: uniform_buffer_count,
        num_storage_buffers: storage_buffer_count,
        num_storage_textures: storage_texture_count,
        props: 0,
    };

    let shader = unsafe {
        SDL_CreateGPUShader(device, &shader_info)
    };
    if shader.is_null() {
        print_err();
        panic!("Failed to create shader");
    }

    shader
}

impl App {
    pub fn new(w: i32, h: i32) -> Self {
        let sdl = init_sdl(w, h);

        let vertex_shader = load_shader(sdl.gpu, "src/triangle.vert", 0, 0, 0, 0);
        let fragment_shader = load_shader(sdl.gpu, "src/solid_color.frag", 0, 0, 0, 0);

        let mut pipeline_info: SDL_GPUGraphicsPipelineCreateInfo = unsafe { mem::zeroed() };
        pipeline_info.vertex_shader = vertex_shader;
        pipeline_info.fragment_shader = fragment_shader;
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipeline_info.target_info.num_color_targets = 1;
        let mut color_target_description: SDL_GPUColorTargetDescription = unsafe { mem::zeroed() };
        color_target_description.format = unsafe { SDL_GetGPUSwapchainTextureFormat(sdl.gpu, sdl.window) };
        pipeline_info.target_info.color_target_descriptions = [color_target_description].as_ptr();

        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        let fill_pipeline = unsafe { SDL_CreateGPUGraphicsPipeline(sdl.gpu, &pipeline_info) };
        if fill_pipeline.is_null() {
            print_err();
            panic!("Failed to create fill graphics pipeline");
        }

        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
        let line_pipeline = unsafe { SDL_CreateGPUGraphicsPipeline(sdl.gpu, &pipeline_info) };
        if line_pipeline.is_null() {
            print_err();
            panic!("Failed to create line graphics pipeline");
        }

        unsafe {
            SDL_ReleaseGPUShader(sdl.gpu, vertex_shader);
            SDL_ReleaseGPUShader(sdl.gpu, fragment_shader);
        }

        println!("Press left to toggle wireframe mode");
        println!("Press down to toggle small viewport");
        println!("Press right to toggle scissor rect");

        Self {
            use_wireframe: false,
            use_small_viewport: false,
            use_scissor_rect: false,
            sdl,
            line_pipeline,
            fill_pipeline,
        }
    }

    pub fn update(&mut self) -> bool {
        unsafe {
            let mut event: SDL_Event = mem::zeroed();
            while SDL_PollEvent(&mut event) {
                match SDL_EventType(event.r#type) {
                    SDL_EVENT_QUIT => return false,
                    SDL_EVENT_KEY_DOWN => {
                        match event.key.key {
                            SDLK_LEFT => self.use_wireframe = !self.use_wireframe,
                            SDLK_RIGHT => self.use_small_viewport = !self.use_small_viewport,
                            SDLK_DOWN => self.use_scissor_rect = !self.use_scissor_rect,
                            _ => (),
                        }
                    }
                    _ => ()
                }
            }
        }

        let cmdbuf = unsafe { SDL_AcquireGPUCommandBuffer(self.sdl.gpu) };
        if cmdbuf.is_null() {
            print_err();
            panic!("failed to acquire GPU command buffer");
        }

        let mut swapchain_texture = unsafe { mem::zeroed() };
        if !unsafe { SDL_AcquireGPUSwapchainTexture(cmdbuf, self.sdl.window, &mut swapchain_texture, ptr::null_mut(), ptr::null_mut()) } {
            print_err();
            panic!("failed to acquire swapchain texture");
        }

        if !swapchain_texture.is_null() {
            let mut color_target_info: SDL_GPUColorTargetInfo = unsafe { mem::zeroed() };
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = SDL_FColor {r: 0.0, g: 0.5, b: 0.0, a: 1.0};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            let render_pass = unsafe { SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, ptr::null()) };
            unsafe { SDL_BindGPUGraphicsPipeline(render_pass, if self.use_wireframe { self.line_pipeline } else { self.fill_pipeline }) };
            if self.use_small_viewport {
                unsafe { SDL_SetGPUViewport(render_pass, &SDL_GPUViewport {
                    x: 160.0,
                    y: 120.0,
                    w: 320.0,
                    h: 240.0,
                    min_depth: 0.1,
                    max_depth: 1.0,
                }) };
            }
            if self.use_scissor_rect {
                unsafe { SDL_SetGPUScissor(render_pass, &SDL_Rect {
                    x: 320,
                    y: 240,
                    w: 320,
                    h: 240,
                }) };
            }
            unsafe {
                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
                SDL_EndGPURenderPass(render_pass);
            }

        }

        unsafe { SDL_SubmitGPUCommandBuffer(cmdbuf) };

        true
    }

    pub fn quit(&mut self) {
        unsafe {
            SDL_Quit()
        }
    }
}

#[cfg(target_os = "emscripten")]
impl MainLoop for App {

    fn main_loop(&mut self) -> MainLoopEvent {
        match self.update() {
            true => MainLoopEvent::Continue,
            false => {
                self.quit();
                MainLoopEvent::Terminate
            }
        }
    }
}

fn main() {
    let mut app = App::new(800, 600);

    #[cfg(target_os = "emscripten")]
    emscripten_main_loop::run(app);

    #[cfg(not(target_os = "emscripten"))]
    {
        while app.update() {
        }
        app.quit();
    }
}
