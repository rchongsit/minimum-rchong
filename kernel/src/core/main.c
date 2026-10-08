// #include "minemu/boot.h"
// #include "minemu/trap.h"
// #include "minemu/trace.h"
// #include "minemu/platform.h"

// static void uart_putc(char c)
// {
//     while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
//     }

//     MINEMU_UART0->tx_data = (uint32_t)c;
// }

// static void uart_puts(const char *s)
// {
//     while (*s) {
//         uart_putc(*s);
//         s++;
//     }
// }

// void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
//     if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
//         boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
//         boot_info->version != MINEMU_ABI_VERSION ||
//         boot_info->size != sizeof(*boot_info) ||
//         boot_info->system_rom_base != UINT32_C(0x08000000) ||
//         boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
//         boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
//         boot_info->direct_map_size != UINT32_C(0x04000000)) {
//         minemu_trace_event(UINT32_C(0xb007bad0));
//         minemu_fail_stop();
//     }
//     uart_puts("hello world\n");
//     minemu_fail_stop();
//     // minemu_trace_event(1);
//     // minemu_fail_stop();
// }

#include <stdint.h>

#include "minemu/boot.h"
#include "minemu/irq.h"
#include "minemu/platform.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/uart.h"

#define MSH_LINE_MAX 20U

static int is_backspace(char c)
{
    return c == 0x08 || c == 0x7f;
}

static int is_echo_command(const char *command, uint32_t length)
{
    return length == 4 &&
           command[0] == 'e' &&
           command[1] == 'c' &&
           command[2] == 'h' &&
           command[3] == 'o';
}

static void uart_putn(const char *s, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        minemu_uart_putc(s[i]);
    }
}

static void process_command(char *line)
{
    uint32_t i = 0;
    uint32_t command_start;
    uint32_t command_length;

    while (line[i] == ' ') {
        i++;
    }

    if (line[i] == '\0') {
        minemu_uart_puts("msh> ");
        return;
    }

    command_start = i;

    while (line[i] != '\0' && line[i] != ' ') {
        i++;
    }

    command_length = i - command_start;

    if (is_echo_command(&line[command_start], command_length)) {
        while (line[i] == ' ') {
            i++;
        }

        minemu_uart_puts(&line[i]);
        minemu_uart_putc('\n');
    } else {
        minemu_uart_puts("command not found: ");
        uart_putn(&line[command_start], command_length);
        minemu_uart_putc('\n');
    }

    minemu_uart_puts("msh> ");
}

static void msh(void)
{
    char line[MSH_LINE_MAX + 1U];
    uint32_t length = 0;

    minemu_uart_puts("msh> ");

    for (;;) {
        char c;

        while (!minemu_uart_getc(&c)) {
        }

        if (c == '\n') {
            line[length] = '\0';
            process_command(line);
            length = 0;
            continue;
        }

        if (is_backspace(c)) {
            if (length > 0) {
                length--;
            }
            continue;
        }

        if (length < MSH_LINE_MAX) {
            line[length] = c;
            length++;
        }
    }
}

void minemu_kernel_main(const struct minemu_boot_info *boot_info)
{
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }

    minemu_uart_init();

    minemu_uart_puts("hello world\n");

    MINEMU_INTERRUPT->enable = UINT32_C(1) << MINEMU_IRQ_UART0;

    minemu_irq_enable();

    msh();
}