#ifndef MINEMU_UART_H
#define MINEMU_UART_H

void minemu_uart_init(void);
void minemu_uart_putc(char c);
void minemu_uart_puts(const char *s);
int minemu_uart_getc(char *out);
void minemu_uart_handle_irq(void);

#endif
