#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>

#define SERVER_PORT 5000
#define BUFFER_SIZE 1024
#define DATA_SIZE_MB 1024
#define DURATION 1800

void sendData(int socket, const char *buffer, size_t bufferSize) {
    size_t sentLength = 0;

    while (sentLength < bufferSize) {
        ssize_t sent = write(socket, buffer + sentLength, bufferSize - sentLength);
        if (sent < 0) {
            perror("write");
            break;
        } else if (sent == 0) {
            break;
        } else {
            sentLength += sent;
        }
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientLength = sizeof(clientAddress);
    int opt = 1;

    // Create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(serverSocket, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength);
        if (clientSocket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Send initial data to the client
        char delay_case[128];
        memset(delay_case, 'A', sizeof(delay_case));
        write(clientSocket, delay_case, sizeof(delay_case));

        // Prepare a buffer with data to be sent repeatedly
        char buffer[BUFFER_SIZE * DATA_SIZE_MB];
        memset(buffer, 'A', sizeof(buffer));

        // Record the start time
        struct timeval startTime, endTime;
        gettimeofday(&startTime, NULL);

        // Send data repeatedly until the specified duration elapses
        while (1) {
            sendData(clientSocket, buffer, sizeof(buffer));
            gettimeofday(&endTime, NULL);
            long double currentTimeMilliseconds = endTime.tv_sec * 1000.0 + endTime.tv_usec / 1000.0;
            if ((currentTimeMilliseconds - (startTime.tv_sec * 1000.0 + startTime.tv_usec / 1000.0)) > DURATION) {
                break;
            }
        }

        // Send a newline character to signal the end
        write(clientSocket, "\n", 1);

        // Close the client socket
        close(clientSocket);
    }

    // Close the server socket (This part will not be reached in the current implementation)
    close(serverSocket);
    return 0;
}
