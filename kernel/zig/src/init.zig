extern fn log_write(s: [*:0]const u8) void;

pub export fn zig_kernel_init() callconv(.c) void {
    log_write("[ZIG] System components initialized\n");
}
