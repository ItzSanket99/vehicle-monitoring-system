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

typedef struct {
    unsigned int speed;
    unsigned int rpm;
    int steering_angle;
} VehicleData;

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

int main(int argc, char *argv[])
{
    const char *destination_ip =
        (argc >= 2) ? argv[1] : "127.0.0.1";
    unsigned int port = DEFAULT_PORT;

    if (argc >= 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 10);

        if (*argv[2] == '\0' || *end != '\0' ||
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

    struct sockaddr_in dashboard = {0};
    dashboard.sin_family = AF_INET;
    dashboard.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, destination_ip,
                  &dashboard.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n",
                destination_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Vehicle ECU: UDP sender started\n");
    printf("Dashboard destination: %s:%u\n",
           destination_ip, port);
    printf("Press Ctrl+C to stop.\n\n");

    unsigned int counter = 0;

    while (running) {
        VehicleData data = {
            .speed = (counter * 7U) % 121U,
            .rpm = 800U + ((counter * 250U) % 5201U),
            .steering_angle = (int)((counter * 10U) % 121U) - 60
        };

        char message[128];
        int length = snprintf(
            message, sizeof(message), "%u,%u,%d",
            data.speed, data.rpm, data.steering_angle);

        if (length < 0 || (size_t)length >= sizeof(message)) {
            fprintf(stderr, "Failed to format vehicle data\n");
            break;
        }

        ssize_t sent = sendto(
            sock, message, (size_t)length, 0,
            (struct sockaddr *)&dashboard, sizeof(dashboard));

        if (sent < 0) {
            perror("sendto");
        } else {
            printf("Sent: Speed=%u km/h | RPM=%u | Steering=%d deg\n",
                   data.speed, data.rpm, data.steering_angle);
            fflush(stdout);
        }

        counter++;

        struct timespec delay = {
            .tv_sec = 1,
            .tv_nsec = 0
        };

        while (running && nanosleep(&delay, &delay) == -1) {
            /* Resume the remaining delay after a signal. */
        }
    }

    close(sock);
    printf("\nVehicle ECU stopped.\n");
    return EXIT_SUCCESS;
}
