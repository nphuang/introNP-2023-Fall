#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

// Define the server's base URL
const string SERVER_BASE_URL = "inp.zoolab.org";
const int SERVER_PORT = 10314;

// Function to perform an HTTP GET request
string performGetRequest(const string& path) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent* server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error: Unable to open socket." << endl;
        return "";
    }

    server = gethostbyname(SERVER_BASE_URL.c_str());
    if (server == NULL) {
        cerr << "Error: No such host." << endl;
        close(sockfd);
        return "";
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    bcopy((char*)server->h_addr, (char*)&server_addr.sin_addr.s_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) < 0)) {
        cerr << "Error: Connection failed." << endl;
        close(sockfd);
        return "";
    }

    stringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << SERVER_BASE_URL << "\r\n";
    request << "Connection: close\r\n\r\n";

    string requestStr = request.str();
    send(sockfd, requestStr.c_str(), requestStr.length(), 0);

    stringstream response;
    char buffer[1024];
    int n;
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0) > 0)) {
        buffer[n] = '\0';
        response << buffer;
    }

    close(sockfd);
    return response.str();
}

// Function to perform an HTTP POST request
string performPostRequest(const string& path, const string& postData) {
    // Implementing POST request without curl is more complex
    // You may need to construct the HTTP request manually and send it via a socket
    // The example here only covers GET requests
    cerr << "Error: POST requests not implemented without libcurl." << endl;
    return "";
}

int main() {
    // 1. Get the server's IP address and port number
    string addrUrl = "/addr";
    string serverAddress = performGetRequest(addrUrl);
    cout << "Server Address: " << serverAddress << endl;

    // 2. Get an OTP and save it as a text file
    string studentId = "110550012";
    string otpUrl = "/otp?name=" + studentId;
    string otpResponse = performGetRequest(otpUrl);
    cout << "OTP: " << otpResponse << endl;

    // Save the OTP to a text file (you can save it to a file or use it in memory)
    // Example: ofstream otpFile("otp.txt"); otpFile << otpResponse; otpFile.close();

    // 3. Upload the OTP file (Not implemented here, as it's more complex without curl)

    // 4. Check if the operation was successful
    // Example: string logsUrl = "/logs";
    // Example: string logsResponse = performGetRequest(logsUrl);
    // Example: cout << "Logs:\n" << logsResponse << endl;

    return 0;
}
