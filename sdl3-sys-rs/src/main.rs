#[cfg(target_os = "emscripten")]
use emscripten_main_loop::{self, MainLoop, MainLoopEvent};

use core::ffi::c_int;
use std::ffi::CStr;
use std::ptr;
use std::mem;
use std::fs;
use std::path::PathBuf;

use sdl3_sys::everything::*;

struct App {
    sdl: SdlContext,
    pipeline: *mut SDL_GPUGraphicsPipeline,
    buf_vertex: *mut SDL_GPUBuffer,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct Vec2 {
    x: f32,
    y: f32,
}

impl Vec2 {
    pub fn new(x: f32, y: f32) -> Self {
        Self {
            x,
            y,
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct Color {
    r: f32,
    g: f32,
    b: f32,
    a: f32,
}

impl Color {
    pub fn new(r: f32, g: f32, b: f32, a: f32) -> Self {
        Self {
            r,
            g,
            b,
            a,
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct Rect {
    x: f32,
    y: f32,
    w: f32,
    h: f32,
}

impl Rect {
    pub fn new(x: f32, y: f32, w: f32, h: f32) -> Self {
        Self {
            x,
            y,
            w,
            h,
        }
    }
}

#[repr(C)]
struct VertInput {
    dst_rect: Rect,
    src_rect: Rect,
    colors: [Color; 4],
    corner_radius: f32,
    edge_softness: f32,
    border_thickness: f32,
}

#[repr(C)]
struct GpuVertex {
    pos: Vec2,
    color: Color,
    rect_min: Vec2,
    rect_max: Vec2,
}

impl GpuVertex {
    pub fn new(pos: Vec2, color: Color, rect_min: Vec2, rect_max: Vec2) -> Self {
        Self {
            pos,
            color,
            rect_min,
            rect_max,
        }
    }
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

macro_rules! assert_created(
    ($obj:expr, $name:expr) => (
        if $obj.is_null() {
            print_err();
            panic!("{} failed", $name);
        }
    )
);

macro_rules! assert_true(
    ($cond:expr) => (
        if !$cond {
            print_err();
            panic!("Assert [{}] failed", stringify!($cond));
        }
    )
);

struct SdlContext {
    window: *mut SDL_Window,
    gpu: *mut SDL_GPUDevice,
}

fn init_sdl(w: i32, h: i32) -> SdlContext {
    unsafe {
        assert_true!(SDL_Init(SDL_INIT_VIDEO));

        let window = SDL_CreateWindow(c"My Window".as_ptr(), w as c_int, h as c_int, 0);
        assert_created!(window, "SDL_CreateWindow");

        let gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, ptr::null());
        assert_created!(gpu, "SDL_CreateGPUDevice");

        assert_true!(SDL_ClaimWindowForGPUDevice(gpu, window));

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
        .unwrap_or_else(|_| panic!("Failed to load file: {}", path.clone().display()));

    let shader_info = SDL_GPUShaderCreateInfo {
        code: code.as_ptr(),
        code_size: code.len(),
        entrypoint: c"main".as_ptr(),
        format: SDL_GPU_SHADERFORMAT_SPIRV,
        stage,
        num_samplers: sampler_count,
        num_uniform_buffers: uniform_buffer_count,
        num_storage_buffers: storage_buffer_count,
        num_storage_textures: storage_texture_count,
        props: 0,
    };

    let shader = unsafe {
        SDL_CreateGPUShader(device, &shader_info)
    };
    assert_created!(shader, "SDL_CreateGPUShader");

    shader
}

impl App {
    pub fn new(w: i32, h: i32) -> Self {
        let sdl = init_sdl(w, h);

        let vertex_shader = load_shader(sdl.gpu, "src/triangle.vert", 0, 1, 0, 0);
        let fragment_shader = load_shader(sdl.gpu, "src/solid_color.frag", 0, 1, 0, 0);

        //let vertices: [VertInput; 1] = [
        //    VertInput {
        //        dst_rect: Rect::new(10.0, 20.0, 100.0, 200.0),
        //        src_rect: Rect::new(0.0, 0.0, 1.0, 1.0),
        //        colors: [
        //            Color::new(1.0, 1.0, 0.0, 1.0),
        //            Color::new(1.0, 1.0, 0.0, 1.0),
        //            Color::new(1.0, 1.0, 0.0, 1.0),
        //            Color::new(1.0, 1.0, 0.0, 1.0),
        //        ],
        //        corner_radius: 5.0,
        //        edge_softness: 1.0,
        //        border_thickness: 2.0,
        //    },
        //];
        //
        //let buf_size = (vertices.len() * mem::size_of::<VertInput>()) as u32;

        let white = Color::new(255.0, 255.0, 0.0, 255.0);
        let rect_min = Vec2::new(20.0, 20.0);
        let rect_max = Vec2::new(300.0, 300.0);
        let vertices: [GpuVertex; 6] = [
            //GpuVertex::new(Vec2::new(20.0, 20.0), white, rect_min, rect_max),
            //GpuVertex::new(Vec2::new(20.0, 300.0), white, rect_min, rect_max),
            //GpuVertex::new(Vec2::new(300.0, 300.0), white, rect_min, rect_max),
            //GpuVertex::new(Vec2::new(20.0, 20.0), white, rect_min, rect_max),
            //GpuVertex::new(Vec2::new(300.0, 300.0), white, rect_min, rect_max),
            //GpuVertex::new(Vec2::new(300.0, 20.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(0.0, 0.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(0.0, 600.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(800.0, 600.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(0.0, 0.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(800.0, 600.0), white, rect_min, rect_max),
            GpuVertex::new(Vec2::new(800.0, 0.0), white, rect_min, rect_max),
        ];
        let buf_size = (vertices.len() * mem::size_of::<GpuVertex>()) as u32;
        //let vertices: [f32; 36] = [
        //    -0.5, -0.5, 1.0, 1.0, 1.0, 1.0,
        //    -0.5, 0.5, 1.0, 1.0, 1.0, 1.0,
        //    0.5, 0.5, 1.0, 1.0, 1.0, 1.0,
        //    -0.5, -0.5, 1.0, 1.0, 1.0, 1.0,
        //    0.5, 0.5, 1.0, 1.0, 1.0, 1.0,
        //    0.5, -0.5, 1.0, 1.0, 1.0, 1.0,
        //];
        //let buf_size = (vertices.len() * mem::size_of::<f32>()) as u32;
        println!("buf_size: {}", buf_size);

        // Buffers
        let mut buffer_info: SDL_GPUBufferCreateInfo = unsafe { mem::zeroed() };
        buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_info.size = buf_size;
        buffer_info.props = 0;
        let buf_vertex = unsafe { SDL_CreateGPUBuffer(sdl.gpu, &buffer_info) };
        assert_created!(buf_vertex, "SDL_CreateGPUBuffer");

        let mut transfer_buffer_info: SDL_GPUTransferBufferCreateInfo = unsafe { mem::zeroed() };
        transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_info.size = buf_size;
        transfer_buffer_info.props = 0;
        let buf_transfer = unsafe { SDL_CreateGPUTransferBuffer(sdl.gpu, &transfer_buffer_info) };
        assert_created!(buf_transfer, "SDL_CreateGPUTransferBuffer");

        unsafe {
            let map = SDL_MapGPUTransferBuffer(sdl.gpu, buf_transfer, false);
            std::ptr::copy_nonoverlapping(
                vertices.as_ptr() as *mut u8,
                map as *mut u8,
                buf_size as usize,
            );
            SDL_UnmapGPUTransferBuffer(sdl.gpu, buf_transfer);
        }

        let cmd = unsafe { SDL_AcquireGPUCommandBuffer(sdl.gpu) };
        let copy_pass = unsafe { SDL_BeginGPUCopyPass(cmd) };
        let mut buf_location: SDL_GPUTransferBufferLocation = unsafe { mem::zeroed() };
        buf_location.transfer_buffer = buf_transfer;
        buf_location.offset = 0;
        let mut dst_region: SDL_GPUBufferRegion = unsafe { mem::zeroed() };
        dst_region.buffer = buf_vertex;
        dst_region.offset = 0;
        dst_region.size = buf_size;
        unsafe {
            SDL_UploadToGPUBuffer(copy_pass, &buf_location, &dst_region, false);
            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(cmd);
        }

        unsafe { SDL_ReleaseGPUTransferBuffer(sdl.gpu, buf_transfer) };

        // Pipeline
        let mut pipeline_info: SDL_GPUGraphicsPipelineCreateInfo = unsafe { mem::zeroed() };
        let mut vertex_buffer_info: SDL_GPUVertexBufferDescription = unsafe { mem::zeroed() };
        let mut vertex_attributes: [SDL_GPUVertexAttribute; 4] = [
            unsafe { mem::zeroed() },
            unsafe { mem::zeroed() },
            unsafe { mem::zeroed() },
            unsafe { mem::zeroed() },
        ];
        let mut blend_state: SDL_GPUColorTargetBlendState = unsafe { mem::zeroed() };
        blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blend_state.enable_blend = true;
        let mut color_target_info: SDL_GPUColorTargetDescription = unsafe { mem::zeroed() };
        color_target_info.format = unsafe { SDL_GetGPUSwapchainTextureFormat(sdl.gpu, sdl.window) };

        color_target_info.blend_state = blend_state;

        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.color_target_descriptions = &color_target_info;

        pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipeline_info.vertex_shader = vertex_shader;
        pipeline_info.fragment_shader = fragment_shader;

        vertex_buffer_info.slot = 0;
        vertex_buffer_info.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertex_buffer_info.instance_step_rate = 0;
        vertex_buffer_info.pitch = mem::size_of::<f32>() as u32 * 10;

        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[0].location = 0;
        vertex_attributes[0].offset = 0;

        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        vertex_attributes[1].location = 1;
        vertex_attributes[1].offset = mem::size_of::<f32>() as u32 * 2;

        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].location = 2;
        vertex_attributes[2].offset = mem::size_of::<f32>() as u32 * 6;

        vertex_attributes[3].buffer_slot = 0;
        vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[3].location = 3;
        vertex_attributes[3].offset = mem::size_of::<f32>() as u32 * 8;

        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_info;
        pipeline_info.vertex_input_state.num_vertex_attributes = 4;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes.as_ptr();

        pipeline_info.props = 0;

        let pipeline = unsafe { SDL_CreateGPUGraphicsPipeline(sdl.gpu, &pipeline_info) };


        unsafe {
            SDL_ReleaseGPUShader(sdl.gpu, vertex_shader);
            SDL_ReleaseGPUShader(sdl.gpu, fragment_shader);
        }

        Self {
            sdl,
            pipeline,
            buf_vertex,
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
                            SDLK_LEFT => { println!("left") },
                            SDLK_RIGHT => { println!("right") },
                            _ => (),
                        }
                    }
                    _ => ()
                }
            }
        }

        let cmdbuf = unsafe { SDL_AcquireGPUCommandBuffer(self.sdl.gpu) };
        assert_created!(cmdbuf, "SDL_AcquireGPUCommandBuffer");

        let mut swapchain_texture = unsafe { mem::zeroed() };
        let acquire_swapchain_result = unsafe { SDL_AcquireGPUSwapchainTexture(cmdbuf, self.sdl.window, &mut swapchain_texture, ptr::null_mut(), ptr::null_mut()) };
        assert_true!(acquire_swapchain_result);

        if !swapchain_texture.is_null() {
            let mut color_target_info: SDL_GPUColorTargetInfo = unsafe { mem::zeroed() };
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = SDL_FColor {r: 0.0, g: 0.5, b: 0.0, a: 1.0};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            let mut vertex_binding: SDL_GPUBufferBinding = unsafe { mem::zeroed() };

            unsafe {

                vertex_binding.buffer = self.buf_vertex;
                vertex_binding.offset = 0;

                SDL_PushGPUVertexUniformData(cmdbuf, 0, (&Vec2::new(800.0, 600.0)) as *const Vec2 as *const _, mem::size_of::<Vec2>() as u32);
                SDL_PushGPUFragmentUniformData(cmdbuf, 0, (&Vec2::new(800.0, 600.0)) as *const Vec2 as *const _, mem::size_of::<Vec2>() as u32);

                let render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, ptr::null());
                SDL_BindGPUGraphicsPipeline(render_pass, self.pipeline);
                SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

                SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
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
