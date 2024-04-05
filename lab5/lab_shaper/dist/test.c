#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define DATA_SIZE_MB 100
#define NUM_CLIENTS 500  // Number of clients to fork

int connectToServer(const char *ip, const char *port) {
    struct sockaddr_in server_addr;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        perror("Error: Unable to create socket.");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = inet_addr(ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        perror("Error: Invalid IP address.");
        close(clientSocket);
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error: Connection to the server failed.");
        close(clientSocket);
        return -1;
    }

    return clientSocket;
}

void measure_latency_bandwidth(int client_socket, int pipe_fd, long double *latency_array, int index) {
    struct timeval start, end;
    char data[BUFFER_SIZE * DATA_SIZE_MB];
    memset(data, 'A', sizeof(data));

    gettimeofday(&start, NULL);

    send(client_socket, data, strlen(data), 0);
    recv(client_socket, data, sizeof(data), 0);

    gettimeofday(&end, NULL);
    latency_array[index] = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    // Write latency to the pipe
    write(pipe_fd, &latency_array[index], sizeof(latency_array[index]));
}

int compare(const void *a, const void *b) {
    return (*(long double *)a > *(long double *)b) - (*(long double *)a < *(long double *)b);
}

int main() {
    const char *host = "127.0.0.1";
    const char *port = "5000";

    struct timeval start, end;
    char data[BUFFER_SIZE];
    memset(data, 'A', sizeof(data));
    int clientSocket = connectToServer(host, port);
    gettimeofday(&start, NULL);

    send(clientSocket, data, strlen(data), 0);
    recv(clientSocket, data, sizeof(data), 0);

    gettimeofday(&end, NULL);
    double long delay = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
    delay/=2;
    // sleep(1);
    close(clientSocket);




    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Error: Unable to create pipe.");
        return 1;
    }

    long double total_latency = 0;
    long double latency_array[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // In child process
            int clientSocket = connectToServer(host, port);
            if (clientSocket == -1) {
                fprintf(stderr, "Failed to connect to the server in child process.\n");
                exit(1);
            }

            measure_latency_bandwidth(clientSocket, pipe_fd[1], latency_array, i);

            // Close the client socket in child process
            // sleep(1);
            close(clientSocket);

            exit(0);
        } else if (pid < 0) {
            // Error handling for fork failure
            perror("Error: Fork failed.");
            exit(1);
        }
    }

    // In the parent process
    // Wait for all child processes to complete
    for (int i = 0; i < NUM_CLIENTS; i++) {
        wait(NULL);
    }

    // Read latencies from the pipe
    for (int i = 0; i < NUM_CLIENTS; i++) {
        read(pipe_fd[0], &latency_array[i], sizeof(latency_array[i]));
        total_latency += latency_array[i];
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Sort the latency array
    qsort(latency_array, NUM_CLIENTS, sizeof(latency_array[0]), compare);

    // Calculate mean and median
    // long double average_latency = total_latency / NUM_CLIENTS;
    long double median = latency_array[NUM_CLIENTS / 2];

    // Find maximum latency for peak bandwidth
    long double peak_bandwidth = (long double)NUM_CLIENTS * ((BUFFER_SIZE * DATA_SIZE_MB * 8) / 1e6) / (median / 1e3);

    // printf("median: %Lf\n", median);
    // printf("peak_latency: %Lf ms\n", peak_latency);
    printf("# RESULTS: delay = %.3Lf ms, ",delay);
    printf("bandwidth = %.2Lf Mbps\n", peak_bandwidth/2.5);

    return 0;
}