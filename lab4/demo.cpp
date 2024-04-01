#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <arpa/inet.h>

using namespace std;

int connectToServer(const string &ip, const string &port)
{
    struct sockaddr_in server_addr;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1)
    {
        cerr << "Error: Unable to create socket." << endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port));
    if (inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr)) <= 0)
    {
        cerr << "Error: Invalid IP address." << endl;
        close(clientSocket);
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        cerr << "Error: Connection to the server failed." << endl;
        close(clientSocket);
        return -1;
    }

    return clientSocket;
}

string readServerResponse(int clientSocket)
{
    string result_response;
    bool inBody = false;
    char buffer[1024];
    while (true)
    {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0)
        {
            break;
        }
        if (!inBody)
        {
            string response(buffer, bytesRead);
            size_t pos = response.find("\r\n\r\n");
            if (pos != string::npos)
            {
                inBody = true;
                bytesRead -= (pos + 4);
                if (bytesRead > 0)
                {
                    result_response.append(buffer + pos + 4, bytesRead);
                }
            }
        }
        else
        {
            result_response.append(buffer, bytesRead);
        }
    }
    return result_response;
}


string urlEncode(const string &input)
{
    ostringstream encoded;
    encoded.fill('0');
    encoded << hex;

    for (char c : input)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded << c;
        }
        else
        {
            encoded << uppercase;
            encoded << '%' << int(static_cast<unsigned char>(c));
            encoded << nouppercase;
        }
    }

    return encoded.str();
}

string sendFileUploadRequest(int clientSocket, const string &host, const string &filename)
{
    // Open the file for reading
    int error = 0;
    socklen_t error_len = sizeof(error);

    ifstream file(filename, ios::in | ios::binary);
    if (!file.is_open())
    {
        cerr << "Failed to open file for reading." << endl;
        return "Error: Failed to open the file.";
    }

    // Read the file content into a string
    string fileContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    // Create the POST request body
    string boundary = "---------------------------" + to_string(time(nullptr));
    string body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
    body += "Content-Type: text/plain\r\n\r\n";
    body += fileContent;
    body += "\r\n--" + boundary + "--\r\n";

    string postRequest = "POST /upload HTTP/1.1\r\n";
    postRequest += "Host: " + host + "\r\n";
    postRequest += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
    postRequest += "Content-Length: " + to_string(body.size()) + "\r\n\r\n";
    postRequest += body;

    // Send the POST request
    send(clientSocket, postRequest.c_str(), postRequest.size(), 0);

    // Read and return the server's response
    return readServerResponse(clientSocket);
}
string sendOTPUploadRequest(int clientSocket, const string &host, const string &otp)
{
    string boundary = "---------------------------" + to_string(time(nullptr));
    string body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"otp.txt\"\r\n";
    body += "Content-Type: text/plain\r\n\r\n";
    body += otp; // Use the OTP directly
    body += "\r\n--" + boundary + "--\r\n";

    string postRequest = "POST /upload HTTP/1.1\r\n";
    postRequest += "Host: " + host + "\r\n";
    postRequest += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
    postRequest += "Content-Length: " + to_string(body.size()) + "\r\n\r\n";
    postRequest += body;

    // Send the POST request
    send(clientSocket, postRequest.c_str(), postRequest.size(), 0);

    // Read and return the server's response
    return readServerResponse(clientSocket);
}

int main()
{
    // 1. Get the server's IP address and port number
    string host = "172.21.0.4";
    string port = "10001";

    int clientSocket = connectToServer(host, port);
    if (clientSocket == -1)
    {
        cerr << "Failed to connect to the server." << endl;
        return 1;
    }
    string addrUrl = "/addr";
    string getRequest = "GET " + addrUrl + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
    send(clientSocket, getRequest.c_str(), getRequest.size(), 0);
    string adrResponse = readServerResponse(clientSocket);
    // close and reconnect
    close(clientSocket);
    clientSocket = connectToServer(host, port);
    cout << "addr:" << adrResponse;
    // 2. Get an OTP and save it as a text file
    string studentId = "110550012";
    string otpUrl = "/otp?name=" + studentId;
    // Send HTTP GET request and store the response in otpResponse
    getRequest = "GET " + otpUrl + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
    send(clientSocket, getRequest.c_str(), getRequest.size(), 0);

    // Receive the server's response
    string otpResponse = readServerResponse(clientSocket);

    close(clientSocket);
    clientSocket = connectToServer(host, port);

    cout << "OTP:" << otpResponse << '\n';
    string encodeOTP = urlEncode(otpResponse);
    cout << "Encode OTP:" << encodeOTP << '\n';
    string verUrl = "/verify?otp=" + encodeOTP;
    // Send HTTP GET request and store the response in verResponse
    getRequest = "GET " + verUrl + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
    send(clientSocket, getRequest.c_str(), getRequest.size(), 0);
    string verResponse = readServerResponse(clientSocket);

    close(clientSocket);
    clientSocket = connectToServer(host, port);

    cout << "verify response:" << verResponse;

    // Save the OTP to a text file
    // string otpFileName = "otp.txt";
    // ofstream otpFile(otpFileName);

    // if (otpFile.is_open())
    // {
    //     otpFile << otpResponse;
    //     otpFile.close();
    //     // cout << "OTP saved to " << otpFileName << endl;

    //     // 3. Upload the OTP file
    //     string uploadResponse = sendFileUploadRequest(clientSocket, host, otpFileName);
    //     cout << "File upload response:" << uploadResponse << endl;
    // }
    // else
    // {
    //     cerr << "Failed to create OTP file." << endl;
    // }
    string uploadResponse = sendOTPUploadRequest(clientSocket, host, otpResponse);
    cout << "upload response:" << uploadResponse << endl;


    close(clientSocket);
    clientSocket = connectToServer(host, port);
    // Check the logs
    string logsUrl = "/logs";
    getRequest = "GET " + logsUrl + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
    send(clientSocket, getRequest.c_str(), getRequest.size(), 0);
    string logsResponse = readServerResponse(clientSocket);
    close(clientSocket);
    cout << "\nlogs response:\n" << logsResponse;

    return 0;
}
