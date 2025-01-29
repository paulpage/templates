const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "main",
        .target = target,
        .optimize = optimize,
    });

    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
        //.preferred_link_mode = .static, // or .dynamic
    });
    const sdl_lib = sdl_dep.artifact("SDL3");
    // exe.linkSystemLibrary("sdl3");

    exe.addCSourceFiles(.{ .files = &.{"main.c"} });
    exe.linkLibC();
    exe.linkLibrary(sdl_lib);

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // ------

    const gl_exe = b.addExecutable(.{
        .name = "gl",
        .target = target,
        .optimize = optimize,
    });
    gl_exe.addCSourceFiles(.{ .files = &.{"gl.c"} });
    gl_exe.linkLibC();
    gl_exe.linkLibrary(sdl_lib);
    gl_exe.linkSystemLibrary("GL");
    gl_exe.linkSystemLibrary("dl");
    b.installArtifact(gl_exe);

    const run_gl_cmd = b.addRunArtifact(gl_exe);
    run_gl_cmd.step.dependOn(b.getInstallStep());

    const run_gl_step = b.step("rungl", "Run gl");
    run_gl_step.dependOn(&run_gl_cmd.step);
}
