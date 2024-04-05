#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h> // 为了使用 isdigit 函数

#define MAX_USERNAME_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_MESSAGE_LEN 150
#define MAX_CHAT_HISTORY_LEN 10
#define MAX_CHAT_ROOMS 100
#define FILTER_LIST_SIZE 5
#define MAX_CLIENTS 100
#define PORT 8000 

int userCount = 0;
int chatRoomCount = 0;

typedef struct {
    char username[MAX_USERNAME_LEN + 1];
    char password[MAX_PASSWORD_LEN + 1];
    int isLoggedIn; // 1 if user is logged in, 0 otherwise
    char status[10]; // online, offline, busy
} User;

typedef struct {
    int roomNumber;
    char owner[MAX_USERNAME_LEN + 1];
    // 可以加入其他聊天室信息，如成员列表等
    // ...

} ChatRoom;

const char* filterList[FILTER_LIST_SIZE] = {"==", "Superpie", "hello", "Starburst Stream", "Domain Expansion"};
int containsKeyword(const char* message) {
    for (int i = 0; i < FILTER_LIST_SIZE; ++i) { 
        if (strcasestr(message, filterList[i]) != NULL) {
            return 1; 
        }
    }
    return 0; 
}

// 处理客户端命令
void processCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount, fd_set* readfds);
// 注册新用户
void registerUser(int clientSocket, char* command, User* users, int* userCount);

// 处理登录命令
void loginUser(int clientSocket, char* command, User* users, int* userCount);

// 处理登出命令
void logoutUser(int clientSocket, char* command, User* users, int* userCount);

// 处理退出命令
void exitCommand(int clientSocket, char* command, User* users, int* userCount, fd_set* readfds);

// 处理 whoami 命令
void whoamiCommand(int clientSocket, char* command, User* users, int* userCount);

// 处理 set-status 命令
void setStatusCommand(int clientSocket, char* command, User* users, int* userCount);

// 处理 list-user 命令
void listUserCommand(int clientSocket, char* command, User* users, int* userCount);

// 处理 enter-chat-room 命令
void enterChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount, fd_set* readfds);

// 处理列出所有聊天室命令
void listChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount);

// 处理关闭聊天室命令
void closeChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount);

// 处理 unknown command
void unknownCommand(int clientSocket, char* command);

// 从文件描述符集合中移除一个文件描述符
void removeClient(int clientSocket, fd_set* readfds, int* maxSocket);

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // 创建 socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址结构
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // 绑定地址
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding server address");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(serverSocket, 5) == -1) {
        perror("Error listening for incoming connections");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    fd_set readfds; // 文件描述符集合
    int maxSocket; // 最大文件描述符

    // 初始化文件描述符集合
    FD_ZERO(&readfds);
    FD_SET(serverSocket, &readfds);
    maxSocket = serverSocket;

    while (1) {
        fd_set tempfds = readfds; // 临时文件描述符集合，用于 select 函数修改后恢复原始集合

        // 使用 select 监听文件描述符的变化
        if (select(maxSocket + 1, &tempfds, NULL, NULL, NULL) == -1) {
            perror("Error in select");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        // 检查是否有新连接
        if (FD_ISSET(serverSocket, &tempfds)) {
            // 有新连接
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket == -1) {
                perror("Error accepting connection");
                close(serverSocket);
                exit(EXIT_FAILURE);
            }

            printf("New connection from %s on socket %d\n", inet_ntoa(clientAddr.sin_addr), clientSocket);

            // 将新连接添加到文件描述符集合
            FD_SET(clientSocket, &readfds);

            // 更新最大文件描述符
            if (clientSocket > maxSocket) {
                maxSocket = clientSocket;
            }
        }

        for (int i = 0; i <= maxSocket; ++i) {
            if (FD_ISSET(i, &tempfds)) {
                if (i == serverSocket) {
                    // 有新连接
                    // ... 其他代码
                } else {
                    // 处理已连接的客户端
                    char buffer[MAX_MESSAGE_LEN + 1];
                    memset(buffer, 0, sizeof(buffer));

                    // 接收客户端命令
                    ssize_t bytesRead = recv(i, buffer, sizeof(buffer), 0);

                    if (bytesRead <= 0) {
                        // 客户端断开连接
                        printf("Client on socket %d disconnected\n", i);
                        removeClient(i, &readfds, &maxSocket);
                    } else {
                        // 处理客户端命令
                        processCommand(i, buffer, users, &userCount, chatRooms, &chatRoomCount, &readfds);
                    }
                }
            }
        }
    }    

    return 0;
}
// 处理客户端命令
void processCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount, fd_set* readfds) {
    char* token = strtok(command, " ");
    if (token != NULL) {
        if (strcasecmp(token, "register") == 0) {
            registerUser(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "login") == 0) {
            loginUser(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "logout") == 0) {
            logoutUser(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "exit") == 0) {
            exitCommand(clientSocket, command, users, userCount, readfds);
        } else if (strcasecmp(token, "whoami") == 0) {
            whoamiCommand(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "set-status") == 0) {
            setStatusCommand(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "list-user") == 0) {
            listUserCommand(clientSocket, command, users, userCount);
        } else if (strcasecmp(token, "enter-chat-room") == 0) {
            enterChatRoomCommand(clientSocket, command, users, userCount, chatRooms, chatRoomCount, readfds);
        } else if (strcasecmp(token, "list-chat-room") == 0) {
            listChatRoomCommand(clientSocket, command, users, userCount, chatRooms, chatRoomCount);
        } else if (strcasecmp(token, "close-chat-room") == 0) {
            closeChatRoomCommand(clientSocket, command, users, userCount, chatRooms, chatRoomCount, readfds);
        } else {
            unknownCommand(clientSocket, command);
        }
    }

    // 这里的示例代码只是简单地将命令原样发送回客户端
    send(clientSocket, command, strlen(command), 0);
}

// 从文件描述符集合中移除一个文件描述符
void removeClient(int clientSocket, fd_set* readfds, int* maxSocket) {
    close(clientSocket);
    FD_CLR(clientSocket, readfds);

    // 更新最大文件描述符
    if (*maxSocket == clientSocket) {
        for (int i = *maxSocket - 1; i >= 0; --i) {
            if (FD_ISSET(i, readfds)) {
                *maxSocket = i;
                break;
            }
        }
    }
}

// 注册新用户
void registerUser(int clientSocket, char* command, User* users, int* userCount) {
    char* token = strtok(NULL, " "); // 获取用户名参数
    if (token != NULL) {
        // 检查用户名是否已存在
        bool usernameExists = false;
        for (int i = 0; i < *userCount; ++i) {
            if (strcasecmp(users[i].username, token) == 0) {
                usernameExists = true;
                break;
            }
        }

        if (usernameExists) {
            // 用户名已存在
            send(clientSocket, "Username is already used.\n", 27, 0);
        } else {
            // 获取密码参数
            token = strtok(NULL, " ");
            if (token != NULL) {
                // 创建新用户
                User newUser;
                strcpy(newUser.username, token);
                token = strtok(NULL, " ");
                strcpy(newUser.password, token);
                newUser.isLoggedIn = 0; // 初始状态为未登录
                strcpy(newUser.status, "offline");

                // 将新用户添加到数组中
                users[*userCount] = newUser;
                (*userCount)++;

                // 发送注册成功消息
                send(clientSocket, "Register successfully.\n", 23, 0);
            } else {
                // 缺少密码参数
                send(clientSocket, "Usage: register <username> <password>\n", 38, 0);
            }
        }
    } else {
        // 缺少用户名参数
        send(clientSocket, "Usage: register <username> <password>\n", 38, 0);
    }
}


// 处理登录命令
void loginUser(int clientSocket, char* command, User* users, int* userCount) {
    char* token = strtok(NULL, " "); // 获取用户名参数
    if (token != NULL) {
        // 查找用户
        int userIndex = -1;
        for (int i = 0; i < *userCount; ++i) {
            if (strcasecmp(users[i].username, token) == 0) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // 用户存在
            if (users[userIndex].isLoggedIn) {
                // 用户已经登录
                send(clientSocket, "Please logout first.\n", 22, 0);
            } else {
                // 获取密码参数
                token = strtok(NULL, " ");
                if (token != NULL) {
                    // 验证密码
                    if (strcasecmp(users[userIndex].password, token) == 0) {
                        // 登录成功
                        users[userIndex].isLoggedIn = 1;
                        send(clientSocket, "Welcome, ", 9, 0);
                        send(clientSocket, users[userIndex].username, strlen(users[userIndex].username), 0);
                        send(clientSocket, ".\n", 2, 0);
                    } else {
                        // 密码错误
                        send(clientSocket, "Login failed.\n", 15, 0);
                    }
                } else {
                    // 缺少密码参数
                    send(clientSocket, "Usage: login <username> <password>\n", 36, 0);
                }
            }
        } else {
            // 用户不存在
            send(clientSocket, "Login failed.\n", 15, 0);
        }
    } else {
        // 缺少用户名参数
        send(clientSocket, "Usage: login <username> <password>\n", 36, 0);
    }
}

// 处理登出命令
void logoutUser(int clientSocket, char* command, User* users, int* userCount) {
    char* token = strtok(NULL, " "); // 获取用户名参数
    if (token != NULL) {
        // 查找用户
        int userIndex = -1;
        for (int i = 0; i < *userCount; ++i) {
            if (strcasecmp(users[i].username, token) == 0) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // 用户存在
            if (users[userIndex].isLoggedIn) {
                // 登出成功
                users[userIndex].isLoggedIn = 0;
                send(clientSocket, "Bye, ", 5, 0);
                send(clientSocket, users[userIndex].username, strlen(users[userIndex].username), 0);
                send(clientSocket, ".\n", 2, 0);
            } else {
                // 用户未登录
                send(clientSocket, "Please login first.\n", 20, 0);
            }
        } else {
            // 用户不存在
            send(clientSocket, "Please login first.\n", 20, 0);
        }
    } else {
        // 缺少用户名参数
        send(clientSocket, "Usage: logout\n", 14, 0);
    }
}

// 处理退出命令
void exitCommand(int clientSocket, char* command, User* users, int* userCount, fd_set* readfds) {
    char* token = strtok(NULL, " "); // 获取用户名参数
    if (token != NULL) {
        // 查找用户
        int userIndex = -1;
        for (int i = 0; i < *userCount; ++i) {
            if (strcasecmp(users[i].username, token) == 0) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // 用户存在
            if (users[userIndex].isLoggedIn) {
                // 如果用户已登录，先执行登出操作
                logoutUser(clientSocket, command, users, userCount);
            }
            // 移除客户端并关闭连接
            removeClient(clientSocket, readfds, &maxSocket);
            printf("Client on socket %d exited\n", clientSocket);
        } else {
            // 用户不存在
            send(clientSocket, "Please login first.\n", 20, 0);
        }
    } else {
        // 缺少用户名参数
        send(clientSocket, "Usage: exit <username>\n", 23, 0);
    }
}

// 处理设置用户状态命令
void setStatusCommand(int clientSocket, char* command, User* users, int* userCount) {
    char* token = strtok(NULL, " "); // 获取用户名参数
    if (token != NULL) {
        // 查找用户
        int userIndex = -1;
        for (int i = 0; i < *userCount; ++i) {
            if (strcasecmp(users[i].username, token) == 0) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // 用户存在
            if (users[userIndex].isLoggedIn) {
                // 获取状态参数
                token = strtok(NULL, " ");
                if (token != NULL) {
                    // 设置用户状态
                    if (strcasecmp(token, "online") == 0 || strcasecmp(token, "offline") == 0 || strcasecmp(token, "busy") == 0) {
                        strcpy(users[userIndex].status, token);
                        // 发送状态设置成功消息
                        send(clientSocket, users[userIndex].username, strlen(users[userIndex].username), 0);
                        send(clientSocket, " ", 1, 0);
                        send(clientSocket, users[userIndex].status, strlen(users[userIndex].status), 0);
                        send(clientSocket, "\n", 1, 0);
                    } else {
                        // 非法状态
                        send(clientSocket, "set-status failed\n", 18, 0);
                    }
                } else {
                    // 缺少状态参数
                    send(clientSocket, "Usage: set-status <status>\n", 27, 0);
                }
            } else {
                // 用户未登录
                send(clientSocket, "Please login first.\n", 20, 0);
            }
        } else {
            // 用户不存在
            send(clientSocket, "Please login first.\n", 20, 0);
        }
    } else {
        // 缺少用户名参数
        send(clientSocket, "Usage: set-status <status>\n", 27, 0);
    }
}
// 处理列出所有用户命令
void listUserCommand(int clientSocket, char* command, User* users, int* userCount) {
    // 检查用户是否已登录
    int userIndex = -1;
    for (int i = 0; i < *userCount; ++i) {
        if (users[i].isLoggedIn && clientSocket == i + 4) {
            userIndex = i;
            break;
        }
    }

    if (userIndex != -1) {
        // 用户已登录，列出所有用户
        for (int i = 0; i < *userCount; ++i) {
            send(clientSocket, users[i].username, strlen(users[i].username), 0);
            send(clientSocket, " ", 1, 0);
            send(clientSocket, users[i].status, strlen(users[i].status), 0);
            send(clientSocket, "\n", 1, 0);
        }
    } else {
        // 用户未登录
        send(clientSocket, "Please login first.\n", 20, 0);
    }
}
// 处理进入聊天室命令
void enterChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount, fd_set* readfds) {
    char* token = strtok(NULL, " "); // 获取房间号参数
    if (token != NULL) {
        // 检查用户是否已登录
        int userIndex = -1;
        for (int i = 0; i < *userCount; ++i) {
            if (users[i].isLoggedIn && clientSocket == i + 4) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // 用户已登录
            int roomNumber = atoi(token);
            if (roomNumber >= 1 && roomNumber <= 100) {
                // 查找聊天室是否存在
                int roomIndex = -1;
                for (int i = 0; i < *chatRoomCount; ++i) {
                    if (chatRooms[i].roomNumber == roomNumber) {
                        roomIndex = i;
                        break;
                    }
                }

                if (roomIndex != -1) {
                    // 聊天室已存在，用户进入聊天室
                    send(clientSocket, "Welcome to the public chat room.\n", 33, 0);
                    send(clientSocket, "Room number: ", 13, 0);
                    send(clientSocket, token, strlen(token), 0);
                    send(clientSocket, "\nOwner: ", 8, 0);
                    send(clientSocket, chatRooms[roomIndex].owner, strlen(chatRooms[roomIndex].owner), 0);
                    send(clientSocket, "\n", 1, 0);

                    // 向聊天室所有用户发送进入消息
                    for (int i = 0; i < *userCount; ++i) {
                        if (users[i].isLoggedIn && users[i].roomNumber == roomNumber) {
                            send(i + 4, users[userIndex].username, strlen(users[userIndex].username), 0);
                            send(i + 4, " had entered the chat room.\n", 27, 0);
                        }
                    }

                    // 发送聊天室历史消息
                    // ...

                    // 设置用户所在聊天室
                    users[userIndex].roomNumber = roomNumber;
                } else {
                    // 聊天室不存在，创建新聊天室
                    // ...

                    // 进入聊天室
                    // ...
                }
            } else {
                // 房间号不合法
                send(clientSocket, "Number is not valid.\n", 21, 0);
            }
        } else {
            // 用户未登录
            send(clientSocket, "Please login first.\n", 20, 0);
        }
    } else {
        // 缺少房间号参数
        send(clientSocket, "Usage: enter-chat-room <number>\n", 31, 0);
    }
}

// 处理列出所有聊天室命令
void listChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount) {
    // 检查用户是否已登录
    int userIndex = -1;
    for (int i = 0; i < *userCount; ++i) {
        if (users[i].isLoggedIn && clientSocket == i + 4) {
            userIndex = i;
            break;
        }
    }

    if (userIndex != -1) {
        // 用户已登录，列出所有聊天室
        for (int i = 0; i < *chatRoomCount; ++i) {
            send(clientSocket, chatRooms[i].owner, strlen(chatRooms[i].owner), 0);
            send(clientSocket, " ", 1, 0);
            char roomNumberStr[4];
            snprintf(roomNumberStr, sizeof(roomNumberStr), "%d", chatRooms[i].roomNumber);
            send(clientSocket, roomNumberStr, strlen(roomNumberStr), 0);
            send(clientSocket, "\n", 1, 0);
        }
    } else {
        // 用户未登录
        send(clientSocket, "Please login first.\n", 20, 0);
    }
}

// 处理关闭聊天室命令
void closeChatRoomCommand(int clientSocket, char* command, User* users, int* userCount, ChatRoom* chatRooms, int* chatRoomCount) {
    // 检查用户是否已登录
    int userIndex = -1;
    for (int i = 0; i < *userCount; ++i) {
        if (users[i].isLoggedIn && clientSocket == i + 4) {
            userIndex = i;
            break;
        }
    }

    if (userIndex != -1) {
        // 用户已登录，检查用户是否为房间拥有者
        int roomNumber = users[userIndex].roomNumber;
        int roomIndex = -1;
        for (int i = 0; i < *chatRoomCount; ++i) {
            if (chatRooms[i].roomNumber == roomNumber) {
                roomIndex = i;
                break;
            }
        }

        if (roomIndex != -1 && strcmp(chatRooms[roomIndex].owner, users[userIndex].username) == 0) {
            // 用户是房间拥有者，关闭聊天室
            // ...

            // 向聊天室内所有用户发送关闭消息
            for (int i = 0; i < *userCount; ++i) {
                if (users[i].isLoggedIn && users[i].roomNumber == roomNumber) {
                    send(i + 4, "Chat room closed.\n", 18, 0);
                }
            }

            // 移除聊天室
            // ...

            // 更新用户房间信息
            users[userIndex].roomNumber = 0;
        } else {
            // 用户不是房间拥有者
            send(clientSocket, "Only the owner can close this chat room.\n", 41, 0);
        }
    } else {
        // 用户未登录
        send(clientSocket, "Please login first.\n", 20, 0);
    }
}

// 处理未知命令
void unknownCommand(int clientSocket, char* command) {
    send(clientSocket, "Error: Unknown command\n", 23, 0);
}
