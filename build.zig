const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .freestanding,
        .abi = .none,
    });
    const optimize = b.standardOptimizeOption(.{});

    const obj = b.addObject(.{
        .name = "qnu_zig",
        .root_module = b.createModule(.{
            .root_source_file = b.path("kernel/zig/src/init.zig"),
            .target = target,
            .optimize = optimize,
            .code_model = .kernel,
        }),
    });

    const install = b.addInstallArtifact(obj, .{});
    b.getInstallStep().dependOn(&install.step);
}
