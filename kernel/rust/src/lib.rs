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

/* ---- memory/string primitives (replace kernel/core/string.c) ---- */

#[no_mangle]
pub unsafe extern "C" fn memset(dst: *mut u8, c: i32, n: usize) -> *mut u8 {
    let mut i = 0usize;
    let b = c as u8;
    while i < n {
        core::ptr::write_volatile(dst.add(i), b);
        i += 1;
    }
    dst
}

#[no_mangle]
pub unsafe extern "C" fn memcpy(dst: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    let mut i = 0usize;
    while i < n {
        core::ptr::write_volatile(dst.add(i), core::ptr::read_volatile(src.add(i)));
        i += 1;
    }
    dst
}

#[no_mangle]
pub unsafe extern "C" fn memmove(dst: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    if (dst as usize) < (src as usize) {
        let mut i = 0usize;
        while i < n { core::ptr::write_volatile(dst.add(i), core::ptr::read_volatile(src.add(i))); i += 1; }
    } else if (dst as usize) > (src as usize) {
        let mut i = n;
        while i > 0 { i -= 1; core::ptr::write_volatile(dst.add(i), core::ptr::read_volatile(src.add(i))); }
    }
    dst
}

#[no_mangle]
pub unsafe extern "C" fn memcmp(a: *const u8, b: *const u8, n: usize) -> i32 {
    let mut i = 0usize;
    while i < n {
        let x = core::ptr::read_volatile(a.add(i));
        let y = core::ptr::read_volatile(b.add(i));
        if x != y { return x as i32 - y as i32; }
        i += 1;
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn strlen(s: *const u8) -> usize {
    let mut n = 0usize;
    while core::ptr::read_volatile(s.add(n)) != 0 { n += 1; }
    n
}

/* ---- extra Rust kernel service: CRC32 for VFS ---- */
#[no_mangle]
pub unsafe extern "C" fn rust_crc32(data: *const u8, len: u64) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    let mut i: u64 = 0;
    while i < len {
        crc ^= core::ptr::read_volatile(data.add(i as usize)) as u32;
        let mut j = 0;
        while j < 8 {
            let mask = (crc & 1).wrapping_neg();
            crc = (crc >> 1) ^ (0xEDB88320u32 & mask);
            j += 1;
        }
        i += 1;
    }
    !crc
}

/* ---- HAL (replaces kernel/hal/hal.c) ---- */
#[no_mangle]
pub extern "C" fn hal_init() {}
#[no_mangle]
pub extern "C" fn hal_cpu_halt() { unsafe { core::arch::asm!("hlt"); } }
#[no_mangle]
pub extern "C" fn hal_irq_enable() { unsafe { core::arch::asm!("sti"); } }
#[no_mangle]
pub extern "C" fn hal_irq_disable() { unsafe { core::arch::asm!("cli"); } }
#[no_mangle]
pub extern "C" fn hal_timer_ticks() -> u64 { unsafe { pit_get_ticks() } }

/* ---- PIT (replaces kernel/drivers/pit.c) ---- */
static mut PIT_TICKS: u64 = 0;
#[no_mangle]
pub extern "C" fn pit_tick() { unsafe { PIT_TICKS = PIT_TICKS.wrapping_add(1); } }
#[no_mangle]
pub extern "C" fn pit_get_ticks() -> u64 { unsafe { core::ptr::read_volatile(&PIT_TICKS) } }
#[no_mangle]
pub extern "C" fn pit_init(hz_in: u32) {
    let mut hz = hz_in; if hz == 0 { hz = 100; }
    let mut divisor = 1193182u32 / hz; if divisor == 0 { divisor = 1; } if divisor > 0xFFFF { divisor = 0xFFFF; }
    unsafe {
        core::arch::asm!("out dx, al", in("dx") 0x43u16, in("al") 0x36u8, options(nomem, nostack, preserves_flags));
        core::arch::asm!("out dx, al", in("dx") 0x40u16, in("al") (divisor & 0xFF) as u8, options(nomem, nostack, preserves_flags));
        core::arch::asm!("out dx, al", in("dx") 0x40u16, in("al") ((divisor >> 8) & 0xFF) as u8, options(nomem, nostack, preserves_flags));
    }
}

#[no_mangle]
pub unsafe extern "C" fn panic(msg: *const u8) -> ! {
    log_write(b"\n[KERNEL PANIC] \0".as_ptr());
    if !msg.is_null() { log_write(msg); } else { log_write(b"(unknown)\0".as_ptr()); }
    log_write(b"\n\0".as_ptr());
    loop { core::arch::asm!("cli; hlt"); }
}

#[no_mangle]
pub unsafe extern "C" fn qnu_assert(cond: bool, expr: *const u8, file: *const u8, line: i32) {
    if cond { return; }
    log_write(b"\n[ASSERT FAILED] \0".as_ptr());
    if !expr.is_null() { log_write(expr); }
    log_write(b" at \0".as_ptr());
    if !file.is_null() { log_write(file); }
    log_write(b"\n\0".as_ptr());
    panic(b"assertion failed\0".as_ptr());
}

#[panic_handler]
fn rust_panic_handler(_info: &PanicInfo) -> ! {
    loop { unsafe { core::arch::asm!("hlt"); } }
}
