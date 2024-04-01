#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
using namespace std;

string download_challenge(const string& student_id) {
    string challenge_url = "https://inp.zoolab.org/binflag/challenge?id=" + student_id;
    string file_name = "challenge.bin";
    string curl_command = "curl -o " + file_name + " " + challenge_url;
    system(curl_command.c_str());
    return file_name;
}

uint16_t toBigEndian(uint16_t value) {
    return ((value >> 8) & 0xFF) | ((value << 8) & 0xFF00);
}
uint32_t toBigEndian(uint32_t value) {
    return ((value >> 24) & 0xFF) |
           ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) |
           ((value << 24) & 0xFF000000);
}


string reconstruct_dictionary_extract_flag(const string& file_name) {
    string flag = "";
    ifstream file(file_name, ios::binary);
    char file_header[16];
    file.read(file_header, 16);
    char magic[8];
    uint32_t datasize;
    uint16_t n_blocks, zeros;
    memcpy(magic, file_header, 8);
    memcpy(&datasize, file_header + 8, 4);
    memcpy(&n_blocks, file_header + 12, 2);
    n_blocks = toBigEndian(n_blocks);
    datasize = toBigEndian(datasize);
    memcpy(&zeros, file_header + 14, 2);
    vector<uint8_t> D(datasize);
    
    for (int i = 0; i < n_blocks; i++) {
        uint32_t offset;
        uint16_t cksum, length;
        char block_header[8];
        file.read(block_header, 8);
        memcpy(&offset, block_header, 4);
        memcpy(&cksum, block_header + 4, 2);
        memcpy(&length, block_header + 6, 2);
        offset = toBigEndian(offset);
        cksum = toBigEndian(cksum);
        length = toBigEndian(length);
        // Read payload
        vector<uint8_t> payload(length);
        file.read(reinterpret_cast<char*>(payload.data()), length);
        uint16_t computed_cksum = 0;
        for (int j = 0; j < length; j += 2) {
            uint16_t val;
            memcpy(&val, payload.data() + j, 2);
            computed_cksum ^= val;
        }
        computed_cksum = toBigEndian(computed_cksum);
        // Verify the checksum
        if (computed_cksum == cksum) {
            int insert_position = offset;
            for (int j = 0; j < length; j++) {
                D[insert_position + j] = payload[j];
            }
        } else { // ignore
            continue;
        }
        
    }
    
    char flag_header[2];
    file.read(flag_header, 2);
    uint16_t length;
    memcpy(&length, flag_header, 2);
    length = toBigEndian(length);
    vector<uint32_t> offsets(length);
    file.read(reinterpret_cast<char*>(offsets.data()), length * sizeof(uint32_t));
    for (uint32_t offset : offsets) {
        offset = toBigEndian(offset);
        string flag_chars(reinterpret_cast<char*>(&D[offset]), 2);
        flag += flag_chars;
    }
    return flag;
}

string convert_to_hex(const string& flag) {
    string hex_flag = "";
    for (char c : flag) {
        char hex[3];
        sprintf(hex, "%02x", static_cast<unsigned char>(c));
        hex_flag += hex;
    }
    return hex_flag;
}

int main() {
    string student_id;
    int branch = 0;
    char yn;
    cout<<"demo2? (y/n)";
    cin>>yn;
    string file_name;
    if(yn == 'y')
        file_name = "demo2.bin";
    else{
        cout << "Input student id: ";
        cin >> student_id;
        file_name = download_challenge(student_id);
    }
    string flag = reconstruct_dictionary_extract_flag(file_name);
    string hex_flag = convert_to_hex(flag);
    cout << "Hexadecimal Flag: " << hex_flag << endl;

    // Verify the flag using the provided URL
    string verification_url = "https://inp.zoolab.org/binflag/verify?v=" + hex_flag;
    string curl_command = "curl " + verification_url;
    system(curl_command.c_str());

    return 0;
}
