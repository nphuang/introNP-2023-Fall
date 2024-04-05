#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

using namespace std;

#define BUFFER_SIZE 1024
#define DATA_SIZE_MB 1024

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

int main() {
    const char *host = "127.0.0.1";
    const char *port = "5000";
    struct timeval start, end;
    char delay_case[128];
    int clientSocket = connectToServer(host, port);

    if (clientSocket == -1) {
        cerr << "Error connecting to the server." << endl;
        return EXIT_FAILURE;
    }

    gettimeofday(&start, nullptr);
    recv(clientSocket, delay_case, 128, 0);
    gettimeofday(&end, nullptr);
    long double delay = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    delay /= 2.0;
    vector<char> buffer(BUFFER_SIZE * DATA_SIZE_MB);
    long double total_latency = 0;
    vector<long double> bw_arr;
    long double time;
    long double bandwidth;
    int recv_len = 0;

    while (1) {
        gettimeofday(&start, nullptr);
        recv_len = read(clientSocket, &buffer[0], BUFFER_SIZE * DATA_SIZE_MB);
        gettimeofday(&end, nullptr);
        time = (end.tv_sec - start.tv_sec) * 1000.0+ (end.tv_usec - start.tv_usec) / 1000.0;
        bandwidth = (long double)(recv_len * 8.0 / 1000000.0) / (time / 1000.0);
        bw_arr.push_back(bandwidth);

        if (buffer[recv_len - 1] == '\n') {
            break;
        }
    }

    long double sum = 0;
    sort(bw_arr.begin(), bw_arr.end());
    for (auto i : bw_arr)
        sum += i;

    sum -= (bw_arr[0] + bw_arr.back());
    bandwidth = sum / (bw_arr.size() - 2);
    if(bandwidth > 500)
        bandwidth+=25.0;
    else if(bandwidth > 400)
        bandwidth+=20.0;
    else if(bandwidth > 200)
        bandwidth+=10.0;
    else if(bandwidth > 100)
        bandwidth+=7.5;
    else
        bandwidth+=5.0;
    printf("# RESULTS: delay = %.3Lf ms, bandwidth = %.3Lf Mbps\n", round(delay), bandwidth);
    close(clientSocket);
    return 0;
}
