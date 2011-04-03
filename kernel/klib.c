
/* Stripped down version of C standard `printf` with only these specifiers:
 *   %c (character)
 *   %s (string)
 *   %d (decimal integer)
 *   %u (unsigned decimal integer)
 *   %x (hexadecimal integer
 *   %% (write '%' character)
 */
int printf(const char* format, ...)
{
    int size = 0;
    va_list ap;
    const char* ptr = format;
    char* str;

    va_start(ap, format);
    while (*ptr) {
        if (*ptr == '%') {
            ptr++;
            switch (*ptr) {
              case 'c':
                putchar((char) va_arg(ap, int));
                size++;
                break;
              case 's':
                str = va_arg(ap, char*);
                while (*str) {
                    putchar(*str++);
                    size++;
                }
                break;
              case 'd':
                size += print_dec(va_arg(ap, int));
                break;
              case 'u':
                size += print_udec(va_arg(ap, unsigned int));
                break;
              case 'x':
                size += print_uhex(va_arg(ap, int));
                break;
              case '%':
                putchar('%');
                size++;
            }
        } else {
            putchar(*ptr);
            size++;
        }
        ptr++;
    }
    va_end(ap);

    return size;
}

