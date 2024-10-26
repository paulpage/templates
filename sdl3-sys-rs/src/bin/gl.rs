#[cfg(target_os = "emscripten")]
use emscripten_main_loop::{self, MainLoop, MainLoopEvent};

use core::ffi::c_int;
use std::ffi::{CStr, CString};
use std::ptr;
use std::mem;
use std::fs;
use std::path::PathBuf;
use gl::types::{GLenum, GLsizei, GLchar};
use std::ffi::c_void;

use sdl3_sys::everything::*;

struct App {
    use_wireframe: bool,
    use_small_viewport: bool,
    use_scissor_rect: bool,
    sdl: SdlContext,
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

pub extern "system" fn debug_callback(
    _source: GLenum,
    _type: GLenum,
    _id: GLenum,
    severity: GLenum,
    _length: GLsizei,
    message: *const GLchar,
    _user_param: *mut c_void,
) {
    let msg = unsafe {
        let c_str = CStr::from_ptr(message);
        match c_str.to_str() {
            Ok(s) => s.to_string(),
            Err(_) => {
                // If UTF-8 conversion fails, fall back to lossy conversion
                String::from_utf8_lossy(c_str.to_bytes()).into_owned()
            }
        }
    };
    if severity != gl::DEBUG_SEVERITY_NOTIFICATION {
        let severity_str = match severity {
            gl::DEBUG_SEVERITY_HIGH => "high",
            gl::DEBUG_SEVERITY_MEDIUM => "medium",
            gl::DEBUG_SEVERITY_LOW => "low",
            gl::DEBUG_SEVERITY_NOTIFICATION => "notification",
            _ => "???"
        };
        println!("DEBUG MESSAGE: [{severity_str}]{}", msg);
    }
}

struct SdlContext {
    window: *mut SDL_Window,
    glcontext: *mut SDL_GLContextState,
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

        let glcontext = SDL_GL_CreateContext(window);
        if glcontext.is_null() {
            print_err();
            panic!("SDL_GL_CreateContext failed");
        }

        gl::load_with(|s| {
            let cstr = CString::new(s).unwrap();
            SDL_GL_GetProcAddress(cstr.as_ptr()).unwrap() as *const _
        });

        gl::Enable(gl::DEBUG_OUTPUT);
        gl::DebugMessageCallback(Some(debug_callback), ptr::null());
        gl::Enable(gl::BLEND);

        SdlContext {
            window,
            glcontext,
        }
    }
}

impl App {
    pub fn new(w: i32, h: i32) -> Self {
        let sdl = init_sdl(w, h);

        Self {
            use_wireframe: false,
            use_small_viewport: false,
            use_scissor_rect: false,
            sdl,
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

        unsafe {
            gl::ClearColor(0.0, 1.0, 0.0, 1.0);
            let error = gl::GetError();
            if error != gl::NO_ERROR {
                println!("opengl error: {:?}", error as u32);
            }
            gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
            SDL_GL_SwapWindow(self.sdl.window);
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
