pub use macroquad::prelude::*;
pub use macroquad::input::KeyCode as Key;

#[cfg(target_os = "linux")]
pub fn mouse_wheel() -> (f32, f32) {
    let (x, y) = macroquad::prelude::mouse_wheel();
    (x, y)
}

#[cfg(not(target_os = "linux"))]
pub fn mouse_wheel() -> (f32, f32) {
    let (x, y) = macroquad::prelude::mouse_wheel();
    (x / 120.0, y / 120.0)
}

pub fn get_text() -> Option<String> {
    let mut text = None;
    while let Some(c) = get_char_pressed() {
        if ((c as u32) < 57344 || (c as u32) > 63743) && !ctrl_down() && !alt_down() && !super_down() {
            text.get_or_insert(String::new()).push_str(&c.to_string());
        }
    }
    text
}

pub fn ctrl_down() -> bool {
    is_key_down(Key::LeftControl) || is_key_down(Key::RightControl)
}

pub fn alt_down() -> bool {
    is_key_down(Key::LeftAlt) || is_key_down(Key::RightAlt)
}

pub fn shift_down() -> bool {
    is_key_down(Key::LeftShift) || is_key_down(Key::RightShift)
}

pub fn super_down() -> bool {
    is_key_down(Key::LeftSuper) || is_key_down(Key::RightSuper)
}

