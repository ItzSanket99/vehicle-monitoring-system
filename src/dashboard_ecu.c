#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_UDP_PORT 5005
#define DEFAULT_TCP_PORT 5006
#define BUFFER_SIZE 512

static volatile sig_atomic_t running = 1;

static void stop_handler(int sig)
{
    (void)sig;
    running = 0;
}

static void display_message(const char *msg)
{
    unsigned int speed, rpm;
    int steering;
    char extra;

    if (sscanf(msg, "%u,%u,%d %c", &speed, &rpm, &steering, &extra) != 3 ||
        speed > 300 || rpm > 20000 ||
        steering < -720 || steering > 720) {
        fprintf(stderr, "Rejected invalid message: %s\n", msg);
        return;
    }

    printf("Vehicle data:\n");
    printf("  Speed:       %u km/h\n", speed);
    printf("  Engine RPM:  %u rpm\n", rpm);
    printf("  Steering:    %d degrees\n\n", steering);
    fflush(stdout);
}

static void process_tcp_client(int client)
{
    char buf[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    size_t used = 0;

    while (running) {
        ssize_t n = recv(client, buf, sizeof(buf), 0);

        if (n == 0) {
            printf("Vehicle ECU disconnected.\n");
            break;
        }

        if (n < 0) {
            if (errno == EINTR && running)
                continue;
            if (running)
                perror("recv");
            break;
        }

        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                line[used] = '\0';
                display_message(line);
                used = 0;
            } else if (used < sizeof(line) - 1) {
                line[used++] = buf[i];
            } else {
                fprintf(stderr, "Discarding oversized TCP message\n");
                used = 0;
            }
        }
    }

    close(client);
}

int main(int argc, char *argv[])
{
    const char *mode = argc > 1 ? argv[1] : "udp";
    unsigned long port = argc > 2 ? strtoul(argv[2], NULL, 10)
                                  : (strcmp(mode, "tcp") == 0
                                         ? DEFAULT_TCP_PORT
                                         : DEFAULT_UDP_PORT);

    if ((strcmp(mode, "tcp") != 0 && strcmp(mode, "udp") != 0) ||
        port == 0 || port > 65535) {
        fprintf(stderr, "Usage: %s [udp|tcp] [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int type = strcmp(mode, "tcp") == 0 ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);

    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return EXIT_FAILURE;
    }

    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);

    if (type == SOCK_STREAM) {
        if (listen(sock, 5) < 0) {
            perror("listen");
            close(sock);
            return EXIT_FAILURE;
        }

        printf("Dashboard listening in TCP mode on 127.0.0.1:%lu\n", port);

        while (running) {
            struct sockaddr_in peer;
            socklen_t peer_len = sizeof(peer);
            int client = accept(sock, (struct sockaddr *)&peer, &peer_len);

            if (client < 0) {
                if (errno == EINTR && !running)
                    break;
                if (errno == EINTR)
                    continue;
                perror("accept");
                break;
            }

            char peer_ip[INET_ADDRSTRLEN] = "?";
            inet_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));
            printf("TCP connection from %s:%u\n",
                   peer_ip, ntohs(peer.sin_port));

            process_tcp_client(client);
        }
    } else {
        printf("Dashboard listening in UDP mode on 127.0.0.1:%lu\n", port);

        while (running) {
            char buf[BUFFER_SIZE];
            ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0, NULL, NULL);

            if (n < 0) {
                if (errno == EINTR && !running)
                    break;
                if (errno == EINTR)
                    continue;
                perror("recvfrom");
                break;
            }

            buf[n] = '\0';
            display_message(buf);
        }
    }

    close(sock);
    printf("Dashboard stopped.\n");
    return EXIT_SUCCESS;
}
