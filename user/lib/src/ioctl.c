#include <stdarg.h>
#include <stdint.h>

#include "minemu/user_abi.h"

static int32_t minemu_ioctl_raw(
    int32_t fd,
    int32_t op,
    uint32_t arg2,
    uint32_t arg3)
{
    register uint32_t r0 __asm__("r0") = (uint32_t)fd;
    register uint32_t r1 __asm__("r1") = (uint32_t)op;
    register uint32_t r2 __asm__("r2") = arg2;
    register uint32_t r3 __asm__("r3") = arg3;
    register uint32_t r7 __asm__("r7") = MINEMU_SYSCALL_IOCTL;

    __asm__ volatile(
        "svc #0"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3), "r"(r7)
        : "memory", "cc");

    return (int32_t)r0;
}

int32_t ioctl(int32_t fd, int32_t op, ...)
{
    uint32_t arg2 = 0;
    uint32_t arg3 = 0;
    va_list args;

    va_start(args, op);

    if ((fd == (int32_t)MINEMU_FD_UART0 ||
         fd == (int32_t)MINEMU_FD_UART1) &&
        (op == (int32_t)MINEMU_IOCTL_UART_READ ||
         op == (int32_t)MINEMU_IOCTL_UART_WRITE)) {
        arg2 = (uint32_t)(uintptr_t)va_arg(args, void *);
        arg3 = va_arg(args, uint32_t);
    } else if (fd == (int32_t)MINEMU_FD_RNG) {
        if (op == (int32_t)MINEMU_IOCTL_RNG_STATE ||
            op == (int32_t)MINEMU_IOCTL_RNG_NEXT) {
            arg2 = (uint32_t)(uintptr_t)va_arg(args, uint32_t *);
        } else if (op == (int32_t)MINEMU_IOCTL_RNG_SEED) {
            arg2 = va_arg(args, uint32_t);
        }
    } else if (fd == (int32_t)MINEMU_FD_TRACE &&
               op == (int32_t)MINEMU_IOCTL_TRACE_EVENT) {
        arg2 = va_arg(args, uint32_t);
    }

    va_end(args);

    return minemu_ioctl_raw(fd, op, arg2, arg3);
}
