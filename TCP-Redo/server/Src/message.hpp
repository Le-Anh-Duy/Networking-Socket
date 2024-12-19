#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#define RECIEVE_BUFFER_SIZE 2048
#define SEND_BUFFER_SIZE 2048
#define SHORT_MESSAGE_LEN 256
#define DATA_LEN 1400

vector <string> valid_message = {
    "GET_LIST",
    "DOWNLOAD_FILE",
    "QUIT",
    "OK",
    "NO",
    "WORKER_GET_CHUNK"
};

#pragma pack(push, 1)
struct short_message {
    int len;
    char content[SHORT_MESSAGE_LEN];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct data_message {
    int len;
    char content[DATA_LEN];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct start_file_transfer {
    unsigned long long file_size;
    int len;
    char filename[SHORT_MESSAGE_LEN];
};

#pragma pack(pop)

#pragma pack(push, 1)
struct start_chunk_transfer {
    unsigned long long file_size;
    unsigned long long offset;
    unsigned long long offset_lenght;
    int len;
    char filename[SHORT_MESSAGE_LEN];
};
#pragma pack(pop)



string get_content_short(const short_message& mess) {
    string res = "";
    for (int i = 0; i < mess.len; i ++) res += mess.content[i];
    return res;

}

string get_content(char ch[], int len) {
    string res = "";
    for (int i = 0; i < len; i ++) res += ch[i];
    return res;  
}

short_message make_short_message(const string& s) {
    short_message res;
    res.len = s.size();
    if (res.len > SHORT_MESSAGE_LEN) res.len = SHORT_MESSAGE_LEN;
    for (int i = 0; i < res.len; i ++) res.content[i] = s[i];

    return res;
}

template <typename T>
bool copy_buffer_to_message(char* buffer, int size, T& target) {
    if (size != sizeof(T)) return false;
    char* data = reinterpret_cast<char*>(&target);
    for (int i = 0; i < size; i ++) data[i] = buffer[i];
    return true;
}

template <typename T>
int send(T& data, SOCKET& server, const std::string& error_message) {
    int total = sizeof(T);  // Total size of the structure in bytes
    int res = 0;            // Number of bytes successfully sent
    int numRetries = 0;     // Retry counter
    int maxRetries = 5;     // Maximum number of retries

    while (res < total) {
        int tmp = ::send(server, reinterpret_cast<char*>(&data) + res, total - res, 0);
        
        if (tmp == SOCKET_ERROR) {
            int errCode = WSAGetLastError();
            std::cerr << "Cannot send data: " + error_message + ", Error code: " << errCode << '\n';

            if (errCode == WSAEWOULDBLOCK || errCode == WSAENOBUFS) {
                if (numRetries >= maxRetries) {
                    std::cerr << "Max retries reached, giving up.\n";
                    return SOCKET_ERROR; // Or handle according to your use case
                }
                numRetries++;
                Sleep(100); // Optional: wait a bit before retrying
                continue;
            } else {
                return SOCKET_ERROR;
            }
        }

        res += tmp; // Successfully sent some bytes
    }

    return res; // Return the number of bytes successfully sent
}

template <typename T>
int recv(T& data, SOCKET& server, const string& error_message) {
    
    char buffer[RECIEVE_BUFFER_SIZE];
    int res = 0;
    int sizeR = sizeof(T);
    while (res < sizeR) {
        int bytesReceived = ::recv(server, buffer + res, sizeR - res, 0);
        
        if (bytesReceived == SOCKET_ERROR) {
            int errCode = WSAGetLastError();
            std::cerr << "Cannot receive data: " + error_message + ", Error code: " << errCode << '\n';
            if (errCode == 10054) return -2;
            return -1;
        }

        if (bytesReceived == 0) {
            std::cerr << "Connection closed unexpectedly: " + error_message << '\n';
            return -1;
        }

        res += bytesReceived;
    }



    if (res < 0) {
        std::cerr << "Cannot receive data: " + error_message;
        return -1;
    }

    if (res != sizeof(T)) {

        std::cout << "Wrong protocols: " << error_message << " - Expected size " << sizeof(T) << " but get " << res << '\n';
        return -1;    
    }
    copy_buffer_to_message(buffer, res, data);
    return res;    
}

bool is_valid_message(const string& message) {
    return find(valid_message.begin(), valid_message.end(), message) != valid_message.end();
}