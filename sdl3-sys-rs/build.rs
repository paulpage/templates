use std::process::Command;
use std::path::Path;

const YELLOW: &str = "\x1b[33m";
const RESET: &str = "\x1b[0m";

fn main() {
    println!("cargo:rerun-if-changed=src/triangle.vert");
    println!("cargo:rerun-if-changed=src/solid_color.frag");

    compile_shader("src/triangle.vert", "src/triangle.spv");
    compile_shader("src/solid_color.frag", "src/solid_color.spv");
}

fn compile_shader(input: &str, output: &str) {
    let input_path = Path::new(input);
    let output_path = Path::new(output);

    if !input_path.exists() {
        println!("cargo:warning=Shader file not found: {}", input);
        return;
    }

    if output_path.exists() && output_path.metadata().unwrap().modified().unwrap() > input_path.metadata().unwrap().modified().unwrap() {
        println!("Shader up to date: {}", output);
        return;
    }

    println!("Compiling shader: {} -> {}", input, output);

    let output = Command::new("glslangValidator")
        .arg("-V")
        .arg(input)
        .arg("-o")
        .arg(output)
        .output()
        .expect("Failed to execute glslangValidator");

    println!("--- glslangValidator stdout:\n{}{}{}", YELLOW, String::from_utf8_lossy(&output.stdout), RESET);
    println!("--- glslangValidator stderr:\n{}{}{}", YELLOW, String::from_utf8_lossy(&output.stderr), RESET);

    if !output.status.success() {
        panic!("Failed to compile shader: {}\n{}", 
               input, 
               String::from_utf8_lossy(&output.stderr));
    }
}
