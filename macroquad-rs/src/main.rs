mod engine;
use engine::{self as g, Key};

#[cfg(not(target_arch = "wasm32"))]
fn print_message(message: &str) {
    println!("{}", message);
}

#[cfg(target_arch = "wasm32")]
#[no_mangle]
fn print_message(message: &str) {
    unsafe {
        print_message_js()
    }
}

// ============================================================

#[no_mangle]
pub extern "C" fn alloc(size: usize) -> *mut u8 {
    let mut buf = Vec::with_capacity(size);
    let ptr = buf.as_mut_ptr();
    std::mem::forget(buf);
    ptr
}

static mut FILE_CONTENT: Option<Vec<u8>> = None;

#[no_mangle]
pub extern "C" fn on_file_loaded(ptr: *const u8, len: usize) {
    let slice = unsafe { std::slice::from_raw_parts(ptr, len) };
    let content = slice.to_vec();
    unsafe {
        FILE_CONTENT = Some(content);
    }
}

fn get_file_content() -> Option<Vec<u8>> {
    unsafe {
        FILE_CONTENT.clone()
    }
}

// ============================================================

fn save_file(content: &str) {
    unsafe {
        save_file_dialog(content.as_ptr(), content.len());
    }
}

// ============================================================

#[cfg(target_arch = "wasm32")]
extern "C" {
    fn print_message_js();
    fn open_file_dialog();
    fn save_file_dialog(ptr: *const u8, len: usize);
}

// ============================================================

#[macroquad::main("hello")]
async fn main() {

    //print_message("Hello workdl!");

    let mut pos_x = 0.0;
    let mut pos_y = 0.0;

    let mut lines = Vec::new();
    lines.push(String::from("Hello"));
    let mut line_idx = 0;

    let mut scroll_string = String::new();

    //let sound = macroquad::audio::load_sound("sample.ogg").await.unwrap();
    //macroquad::audio::play_sound(
    //    &sound,
    //    macroquad::audio::PlaySoundParams {
    //        looped: true,
    //        volume: 1.0,
    //    }
    //);

    let mut s = String::from("[text will go here]");
    let mut rotation = 0.0;

    loop {

        if g::is_key_pressed(Key::A) {
            print_message("key was pressed!!!!");
        }

        if g::is_key_pressed(Key::O) {
            unsafe {
                open_file_dialog();
            }
        }
        if g::is_key_pressed(Key::S) {
            s = String::from("Hello from wasm");
            save_file(&s);
        }

        if let Some(content) = get_file_content() {
            s = String::from_utf8(content).expect("invalid utf-8");
            //s = format!("{:?}", content);
        }

        g::clear_background(g::RED);

        g::draw_text(&s, 20.0, 100.0, 30.0, g::DARKGRAY);

        g::draw_line(40.0, 40.0, 100.0, 200.0, 15.0, g::BLUE);
        g::draw_rectangle(g::screen_width() / 2.0 - 60.0, 100.0, 120.0, 60.0, g::GREEN);
        g::draw_circle(g::screen_width() - 30.0, g::screen_height() - 30.0, 15.0, g::YELLOW);

        let (wheel_x, wheel_y) = g::mouse_wheel();
        pos_y += wheel_y * 30.0;
        pos_x += wheel_x * 30.0;

        if wheel_y != 0.0 {
            scroll_string = format!("scroll: {}", wheel_y);
        }

        if let Some(s) = g::get_text() {
            lines[line_idx].push_str(&s);
        }
        if g::is_key_pressed(Key::Enter) {
            lines.push(String::new());
            line_idx += 1;
        }

        g::draw_text(&scroll_string, 20.0, 20.0, 30.0, g::DARKGRAY);
        for (i, line) in lines.iter().enumerate() {
            g::draw_text(line.as_str(), 20.0 + pos_x, 20.0 + pos_y + 35.0 * (i + 1) as f32, 30.0, g::DARKGRAY);
        }

        g::next_frame().await
    }
}
