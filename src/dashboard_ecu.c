#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 5005
#define BUFFER_SIZE 256

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

int main(int argc, char *argv[])
{
    unsigned int port = DEFAULT_PORT;

    if (argc >= 2) {
        char *end = NULL;
        unsigned long value = strtoul(argv[1], &end, 10);

        if (*argv[1] == '\0' || *end != '\0' ||
            value == 0 || value > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return EXIT_FAILURE;
        }
        port = (unsigned int)value;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sock, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind");
        close(sock);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Dashboard ECU: UDP receiver listening on 127.0.0.1:%u\n",
           port);
    printf("Waiting for vehicle data. Press Ctrl+C to stop.\n\n");

    while (running) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in sender;
        socklen_t sender_length = sizeof(sender);

        ssize_t received = recvfrom(
            sock, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr *)&sender, &sender_length);

        if (received < 0) {
            if (errno == EINTR && !running)
                break;

            if (errno == EINTR)
                continue;

            perror("recvfrom");
            close(sock);
            return EXIT_FAILURE;
        }

        buffer[received] = '\0';

        unsigned int speed, rpm;
        int steering;
        char extra;

        if (sscanf(buffer, "%u,%u,%d %c",
                   &speed, &rpm, &steering, &extra) != 3 ||
            speed > 300 || rpm > 20000 ||
            steering < -720 || steering > 720) {
            fprintf(stderr, "Rejected invalid vehicle message: %s\n",
                    buffer);
            continue;
        }

        char sender_ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &sender.sin_addr,
                  sender_ip, sizeof(sender_ip));

        printf("Vehicle data from %s:%u\n",
               sender_ip, ntohs(sender.sin_port));
        printf("  Speed:         %u km/h\n", speed);
        printf("  Engine RPM:    %u rpm\n", rpm);
        printf("  Steering:      %d degrees\n\n", steering);
        fflush(stdout);
    }

    close(sock);
    printf("\nDashboard ECU stopped.\n");
    return EXIT_SUCCESS;
}
