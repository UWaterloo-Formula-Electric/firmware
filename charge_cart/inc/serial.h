#include <stdint.h>
#include <termios.h>

int serial_open (const char *port, speed_t baudrate);
int serial_close (int fd);
int serial_read (int fd, uint8_t *buf, int len);
int serial_write (int fd, const uint8_t *buf, int len);
speed_t int_to_speed (int baudrate);