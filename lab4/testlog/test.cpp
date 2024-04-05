#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <sstream>

using namespace std;
// Define the server's base URL
#define SERVER_BASE_URL "http://inp.zoolab.org:10314"

// Function to perform an HTTP GET request
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    string* data = (string*)userp;
    data->append((char*)contents, realsize);
    return realsize;
}

void performGetRequest(const string& url, string& response) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);
    }
}

// Function to perform an HTTP POST request with a file
void performPostRequestWithFile(const string& url, const string& filePath) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_mime* form = curl_mime_init(curl);

        curl_mimepart* field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, filePath.c_str());

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }

        curl_mime_free(form);
        curl_easy_cleanup(curl);
    }
}

string urlEncode(const string &input) {
    ostringstream encoded;
    encoded.fill('0');
    encoded << hex;

    for (char c : input) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << uppercase;
            encoded << '%' << int(static_cast<unsigned char>(c));
            encoded << nouppercase;
        }
    }

    return encoded.str();
}


int main() {
    // 1. Get the server's IP address and port number
    string addrUrl = SERVER_BASE_URL;
    addrUrl += "/addr";
    string serverAddress;
    performGetRequest(addrUrl, serverAddress);
    cout << "Server Address: " << serverAddress << endl;

    // 2. Get an OTP and save it as a text file
    string studentId = "110550012";
    string otpUrl = SERVER_BASE_URL;
    otpUrl += "/otp?name=" + studentId;
    string otpResponse;
    performGetRequest(otpUrl, otpResponse);
    cout<<"OTP:"<<otpResponse<<'\n';
    string encodeOTP = urlEncode(otpResponse);
    cout<<"Encode OTP:"<<encodeOTP<<'\n';
    string verUrl = SERVER_BASE_URL;
    verUrl += "/verify?otp=" + encodeOTP;
    string verResponse;
    performGetRequest(verUrl, verResponse);
    cout<<"verify response:"<< verResponse<<'\n';

    // Save the OTP to a text file
    string otpFileName = "otp.txt";
    ofstream otpFile(otpFileName);
    if (otpFile.is_open()) {
        otpFile << otpResponse;
        otpFile.close();
        cout << "OTP saved to " << otpFileName << endl;

        // 3. Upload the OTP file
        string uploadUrl = SERVER_BASE_URL;
        uploadUrl += "/upload";
        performPostRequestWithFile(uploadUrl, otpFileName);
        cout << "OTP file uploaded to the server." << endl;

        // 4. Check if the operation was successful
        // performGetRequest(uploadUrl, otpResponse);
        // cout << "Operation status: " << otpResponse << endl;

        // 5. Check the logs
        string logsUrl = SERVER_BASE_URL;
        logsUrl += "/logs";
        string logsResponse;
        performGetRequest(logsUrl, logsResponse);
        cout << "Logs:\n" << logsResponse << endl;
    } else {
        cerr << "Failed to create OTP file." << endl;
    }

    return 0;
}
