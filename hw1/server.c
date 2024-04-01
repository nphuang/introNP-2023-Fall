#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#define METHOD_SIZE 32
#define PATH_SIZE 256

#define errquit(m) { perror(m); exit(EXIT_FAILURE); }

static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";

typedef struct {
    char method[METHOD_SIZE];
    char path[PATH_SIZE];
} HTTPRequest;

typedef struct {
    const char *extension;
    const char *mime_type;
} MimeMap;

MimeMap mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".txt", "text/plain; charset=utf-8"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {NULL, NULL}
};

// Function declarations
int parse_http_request(const char *request, HTTPRequest *http_request);
const char *get_mime_type(const char *file_name);
void url_decode(const char *encoded, char *decoded);
void *handle_client(void *arg);

int main(int argc, char *argv[]) {
    int server_socket;
    struct sockaddr_in server_address;

    if (argc > 1) { port_http = strtol(argv[1], NULL, 0); }
    if (argc > 2) { if ((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
    if (argc > 3) { port_https = strtol(argv[3], NULL, 0); }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

    // Enable reuse of the address
    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) errquit("setsockopt");

    // Set up server address structure
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_http);

    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) errquit("bind");
    if (listen(server_socket, SOMAXCONN) < 0) errquit("listen");

    do {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);

        if ((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length)) < 0) {
            perror("accept");
            continue;
        }

        // Create a separate memory for the argument to pass to the thread
        int *client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL) {
            perror("malloc");
            close(client_socket);
            continue;
        }
        *client_socket_ptr = client_socket;

        // Create a thread for handling the client request
        pthread_t thread_id;
        int create_status = pthread_create(&thread_id, NULL, handle_client, (void *)client_socket_ptr);
        if (create_status != 0) {
            fprintf(stderr, "pthread_create error: %s\n", strerror(create_status));
            close(client_socket);
            free(client_socket_ptr);
            continue;
        }

        // Detach the thread since it will no longer be joined
        pthread_detach(thread_id);

    } while (1);

    return 0;
}

int parse_http_request(const char *request, HTTPRequest *http_request) {
    const char *delim = " ";
    char *token;

    char *line = strdup(request);
    if (line == NULL) {
        perror("strdup");
        return -1;
    }

    token = strtok(line, delim);
    if (token == NULL || strlen(token) >= METHOD_SIZE) {
        fprintf(stderr, "Invalid HTTP method\n");
        free(line);
        return -1;
    }
    strcpy(http_request->method, token);

    token = strtok(NULL, delim);
    if (token == NULL || strlen(token) >= PATH_SIZE) {
        fprintf(stderr, "Invalid path\n");
        free(line);
        return -1;
    }
    char *pos = strrchr(token, '?');
    if(pos != NULL){
        int len = pos - token;
        if(len == 1){
            char *tmp = (char *)"/";
            strcpy(http_request->path, tmp);
        }
        else
            strncpy(http_request->path, token, len-1);
    }
    else 
        strcpy(http_request->path, token);

    free(line);
    return 0;
}

const char *get_mime_type(const char *file_name) {
    char *extension = strrchr(file_name, '.');
    if (extension && extension[strlen(extension)-1] == '/') {
        extension = strtok(extension, "/");
    }
    
    if (extension) {
        for (int i = 0; mime_types[i].extension != NULL; ++i) {
            if (strcmp(extension, mime_types[i].extension) == 0) {
                return mime_types[i].mime_type;
            }
        }
    }
    
    return "application/octet-stream"; // Default mime type
}

void url_decode(const char *encoded, char *decoded) {
    const char *source = encoded;
    char hex_code[3] = {0};
    
    while (*source && *source != '?') {
        if (*source == '%') {
            memcpy(hex_code, ++source, 2);
            *decoded++ = (char)strtoul(hex_code, NULL, 16);
            source += 2;
        } else {
            *decoded++ = *source++;
        }
    }
    
    *decoded = '\0';
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);

    char buffer[8192];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        HTTPRequest http_request;
        if (parse_http_request(buffer, &http_request) == -1) {
            char error_response[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
            send(client_socket, error_response, strlen(error_response), 0);
            close(client_socket);
            return NULL;
        }

        if (strcmp(http_request.method, "GET") != 0) {
            char error_response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
            send(client_socket, error_response, strlen(error_response), 0);
            close(client_socket);
            return NULL;
        }

        char *requested_path = http_request.path;
        int len = strlen(requested_path);
        char full_path[256];

        bool redirect = false;
        if (strcmp(requested_path, "/") == 0 || requested_path[len - 1] == '/') {
            strcat(requested_path, "index.html");
            redirect= true;
        }
        char decoded_path[PATH_SIZE] = {0};
        url_decode(requested_path, decoded_path);
        snprintf(full_path, sizeof(full_path), "%s%s", docroot, decoded_path);

        if (decoded_path[len - 1] != '/' && len > 1) {
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char redirect_header[512];
                snprintf(redirect_header, sizeof(redirect_header), "HTTP/1.0 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", decoded_path);
                send(client_socket, redirect_header, strlen(redirect_header), 0);
                close(client_socket);
                return NULL;
            }
        }

        int fd = open(full_path, O_RDONLY);
        if (fd == -1) {
            if (access(full_path, F_OK) == -1 && redirect) {
                char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
                send(client_socket, forbidden_header, strlen(forbidden_header), 0);
                close(client_socket);
                return NULL;
            } else {
                char not_found_header[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>404 Not Found</TITLE><BODY><P>404 Not Found<br></BODY></HTML>\r\n";
                send(client_socket, not_found_header, strlen(not_found_header), 0);
            }
        } else {
            struct stat file_stat;
            fstat(fd, &file_stat);

            char header[512];
            snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(full_path), file_stat.st_size);
            send(client_socket, header, strlen(header), 0);

            off_t offset = 0;
            ssize_t remain_data = file_stat.st_size;
            while (remain_data > 0) {
                ssize_t sent_bytes = sendfile(client_socket, fd, &offset, BUFSIZ);
                if (sent_bytes <= 0) {
                    if (sent_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("sendfile");
                    }
                    break;
                }
                remain_data -= sent_bytes;
            }
            close(fd);
        }
    } else if (bytes_read == 0) {
        perror("receive zero byte");
    } else {
        perror("recv");
    }

    close(client_socket);
    return NULL;
}

