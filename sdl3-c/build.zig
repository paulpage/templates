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
}
