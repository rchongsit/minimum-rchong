#include <stdint.h>

#include "minemu/platform.h"
#include "minemu/irq.h"
#include "minemu/uart.h"

#define UART_RX_BUFFER_SIZE 256U


static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint32_t rx_head;
static volatile uint32_t rx_tail;

void minemu_uart_init(void)
{
    rx_head = 0;
    rx_tail = 0;
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
}

void minemu_uart_putc(char c)
{
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
    }

    MINEMU_UART0->tx_data = (uint32_t)c;
}

void minemu_uart_puts(const char *s)
{
    while (*s) {
        minemu_uart_putc(*s);
        s++;
    }
}

int minemu_uart_getc(char *out)
{
    int available = 0;

    minemu_irq_disable();

    if (rx_head != rx_tail) {
        *out = (char)rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1U) & (UART_RX_BUFFER_SIZE - 1U);
        available = 1;
    }

    minemu_irq_enable();

    return available;
}

void minemu_uart_handle_irq(void)
{
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        uint8_t byte = (uint8_t)MINEMU_UART0->rx_data;
        uint32_t next = (rx_head + 1U) & (UART_RX_BUFFER_SIZE - 1U);

        if (next != rx_tail) {
            rx_buffer[rx_head] = byte;
            rx_head = next;
        }
    }
}
