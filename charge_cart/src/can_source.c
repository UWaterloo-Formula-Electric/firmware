#include <stdint.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../inc/can_source.h"

int can_open (const char *ifname) {
    int fd;
    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd == -1) {
        perror("Unable to open CAN socket");
        return -1;
    }
    struct ifreq ifr;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) == -1) {
        perror("Failed to get CAN interface index");
        close(fd);
        return -1;
    }
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Failed to bind CAN socket");
        close(fd);
        return -1;
    }
    fcntl(fd, F_SETFL, O_NONBLOCK);
    return fd;
}

int can_close (int fd) {
    if (close(fd) != 0) {
        perror("Failed to close CAN socket");
        return -1;
    }
    return 0;
}

int can_poll (int fd, can_frame_t *out, int max) {
    int count = 0;
    while (count < max) {
        struct can_frame kf;
        ssize_t n = read(fd, &kf, sizeof kf);
        if (n < 0) {
            if (errno == EAGAIN) {
                break;
            }
            perror("Failed to read from CAN socket");
            return -1;
        } else if (n == 0) {
            break;
        } else {
            out[count].id  = kf.can_id & CAN_EFF_MASK;
            out[count].len = kf.can_dlc;
            memcpy(out[count].data, kf.data, 8);
            out[count].rx_ms = now_ms();
            count++;
        }
    }
    return count;
}