#![no_std]
#![no_main]

use core::panic::PanicInfo;

extern "C" {
    fn log_write(s: *const u8);
}

#[no_mangle]
pub extern "C" fn rust_kernel_init() {
    let msg = b"[RUST] Safe kernel components initialized\n\0";
    unsafe { log_write(msg.as_ptr()); }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        unsafe { core::arch::asm!("hlt"); }
    }
}
