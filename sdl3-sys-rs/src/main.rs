#[cfg(target_os = "emscripten")]
use emscripten_main_loop::{self, MainLoop, MainLoopEvent};

use sdl3_sys::{
    gpu::{SDL_CreateGPUDevice, SDL_GPU_SHADERFORMAT_SPIRV},
    error::SDL_GetError,
    init::{SDL_Init, SDL_Quit, SDL_INIT_VIDEO},
    events::{SDL_Event, SDL_PollEvent, SDL_EVENT_QUIT},
};

struct App {

}

impl App {
    pub fn new() -> Self {

        unsafe {
            if SDL_Init(SDL_INIT_VIDEO) {
                println!("SDL init");
            }

            let gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, c"gpu".as_ptr());
        }

        Self {
        }
    }

    pub fn update(&mut self) -> bool {
        unsafe {
            let mut event: SDL_Event = std::mem::zeroed();
            while SDL_PollEvent(&mut event) {
                if event.r#type == SDL_EVENT_QUIT.into() {
                    return false;
                }
            }
        }
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
    let mut app = App::new();

    #[cfg(target_os = "emscripten")]
    emscripten_main_loop::run(app);

    #[cfg(not(target_os = "emscripten"))]
    while app.update() {
    }
    app.quit();
}
