// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

// #define errquit(m)	{ perror(m); exit(-1); }

// static int port_http = 80;
// static int port_https = 443;
// static const char *docroot = "/html";

// int main(int argc, char *argv[]) {
// 	int s;
// 	struct sockaddr_in sin;

// 	if(argc > 1) { port_http  = strtol(argv[1], NULL, 0); }
// 	if(argc > 2) { if((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
// 	if(argc > 3) { port_https = strtol(argv[3], NULL, 0); }

// 	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

// 	do {
// 		int v = 1;
// 		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
// 	} while(0);

// 	bzero(&sin, sizeof(sin));
// 	sin.sin_family = AF_INET;
// 	sin.sin_port = htons(80);
// 	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) errquit("bind");
// 	if(listen(s, SOMAXCONN) < 0) errquit("listen");

// 	do {
// 		int c;
// 		struct sockaddr_in csin;
// 		socklen_t csinlen = sizeof(csin);

// 		if((c = accept(s, (struct sockaddr*) &csin, &csinlen)) < 0) {
// 			perror("accept");
// 			continue;
// 		}
// 		char buf[1024];
// 		read(c, buf, 1024);
// 		char msg[1024] = "HTTP/1.0 200 OK\r\nContent-Type: text/html;charset=utf-8\r\n\r\n<html><head><title>Test</title></head><body><h2>It works!</h2><p>Hello，World</p></body></html>";
// 		write(c, msg, sizeof(msg));
// 		close(c);
// 	} while(1);

// 	return 0;
// }


// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <sys/sendfile.h>
// #include <netinet/in.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <stdbool.h>

// #define METHOD_SIZE 32
// #define PATH_SIZE 256
// #define errquit(m) { perror(m); exit(-1); }

// static int port_http = 80;
// static int port_https = 443;
// static const char *docroot = "/html";

// typedef struct {
//     char method[METHOD_SIZE];
//     char path[PATH_SIZE];
// } HTTPRequest;

// int parse_http_request(const char *request, HTTPRequest *http_request) {
//     const char *delim = " ?";
//     char *token;

//     char *line = strdup(request);
//     if (line == NULL) {
//         perror("strdup");
//         return -1;
//     }

//     token = strtok(line, delim);
//     if (token == NULL || strlen(token) >= METHOD_SIZE) {
//         fprintf(stderr, "Invalid HTTP method\n");
//         free(line);
//         return -1;
//     }
//     strcpy(http_request->method, token);

//     token = strtok(NULL, delim);
//     if (token == NULL || strlen(token) >= PATH_SIZE) {
//         fprintf(stderr, "Invalid path\n");
//         free(line);
//         return -1;
//     }
//     strcpy(http_request->path, token);

//     free(line);
//     return 0;
// }


// typedef struct {
//     const char *extension;
//     const char *mime_type;
// } MimeMap;

// MimeMap mime_types[] = {
//     {".html", "text/html"},
//     {".htm", "text/html"},
//     {".txt", "text/plain"},
//     {".jpg", "image/jpeg"},
//     {".jpeg", "image/jpeg"},
//     {".png", "image/png"},
//     {".gif", "image/gif"},
//     {".mp3", "audio/mpeg"},
//     {".wav", "audio/wav"},
//     // Add more mime types as needed
//     {NULL, NULL}
// };

// const char *get_mime_type(const char *file_name) {
//     // const char *extension = strrchr(file_name, '.');
// 	char *extension = strrchr(file_name, '.');
// 	if(extension[strlen(extension)-1] == '/'){
// 	    extension = strtok(extension, "/");
// 	}
//     if (extension) {
//         for (int i = 0; mime_types[i].extension != NULL; ++i) {
//             if (strcmp(extension, mime_types[i].extension) == 0) {
//                 return mime_types[i].mime_type;
//             }
//         }
//     }
//     return "application/octet-stream"; // Default mime type
// }

// void unimplemented(int c) {
//     char error_response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
//     send(c, error_response, strlen(error_response), 0);
// }

// void url_decode(const char *src, char *dest) {
// 	// memset(dest, '\0', sizeof(dest));
//     const char *p = src; // Pointer to the source URL-encoded string
//     char code[3] = {0}; // Buffer to store the hex-encoded characters ("%XY")
    
//     // Loop through the source string until reaching the end or encountering a '?' character
//     while (*p && *p != '?') {
//         if (*p == '%') {
//             // If the current character is '%', indicating an encoded character
//             memcpy(code, ++p, 2); // Copy the next two characters after '%' into the 'code' buffer
//             *dest++ = (char)strtoul(code, NULL, 16); // Convert the hexadecimal 'code' to its character equivalent and store it in 'dest'
//             p += 2; // Move the pointer 'p' ahead by 2 positions to skip the encoded characters
//         } 
// 		else {
//             // If the current character is not '%', implying a regular character
//             *dest++ = *p++; // Copy the character as it is into 'dest' and move the pointers ahead
//         }
//     }
    
//     *dest = '\0'; // Add a null-terminator to mark the end of the decoded string
// }


// void handle_request(int c) {
//     char buffer[8192];
//     ssize_t bytes_read = recv(c, buffer, sizeof(buffer) - 1, 0);
//     if (bytes_read > 0) {
//         buffer[bytes_read] = '\0';

//         HTTPRequest http_request;
//         if (parse_http_request(buffer, &http_request) == -1) {
//             char error_response[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
//             send(c, error_response, strlen(error_response), 0);
//             close(c);
//             return;
//         }

// 		if(strcmp(http_request.method, "GET") != 0){
// 			unimplemented(c);
// 			close(c);
// 			return;
// 		}

//         char *requested_path = http_request.path;
//         int len = strlen(requested_path);
// 		char full_path[256];

// 		bool redirect = 0;
// 		if(strcmp(requested_path, "/") == 0 || requested_path[len-1] == '/'){
// 			// requested_path = (char *)"/index.html";
// 			strcat(requested_path, "index.html");
// 			redirect = 1;
// 		}
// 		// char decoded_path[1024] = {0};
// 		// url_decode(requested_path, decoded_path);
// 		// snprintf(full_path, sizeof(full_path), "%s%s", docroot, decoded_path);
// 		snprintf(full_path, sizeof(full_path), "%s%s", docroot, requested_path);

// 		if (requested_path[len - 1] != '/' && len > 1) {
//     // Check if it's a directory without a trailing slash
// 			struct stat st;
// 			if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
// 				char redirect_header[512];
// 				snprintf(redirect_header, sizeof(redirect_header), "HTTP/1.0 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", requested_path);
// 				send(c, redirect_header, strlen(redirect_header), 0);
// 				// handle_request(c);
// 				close(c);
// 				return; // Redirect and exit the function
// 			}
// 		}
		

// 		int fd = open(full_path, O_RDONLY);
// 		if (fd == -1) {
// 			if (access(full_path, F_OK) == -1 && redirect) {
// 				char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
// 				send(c, forbidden_header, strlen(forbidden_header), 0);
// 				close(c);
// 				return;
// 			}
// 			// if (errno == EACCES || errno == EPERM) {
// 			// 	char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
// 			// 	send(c, forbidden_header, strlen(forbidden_header), 0);
// 			// } 
// 			else {
// 				char not_found_header[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>404 Not Found</TITLE><BODY><P>404 Not Found<br></BODY></HTML>\r\n";
// 				send(c, not_found_header, strlen(not_found_header), 0);
// 				// char tmp[1024];
// 				// snprintf(tmp, sizeof(tmp), "%s</BODY></HTML>", requested_path);
// 				// // send(c, requested_path, strlen(requested_path), 0);
// 				// send(c, tmp, strlen(tmp), 0);
// 				// sprintf(tmp, "\r\n");
// 				// send(c, tmp, strlen(tmp), 0);
// 			}
// 		} 
// 		else {
// 			struct stat file_stat;
// 			fstat(fd, &file_stat);

// 			char header[256];
// 			snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(full_path), file_stat.st_size);
// 			send(c, header, strlen(header), 0);

// 			off_t offset = 0;
// 			ssize_t remain_data = file_stat.st_size;
// 			while (remain_data > 0) {
// 				ssize_t sent_bytes = sendfile(c, fd, &offset, BUFSIZ);
// 				if (sent_bytes <= 0) {
// 					if (sent_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
// 						perror("sendfile");
// 					}
// 					break;
// 				}
// 				remain_data -= sent_bytes;
// 			}
// 			close(fd);
// 		}
//     //     if (strtok(temp, ".") == NULL && temp[len-1] != '/') { // maybe wrong!
//     //         char redirect_header[512];
//     //         snprintf(redirect_header, sizeof(redirect_header), "HTTP/1.0 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", requested_path);
//     //         send(c, redirect_header, strlen(redirect_header), 0);
//     //     } else {
//     //         char full_path[256];
// 	// 		if(strcmp(requested_path, "/") == 0){
// 	// 			requested_path = (char *)"/index.html";
// 	// 		}
			
// 	// 		// requested_path = (char *)"/demo.html";
//     //         snprintf(full_path, sizeof(full_path), "%s%s", docroot, requested_path);

//     //         int fd = open(full_path, O_RDONLY);
//     //         if (fd == -1) {
//     //             if (errno == EACCES || errno == EPERM) {
//     //                 char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
//     //                 send(c, forbidden_header, strlen(forbidden_header), 0);
//     //             } else {
//     //                 char not_found_header[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>404 Not Found</TITLE><BODY><P>404 Not Found<br></BODY></HTML>\r\n";
//     //                 char tmp[1024];
// 	// 				send(c, not_found_header, strlen(not_found_header), 0);
// 	// 				// snprintf(tmp, sizeof(tmp), "%s</BODY></HTML>", requested_path);
// 	// 				// // send(c, requested_path, strlen(requested_path), 0);
// 	// 				// send(c, tmp, strlen(tmp), 0);
// 	// 				// sprintf(tmp, "\r\n");
// 	// 				// send(c, tmp, strlen(tmp), 0);
//     //             }
//     //         } else {
//     //             struct stat file_stat;
//     //             fstat(fd, &file_stat);

//     //             char header[256];
//     //             snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(full_path), file_stat.st_size);
//     //             send(c, header, strlen(header), 0);

//     //             off_t offset = 0;
//     //             ssize_t remain_data = file_stat.st_size;
// 	// 			// while (remain_data > 0) {
//     //             //     ssize_t sent_bytes = send(c, buffer + offset, BUFSIZ, 0);
//     //             //     if (sent_bytes == -1) {
//     //             //         perror("send");
//     //             //         break;
//     //             //     }
//     //             //     remain_data -= sent_bytes;
//     //             //     offset += sent_bytes;
//     //             // }
// 	// 			while (remain_data > 0) {
// 	// 				ssize_t sent_bytes = sendfile(c, fd, &offset, BUFSIZ);
// 	// 				if (sent_bytes <= 0) {
// 	// 					if (sent_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
// 	// 						perror("sendfile");
// 	// 					}
// 	// 					break;
// 	// 				}
// 	// 				remain_data -= sent_bytes;
// 	// 			}
//     //             // while (((ssize_t)offset = sendfile(c, fd, &offset, BUFSIZ)) > 0 && (remain_data -= offset) > 0);
//     //             close(fd);
//     //         }
//     //     }
//     } else if (bytes_read == 0) {
// 		perror("receive zero byte");
//         // Connection closed by client
//     } else {
//         perror("recv");
//     }
//     close(c);
// 	return;
// }

// int main(int argc, char *argv[]) {
//     int s;
//     struct sockaddr_in sin;

//     if(argc > 1) { port_http  = strtol(argv[1], NULL, 0); }
//     if(argc > 2) { if((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
//     if(argc > 3) { port_https = strtol(argv[3], NULL, 0); }

//     if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

//     do {
//         int v = 1;
//         setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
//     } while(0);

//     bzero(&sin, sizeof(sin));
//     sin.sin_family = AF_INET;
//     sin.sin_port = htons(80);
//     if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) errquit("bind");
//     if(listen(s, SOMAXCONN) < 0) errquit("listen");

//     do {
//         int c;
//         struct sockaddr_in csin;
//         socklen_t csinlen = sizeof(csin);

//         if ((c = accept(s, (struct sockaddr*)&csin, &csinlen)) < 0) {
//             perror("accept");
//             continue;
//         }

//         handle_request(c);
//     } while (1);

//     return 0;
// }




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
#define errquit(m) { perror(m); exit(-1); }

static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";

typedef struct {
    char method[METHOD_SIZE];
    char path[PATH_SIZE];
} HTTPRequest;

int parse_http_request(const char *request, HTTPRequest *http_request) {
    const char *delim = " ?";
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
    strcpy(http_request->path, token);

    free(line);
    return 0;
}


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
    // Add more mime types as needed
    {NULL, NULL}
};

const char *get_mime_type(const char *file_name) {
    // const char *extension = strrchr(file_name, '.');
	char *extension = strrchr(file_name, '.');
	if(extension[strlen(extension)-1] == '/'){
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

void unimplemented(int c) {
    char error_response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(c, error_response, strlen(error_response), 0);
}

void url_decode(const char *src, char *dest) {
	// memset(dest, '\0', sizeof(dest));
    const char *p = src; // Pointer to the source URL-encoded string
    char code[3] = {0}; // Buffer to store the hex-encoded characters ("%XY")
    
    // Loop through the source string until reaching the end or encountering a '?' character
    while (*p && *p != '?') {
        if (*p == '%') {
            // If the current character is '%', indicating an encoded character
            memcpy(code, ++p, 2); // Copy the next two characters after '%' into the 'code' buffer
            *dest++ = (char)strtoul(code, NULL, 16); // Convert the hexadecimal 'code' to its character equivalent and store it in 'dest'
            p += 2; // Move the pointer 'p' ahead by 2 positions to skip the encoded characters
        } 
		else {
            // If the current character is not '%', implying a regular character
            *dest++ = *p++; // Copy the character as it is into 'dest' and move the pointers ahead
        }
    }
    
    *dest = '\0'; // Add a null-terminator to mark the end of the decoded string
}


void handle_request(int c) {
    char buffer[8192];
    ssize_t bytes_read = recv(c, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        HTTPRequest http_request;
        if (parse_http_request(buffer, &http_request) == -1) {
            char error_response[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
            send(c, error_response, strlen(error_response), 0);
            close(c);
            return;
        }

		if(strcmp(http_request.method, "GET") != 0){
			unimplemented(c);
			close(c);
			return;
		}

        char *requested_path = http_request.path;
        int len = strlen(requested_path);
		char full_path[256];

		bool redirect = 0;
		if(strcmp(requested_path, "/") == 0 || requested_path[len-1] == '/'){
			// requested_path = (char *)"/index.html";
			strcat(requested_path, "index.html");
			redirect = 1;
		}
		char decoded_path[PATH_SIZE] = {0};
		url_decode(requested_path, decoded_path);
		snprintf(full_path, sizeof(full_path), "%s%s", docroot, decoded_path);
		// snprintf(full_path, sizeof(full_path), "%s%s", docroot, requested_path);
        
        // Check if it's a directory without a trailing slash
		if (decoded_path[len - 1] != '/' && len > 1) {
        
			struct stat st;
			if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
				char redirect_header[512];
				snprintf(redirect_header, sizeof(redirect_header), "HTTP/1.0 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", decoded_path);
				send(c, redirect_header, strlen(redirect_header), 0);
				// handle_request(c);
				close(c);
				return; // Redirect and exit the function
			}
		}
		

		int fd = open(full_path, O_RDONLY);
		if (fd == -1) {
			if (access(full_path, F_OK) == -1 && redirect) {
				char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
				send(c, forbidden_header, strlen(forbidden_header), 0);
				close(c);
				return;
			}
			// if (errno == EACCES || errno == EPERM) {
			// 	char forbidden_header[] = "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>403 Forbidden</TITLE><BODY><P>403 Forbidden</BODY></HTML>\r\n";
			// 	send(c, forbidden_header, strlen(forbidden_header), 0);
			// } 
			else {
				char not_found_header[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>404 Not Found</TITLE><BODY><P>404 Not Found<br></BODY></HTML>\r\n";
				send(c, not_found_header, strlen(not_found_header), 0);
				// char tmp[1024];
				// snprintf(tmp, sizeof(tmp), "%s</BODY></HTML>", requested_path);
				// // send(c, requested_path, strlen(requested_path), 0);
				// send(c, tmp, strlen(tmp), 0);
				// sprintf(tmp, "\r\n");
				// send(c, tmp, strlen(tmp), 0);
			}
		} 
		else {
			struct stat file_stat;
			fstat(fd, &file_stat);

			char header[512]; //, content_type[] = get_mime_type(full_path);
            // if(strcmp(content_type, "text/plain") === 0)
            //     snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(full_path), file_stat.st_size);
            // else
			snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", get_mime_type(full_path), file_stat.st_size);
			send(c, header, strlen(header), 0);

			off_t offset = 0;
			ssize_t remain_data = file_stat.st_size;
			while (remain_data > 0) {
				ssize_t sent_bytes = sendfile(c, fd, &offset, BUFSIZ);
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
    } 
    else if(bytes_read == 0){
		perror("receive zero byte");
        // Connection closed by client
    } 
    else{
        perror("recv");
    }
    close(c);
	return;
}

// Function to handle each client request
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg); // Free the memory allocated for the argument
    
    handle_request(client_socket);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int s;
    struct sockaddr_in sin;

    if(argc > 1) { port_http  = strtol(argv[1], NULL, 0); }
    if(argc > 2) { if((docroot = strdup(argv[2])) == NULL) errquit("strdup"); }
    if(argc > 3) { port_https = strtol(argv[3], NULL, 0); }

    if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) errquit("socket");

    do {
        int v = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
    } while(0);

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) errquit("bind");
    if(listen(s, SOMAXCONN) < 0) errquit("listen");
    // if(listen(s, 500) < 0) errquit("listen");

    do {
        int c;
        struct sockaddr_in csin;
        socklen_t csinlen = sizeof(csin);

        if ((c = accept(s, (struct sockaddr*)&csin, &csinlen)) < 0) {
            perror("accept");
            continue;
        }

        // Create a separate memory for the argument to pass to the thread
        int *client_socket = malloc(sizeof(int));
        if (client_socket == NULL) {
            perror("malloc");
            close(c);
            continue;
        }
        *client_socket = c;

        // Create a thread for handling the client request
        pthread_t tid;
        int create_status = pthread_create(&tid, NULL, handle_client, (void *)client_socket);
        if (create_status != 0) {
            fprintf(stderr, "pthread_create error: %s\n", strerror(create_status));
            close(c);
            free(client_socket);
            continue;
        }

        // Detach the thread since it will no longer be joined
        pthread_detach(tid);

        // handle_request(c);
    } while (1);

    return 0;
}
