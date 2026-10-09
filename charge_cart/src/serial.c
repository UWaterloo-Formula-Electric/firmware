#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h> // probably wont be an include error on a linux machine
#include "../inc/serial.h"

static speed_t int_to_speed (int baudrate) {
    switch (baudrate) {
        case 0: return B0;
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default: return B0;
    }
}

int serial_open (const char *port, int baudrate) {

    speed_t speed = int_to_speed(baudrate);
    if (speed == B0) {
        fprintf(stderr, "Invalid baudrate: %d\n", baudrate);
        return -1;
    }

    int fd;
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Unable to open serial port");
        return -1;
    }
    struct termios interface;
    if (tcgetattr(fd, &interface) != 0) {
        perror("Failed to get serial port attributes");
        close(fd);
        return -1;
    }
    cfmakeraw(&interface);
    cfsetispeed(&interface, speed);
    cfsetospeed(&interface, speed);

    interface.c_cc[VMIN] = 0;
    interface.c_cc[VTIME] = 0;
    interface.c_cflag |= (CLOCAL | CREAD);

    if (tcsetattr(fd, TCSANOW, &interface) != 0) {
        perror("Failed to set serial port attributes");
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);

    return fd;
}

int serial_close (int fd) {
    if (close(fd) != 0) {
        perror("Failed to close serial port");
        return -1;
    }
    return 0;
}

int serial_read (int fd, uint8_t *buf, int len) {
    ssize_t bytes_read = read(fd, buf, len);
    if (bytes_read < 0) {
        if (errno == EAGAIN) {
            return 0;
        }
        perror("Failed to read from serial port");
        return -1;
    } else if (bytes_read == 0) {
        return 0;
    } else {
        return (int)bytes_read;
    }
}

int serial_write (int fd, const uint8_t *buf, int len) {
    ssize_t bytes_written = write(fd, buf, len);
    if (bytes_written < 0) {
        perror("Failed to write to serial port");
        return -1;
    } else {
        return (int)bytes_written;
    }
}