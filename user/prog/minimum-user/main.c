#include <stdint.h>
#include "minemu/user_abi.h"

#define MSH_LINE_MAX 64U

static int is_backspace(char c)
{
    return c == 0x08 || c == 0x7f;
}

static int is_echo_command(const char *command, uint32_t length)
{
    return length == 4U &&
           command[0] == 'e' &&
           command[1] == 'c' &&
           command[2] == 'h' &&
           command[3] == 'o';
}

static void uart_putn(const char *s, uint32_t length)
{
    if (length != 0U) {
        (void)ioctl(
            (int32_t)MINEMU_FD_UART_OUT,
            (int32_t)MINEMU_IOCTL_UART_WRITE,
            (void *)s,
            length
        );
    }
}

static void uart_puts(const char *s)
{
    uint32_t length = 0;

    while (s[length] != '\0') {
        length++;
    }

    uart_putn(s, length);
}

static void uart_getc(char *c)
{
    while (ioctl(
        (int32_t)MINEMU_FD_UART_IN,
        (int32_t)MINEMU_IOCTL_UART_READ,
        (void *)c,
        (uint32_t)1
    ) <= 0) {
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
        uart_puts("msh> ");
        return;
    }

    command_start = i;

    while (line[i] != '\0' && line[i] != ' ') {
        i++;
    }

    command_length = i - command_start;

    if (is_echo_command(&line[command_start], command_length)) {
        char output[MSH_LINE_MAX + 1U];
        uint32_t output_length = 0;
        int pending_space = 0;

        while (line[i] == ' ') {
            i++;
        }

        while (line[i] != '\0') {
            if (line[i] == ' ') {
                pending_space = 1;
            } else {
                if (pending_space && output_length > 0U) {
                    output[output_length++] = ' ';
                }

                output[output_length++] = line[i];
                pending_space = 0;
            }

            i++;
        }

        output[output_length] = '\0';
        uart_puts(output);
        uart_puts("\n");
    } else {
        uart_puts("command not found: ");
        uart_putn(&line[command_start], command_length);
        uart_puts("\n");
    }

    uart_puts("msh> ");
}

void minemu_user_main(void)
{
    char line[MSH_LINE_MAX + 1U];
    uint32_t length = 0;
    int overlong = 0;

    uart_puts("msh> ");

    for (;;) {
        char c;

        uart_getc(&c);

        if (c == '\n') {
            if (overlong) {
                uart_puts("msh> ");
            } else {
                line[length] = '\0';
                process_command(line);
            }

            length = 0;
            overlong = 0;
            continue;
        }

        if (is_backspace(c)) {
            if (!overlong && length > 0U) {
                length--;
            }

            continue;
        }

        if (!overlong) {
            if (length < MSH_LINE_MAX) {
                line[length++] = c;
            } else {
                overlong = 1;
            }
        }
    }
}
