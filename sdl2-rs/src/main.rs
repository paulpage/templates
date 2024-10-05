use sdl2::pixels::Color;
use sdl2::event::{Event, EventPollIterator};
use sdl2::keyboard::Keycode;
use sdl2::render::Canvas;
use sdl2::video::Window;
use sdl2::EventPump;
use std::time::Duration;

pub struct App {
    canvas: Canvas<Window>,
    event_pump: EventPump,
}

impl App {
    pub fn new() -> Self {
        let sdl_context = sdl2::init().unwrap();
        let video_subsystem = sdl_context.video().unwrap();
        let window = video_subsystem.window("Application", 800, 600)
            .position_centered()
            .resizable()
            .build()
            .unwrap();

        let canvas = window.into_canvas().build().unwrap();

        let event_pump = sdl_context.event_pump().unwrap();

        Self {
            canvas,
            event_pump,
        }

    }

    pub fn events(&mut self) -> EventPollIterator {
        self.event_pump.poll_iter()
    }

    pub fn clear(&mut self, color: Color) {
        self.canvas.set_draw_color(color);
        self.canvas.clear();
    }

    pub fn present(&mut self) {
        self.canvas.present();
    }
}

fn main() {
    let mut app = App::new();

    let mut i = 0;
    'running: loop {
        i = (i + 1) % 255;
        app.clear(Color::RGB(i, 64, 255 - i));
        for event in app.events() {
            match event {
                Event::Quit {..} |
                Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    break 'running
                },
                _ => {}
            }
        }

        app.present();
        ::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
    }
}
