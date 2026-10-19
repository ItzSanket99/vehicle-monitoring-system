#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT 5005

static volatile sig_atomic_t running = 1;

static void stop_handler(int sig)
{
    (void)sig;
    running = 0;
}

static int send_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0)
            return -1;
        sent += (size_t)n;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    const char *mode = argc > 1 ? argv[1] : "udp";
    const char *ip = argc > 2 ? argv[2] : "127.0.0.1";
    unsigned long port = argc > 3 ? strtoul(argv[3], NULL, 10)
                                  : (strcmp(mode, "tcp") == 0 ? 5006 : 5005);

    if ((strcmp(mode, "tcp") != 0 && strcmp(mode, "udp") != 0) ||
        port == 0 || port > 65535) {
        fprintf(stderr, "Usage: %s [udp|tcp] [IPv4] [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, ip, &dest.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", ip);
        return EXIT_FAILURE;
    }

    int type = strcmp(mode, "tcp") == 0 ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (type == SOCK_STREAM) {
        printf("Connecting to TCP dashboard at %s:%lu...\n", ip, port);
        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("connect");
            close(sock);
            return EXIT_FAILURE;
        }
    }

    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);
    signal(SIGPIPE, SIG_IGN);

    printf("Vehicle ECU started in %s mode. Press Ctrl+C to stop.\n", mode);

    unsigned int counter = 0;

    while (running) {
        unsigned int speed = (counter * 7U) % 121U;
        unsigned int rpm = 800U + ((counter * 250U) % 5201U);
        int steering = (int)((counter * 10U) % 121U) - 60;

        char msg[128];
        int n = snprintf(msg, sizeof(msg), "%u,%u,%d\n",
                         speed, rpm, steering);

        if (n < 0 || (size_t)n >= sizeof(msg))
            break;

        int result;

        if (type == SOCK_STREAM) {
            result = send_all(sock, msg, (size_t)n);
        } else {
            ssize_t sent = sendto(sock, msg, (size_t)n, 0,
                                  (struct sockaddr *)&dest, sizeof(dest));
            result = sent == n ? 0 : -1;
        }

        if (result < 0) {
            perror("send");
            break;
        }

        printf("Sent: Speed=%u km/h | RPM=%u | Steering=%d deg\n",
               speed, rpm, steering);
        fflush(stdout);
        counter++;

        struct timespec delay = {.tv_sec = 1, .tv_nsec = 0};
        while (running && nanosleep(&delay, &delay) == -1) {
            /* Continue remaining delay after a signal. */
        }
    }

    if (type == SOCK_STREAM)
        shutdown(sock, SHUT_WR);

    close(sock);
    printf("Vehicle ECU stopped.\n");
    return EXIT_SUCCESS;
}
