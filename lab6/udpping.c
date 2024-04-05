#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUFFER_SIZE 2048

void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *readPath = argv[1];
    int totalFiles = atoi(argv[2]);
    int port = atoi(argv[3]);
    const char *serverIP = argv[4];

    int sockfd;
    struct sockaddr_in serverAddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        die("socket");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) != 1)
        die("inet_pton");

    for (int fileNumber = 0; fileNumber < totalFiles; ++fileNumber) {
        char fileName[20];
        sprintf(fileName, "%s/%06d", readPath, fileNumber);

        FILE *file = fopen(fileName, "rb");
        if (file == NULL) {
            perror("fopen");
            continue;
        }

        char buffer[MAX_BUFFER_SIZE];
        size_t bytesRead;

        do {
            bytesRead = fread(buffer, 1, sizeof(buffer), file);
            sendto(sockfd, buffer, bytesRead, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        } while (bytesRead > 0);

        fclose(file);
    }

    close(sockfd);

    return 0;
}
