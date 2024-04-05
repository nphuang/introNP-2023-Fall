#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <set>
#include <cctype>
#include <deque>

using namespace std;
// Define user structure

class ChatHistory
{
private:
    deque<string> records;
    size_t maxRecords;
    string temp = "";

public:
    ChatHistory(size_t maxSize) : maxRecords(maxSize) {}

    void addRecord(const string &record)
    {
        records.push_back(record);

        // Keep only the latest maxRecords records
        while (records.size() > maxRecords)
        {
            temp = records.front();
            records.pop_front();
        }
    }
    void addPinMessage(const string &pinMessage)
    {
        // Remove any existing pin message
        removePinMessage();

        // Add the new pin message
        records.push_back(pinMessage);

        // Keep only the latest maxRecords records
        while (records.size() > maxRecords)
        {
            temp = records.front();
            records.pop_front();
        }
    }

    string getChatHistory() const
    {
        string history;
        int i = 0;
        for (const auto &record : records)
        {
            ++i;
            history += record;
        }
        if (i == 9)
            history += temp;
        return history;
    }
    void removePinMessage()
    {
        // Iterate through records and remove any pin messages
        auto it = remove_if(records.begin(), records.end(), [](const string &record)
                            { return record.find("Pin ->") != string::npos; });

        // Erase removed elements
        records.erase(it, records.end());
    }
};

vector<string> filterList = {"==", "superpie", "hello", "starburst stream", "domain expansion"};
string toLower(const string &str)
{
    string lowerStr = str;
    transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
              [](unsigned char c)
              { return tolower(c); });
    return lowerStr;
}

string filterMessage(string message)
{
    string lowerMessage = toLower(message);
    for (const auto &word : filterList)
    {
        // Find the start of the word in the message
        size_t startPos = lowerMessage.find(word);
        while (startPos != string::npos)
        {
            // Replace the word with asterisks in the original message
            message.replace(startPos, word.length(), word.length(), '*');
            // Find the next occurrence of the word
            startPos = lowerMessage.find(word, startPos + word.length());
        }
    }
    return message;
}
struct User
{
    int socket;
    string username = "";
    string password = "";
    string status = "offline";
    bool ano = false;
    int inChatRoom = 0; // 0: not in chat room, 1~100: in chat room1~100
    // Add other necessary information
};
struct info
{
    string password;
    string status;
    bool ano = false;
};
// Define chat room structure
struct ChatRoom
{
    string owner;
    set<string> members;
    // Set the pin message in the chat room.
    string pinMessage = ""; // at most 150 characters
    // chat history
    ChatHistory chatHistory = ChatHistory(10);
};
map<int, ChatRoom> chatRoomMap;
// size: 1000
vector<User> connected_users;

// Function to send a message to a user
void sendMessage(const User &user, const string &message)
{
    send(user.socket, message.c_str(), message.size(), 0);
}

// Function to handle registration
// unordered_set<string> usernames;
map<string, info> userCredentials;
// userCredentials.clear();
void handleRegister(User &user, const string &username, const string &password)
{
    // Check for existing username
    if (userCredentials.find(username) != userCredentials.end())
    {
        // Username is already used.
        sendMessage(user, "Username is already used.\n% ");
        return;
    }
    // Otherwise, register the user and send Register successfully.
    // user.username = username;
    // user.password = password;
    user.status = "offline";
    info newInfo;
    newInfo.password = password;
    newInfo.status = "offline";
    userCredentials[username] = newInfo;
    sendMessage(user, "Register successfully.\n% ");
}

// Function to handle login
void handleLogin(User &user, const string &username, const string &password)
{
    // Check if already logged in
    // If logged in, send response Please logout first.
    if (user.status != "offline")
    {
        // User is already logged in
        sendMessage(user, "Please logout first.\n% ");
        return;
    }
    // Check if username exists and password is correct
    // If not, send response Login failed.
    if (userCredentials.find(username) == userCredentials.end() || userCredentials[username].password != password || userCredentials[username].status != "offline")
    {
        // Username does not exist or password is incorrect
        sendMessage(user, "Login failed.\n% ");
        return;
    }
    // If successful, send Welcome, <username>.
    user.username = username;
    user.password = password;
    user.status = "online";
    userCredentials[username].status = "online";
    // Send login success message
    sendMessage(user, "Welcome, " + username + ".\n% ");
}

// Function to handle logout
void handleLogout(User &user)
{
    // Check if logged in
    if (user.status == "offline")
    {
        // User is not logged in
        sendMessage(user, "Please login first.\n");
        return;
    }

    // Logout the user and send Bye, <username>.
    sendMessage(user, "Bye, " + user.username + ".\n");
    user.status = "offline";
    userCredentials[user.username].status = "offline";
    user.username = "";
}

// Function to handle set-status
void handleSetStatus(User &user, const string &status)
{
    if (user.status == "offline")
    {
        sendMessage(user, "Please login first.\n% ");
        return;
    }

    // Check if status is valid (online, offline, busy)
    if (status != "online" && status != "offline" && status != "busy")
    {
        sendMessage(user, "set-status failed\n% ");
        return;
    }
    // Otherwise, set status and send <username> <status>.
    if (status == "offline")
    {
        sendMessage(user, user.username + " " + status + "\n% ");
        user.ano = true;
        userCredentials[user.username].ano = true;

        return;
    }
    else
    {
        user.status = status;
        user.ano = false;
        userCredentials[user.username].ano = false;
        userCredentials[user.username].status = status;
        sendMessage(user, user.username + " " + status + "\n% ");
    }
}

// Function to handle list-user
void handleListUser(User &user)
{
    // Check if logged in
    // If not, send response Please login first.
    // Sort users by username
    // Send the list of users and their status
    string userList = "";
    for (const auto &entry : userCredentials)
    {

        if (entry.second.ano == true)
        {
            // sendMessage(user, entry.first + " offline\n");
            userList += entry.first + " offline\n";
        }
        else
        {
            // sendMessage(user, entry.first + " " + entry.second.status + "\n");
            userList += entry.first + " " + entry.second.status + "\n";
        }
    }
    sendMessage(user, userList+"% ");
}

// Function to find a user by username
User &findUserByUsername(const string &username)
{
    // Find the user in connected_users
    auto userIt = find_if(connected_users.begin(), connected_users.end(), [username](const User &user)
                          { return user.username == username; });
    return *userIt;
}
// Function to handle enter-chat-room
void handleEnterChatRoom(User &user, int roomNumber)
{
    // Check if the chat room exists
    auto roomIt = chatRoomMap.find(roomNumber);
    if (roomIt == chatRoomMap.end())
    {
        // Create a new chat room if it doesn't exist
        ChatRoom newRoom;
        newRoom.owner = user.username;
        // newRoom.members.push_back(user.username);
        chatRoomMap[roomNumber] = newRoom;
        user.inChatRoom = roomNumber;
        sendMessage(user, "Welcome to the public chat room.\n");
        sendMessage(user, "Room number: " + to_string(roomNumber) + "\n");
        sendMessage(user, "Owner: " + user.username + "\n");
    }
    else
    {
        // Show the message to all clients in the chat room. <username> had enter the chat room.
        // roomIt->second.members.push_back(user.username);
        for (const auto &member : roomIt->second.members)
        {
            if (member != user.username)
            {
                sendMessage(findUserByUsername(member), user.username + " had enter the chat room.\n");
            }
        }
        user.inChatRoom = roomNumber;
        sendMessage(user, "Welcome to the public chat room.\n");
        sendMessage(user, "Room number: " + to_string(roomNumber) + "\n");
        sendMessage(user, "Owner: " + roomIt->second.owner + "\n");
        //<chat_history>
        sendMessage(user, roomIt->second.chatHistory.getChatHistory());
    }

    // Add the user to the chat room
    chatRoomMap[roomNumber].members.insert(user.username);
}

// Function to handle list-chat-room
void handleListChatRoom(User &user)
{
    // Implement list-chat-room logic
    string roomList;
    for (const auto &entry : chatRoomMap)
    {
        roomList += entry.second.owner + " " + to_string(entry.first) + "\n";
    }
    sendMessage(user, roomList+"% ");
}

// Function to handle close-chat-room
void handleCloseChatRoom(User &user, int roomNumber)
{
    // Check if the chat room exists
    auto roomIt = chatRoomMap.find(roomNumber);
    if (roomIt == chatRoomMap.end())
    {
        sendMessage(user, "Chat room " + to_string(roomNumber) + " does not exist.\n% ");
    }
    else
    {
        // Check if the user is the owner of the chat room
        if (roomIt->second.owner == user.username)
        {
            // Notify other users in the chat room
            for (const auto &member : roomIt->second.members)
            {
                // cout << "member: " << member << endl;
                sendMessage(findUserByUsername(member), "Chat room " + to_string(roomNumber) + " was closed.\n% ");
                // set inChatRoom to 0
                if (findUserByUsername(member).inChatRoom == roomNumber)
                {
                    findUserByUsername(member).inChatRoom = 0;
                }
                // cout << "username: " << findUserByUsername(member).socket << endl;
            }
            if (roomIt->second.members.find(roomIt->second.owner) == roomIt->second.members.end())
            {
                sendMessage(findUserByUsername(roomIt->second.owner), "Chat room " + to_string(roomNumber) + " was closed.\n% ");

                findUserByUsername(roomIt->second.owner).inChatRoom = 0;
            }
            // Remove the chat room
            chatRoomMap.erase(roomIt);
        }
        else
        {
            sendMessage(user, "Only the owner can close this chat room.\n% ");
        }
    }
}

// Main function
int main(int argc, char *argv[])
{
    // Check if port number is provided
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " [port number]" << endl;
        return -1;
    }
    int port = stoi(argv[1]);

    // Set up server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // cout<<serverSocket<<endl;
    if (serverSocket == -1)
    {
        cerr << "Error creating server socket." << endl;
        return -1;
    }
    // Set up server address struct
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Error binding server socket." << endl;
        close(serverSocket);
        return -1;
    }
    // Listen for incoming connections
    if (listen(serverSocket, 2000) == -1)
    {
        cerr << "Error listening for connections." << endl;
        close(serverSocket);
        return -1;
    }

    // Set up variables for select
    fd_set masterSet;
    FD_ZERO(&masterSet);
    FD_SET(serverSocket, &masterSet);
    int maxSocket = serverSocket;

    // Main server loop
    while (true)
    {
        
        fd_set readSet = masterSet;
        if (select(maxSocket + 1, &readSet, NULL, NULL, NULL) == -1)
        {
            cerr << "Error in select." << endl;
            close(serverSocket);
            return -1;
        }

        // Check for new connection
        if (FD_ISSET(serverSocket, &readSet))
        {
            // Accept new connection
            sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressSize);

            if (clientSocket == -1)
            {
                cerr << "Error accepting new connection." << endl;
            }
            else
            {
                // Add new client socket to master set
                FD_SET(clientSocket, &masterSet);
                maxSocket = max(maxSocket, clientSocket);

                // Send welcome message to the new client
                string welcomeMessage = "*********************************\n";
                welcomeMessage += "** Welcome to the Chat server. **\n";
                welcomeMessage += "*********************************\n";
                send(clientSocket, welcomeMessage.c_str(), welcomeMessage.size(), 0);
                send(clientSocket, "% ", 2, 0);
                // Create a new user and store the socket
                User newUser;
                newUser.socket = clientSocket;
                connected_users.push_back(newUser);
                // cout<<"new client socket: "<<clientSocket<<endl;

            }
        }
        
        // Handle existing connections
        for (int i = serverSocket + 1; i <= maxSocket; ++i)
        {
            
            if (FD_ISSET(i, &readSet))
            {
                // Find the user associated with the socket
                auto userIt = find_if(connected_users.begin(), connected_users.end(), [i](const User &user)
                                      { return user.socket == i; });
                if (userIt != connected_users.end())
                {
                    // Receive command from client and handle it

                    if (userIt->inChatRoom)
                    {
                        char buffer[1024];
                        ssize_t bytesRead = recv(i, buffer, sizeof(buffer) - 1, 0);
                        buffer[bytesRead] = '\0';
                        string command(buffer);
                        
                        if (command.substr(0, 5) == "/pin ")
                        {
                            string message = command.substr(5);
                            size_t endPos = message.find_last_not_of("\n\r");
                            if (endPos != string::npos) // remove \n\r
                            {
                                message = message.substr(0, endPos + 1);
                            }
                            message = filterMessage(message);
                            if (message.length() <= 150)
                            {
                                // Set the pin message in the chat room
                                string pinMessage = "Pin -> [" + userIt->username + "]: " + message + "\n";
                                // we need to remove the old pin message from chat history

                                chatRoomMap[userIt->inChatRoom].chatHistory.addPinMessage(pinMessage);
                                chatRoomMap[userIt->inChatRoom].pinMessage = pinMessage;
                                // Send Pin -> [<username>]: <message>\n to all users in the chatroom
                                for (const auto &member : chatRoomMap[userIt->inChatRoom].members)
                                {
                                    if (findUserByUsername(member).inChatRoom == userIt->inChatRoom)
                                    {
                                        sendMessage(findUserByUsername(member), pinMessage);
                                    }
                                }
                            }
                        }
                        else if (command == "/delete-pin\n")
                        {
                            if (!chatRoomMap[userIt->inChatRoom].pinMessage.empty())
                            {
                                // Delete the pin message in the chat room
                                chatRoomMap[userIt->inChatRoom].pinMessage = "";
                                chatRoomMap[userIt->inChatRoom].chatHistory.removePinMessage();
                            }
                            else
                            {
                                // If there is no pin message in the chat room
                                string response = "No pin message in chat room " + to_string(userIt->inChatRoom) + "\n";
                                send(userIt->socket, response.c_str(), response.size(), 0);
                            }
                        }
                        else if (command == "/exit-chat-room\n")
                        {
                            // Switch to the chat server mode
                            // Send the message <username> had left the chat room to all clients in the chat room
                            string exitMessage = userIt->username + " had left the chat room.\n";
                            // cout << "exitMessage: " << exitMessage << endl;
                            for (const auto &member : chatRoomMap[userIt->inChatRoom].members)
                            {
                                // cout << "member: " << member << endl;
                                // cout << "findUserByUsername(member).inChatRoom: " << findUserByUsername(member).inChatRoom << endl;
                                if (findUserByUsername(member).inChatRoom == userIt->inChatRoom)
                                {
                                    // cout << "member: " << member << endl;
                                    if (member != userIt->username)
                                    {
                                        sendMessage(findUserByUsername(member), exitMessage);
                                    }
                                    else send(findUserByUsername(member).socket, "% ", 2, 0);
                                }
                            }
                            chatRoomMap[userIt->inChatRoom].members.erase(userIt->username);
                            // cout<<"HERE\n";
                            userIt->inChatRoom = 0;
                        }
                        else if (command == "/list-user\n")
                        {
                            // List all users in this chat room
                            string userList;
                            for (const auto &member : chatRoomMap[userIt->inChatRoom].members)
                            {
                                //<username> <status>
                                if (userCredentials[member].ano == true)
                                {
                                    userList += member + " offline\n";
                                }
                                else
                                {
                                    userList += member + " " + userCredentials[member].status + "\n";
                                }
                            }
                            send(userIt->socket, userList.c_str(), userList.size(), 0);
                        }
                        else if (command[0] == '/')
                        {
                            sendMessage(*userIt, "Error: Unknown command\n");
                        }
                        else
                        {

                            command = filterMessage(command);
                            string chatMessage = "[" + userIt->username + "]: " + command;
                            // Send [<username>]: <message>\n to all users in the chatroom.
                            chatRoomMap[userIt->inChatRoom].chatHistory.addRecord(chatMessage);
                            for (const auto &member : chatRoomMap[userIt->inChatRoom].members)
                            {
                                if (findUserByUsername(member).inChatRoom == userIt->inChatRoom)
                                {
                                    sendMessage(findUserByUsername(member), chatMessage);
                                }
                            }
                        }
                    }
                    else
                    {
                        //send % to client when waiting client input command
                        char buffer[1024];
                        ssize_t bytesRead = recv(i, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead <= 0)
                        {
                            // Handle disconnection
                            close(i);
                            FD_CLR(i, &masterSet);
                            auto credentialsIt = userCredentials.find(userIt->username);
                            if (credentialsIt != userCredentials.end())
                            {
                                // Update user status to "offline"
                                if (userIt->status != "offline")
                                {
                                    userIt->status = "offline";
                                    // cout << "User: " << userIt->username << " disconnect." << endl;
                                    credentialsIt->second.status = "offline";
                                }
                                // cout << "socket:" << userIt->socket << " is erased." << endl;
                                connected_users.erase(userIt);
                            }
                            else
                            {
                                // cout << "socket:" << userIt->socket << " is erased." << endl;
                                connected_users.erase(userIt);
                                // cout<<connected_users.size()<<endl;
                            }
                        }
                        else
                        {
                            buffer[bytesRead] = '\0';
                            string command(buffer);
                            // Parse command and extract necessary information
                            // Call corresponding handler functions based on the command
                            
                            if (command.substr(0, 8) == "register")
                            {
                                if (command[8] == ' ')
                                {
                                    // Extract username and password
                                    size_t spacePos = command.find(' ', 9);
                                    if (spacePos != string::npos)
                                    {
                                        // Extract username and password
                                        string username = command.substr(9, spacePos - 9);
                                        string password = command.substr(spacePos + 1);
                                        size_t endPos = password.find_last_not_of("\n\r");
                                        if (endPos != string::npos)
                                        {
                                            password = password.substr(0, endPos + 1);
                                        }
                                        if (username.length() > 20 || username.length() < 1 || password.length() > 20 || password.length() < 1 || password == " " || password == "\r" || password == "\n")
                                        {
                                            sendMessage(*userIt, "Usage: register <username> <password>\n% ");
                                        }
                                        else
                                        {
                                            // cout << "reg request: " << username << " " << password << "%" << endl;
                                            size_t extraSpacePos = password.find(' ');
                                            if (extraSpacePos == string::npos)
                                            {
                                                handleRegister(*userIt, username, password);
                                            }
                                            else
                                            {
                                                // Invalid command format (extra space)
                                                sendMessage(*userIt, "Usage: register <username> <password>\n% ");
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Invalid command format
                                        sendMessage(*userIt, "Usage: register <username> <password>\n% ");
                                    }
                                }
                                else
                                {
                                    // Invalid command format
                                    sendMessage(*userIt, "Usage: register <username> <password>\n% ");
                                }
                            }
                            else if (command.substr(0, 5) == "login")
                            {
                                // Extract username and password
                                if (command[5] == ' ')
                                {
                                    // Extract username and password
                                    size_t spacePos = command.find(' ', 6);
                                    if (spacePos != string::npos)
                                    {
                                        // Extract username and password
                                        string username = command.substr(6, spacePos - 6);
                                        string password = command.substr(spacePos + 1);
                                        size_t endPos = password.find_last_not_of("\n\r");
                                        if (endPos != string::npos)
                                        {
                                            password = password.substr(0, endPos + 1);
                                        }
                                        if (username.length() > 20 || username.length() < 1 || password.length() > 20 || password.length() < 1 || password == " " || password == "\r" || password == "\n")
                                        {
                                            sendMessage(*userIt, "Usage: login <username> <password>\n% ");
                                        }
                                        else
                                        {
                                            // cout << "log request: " << username << " " << password << "%" << endl;
                                            size_t extraSpacePos = password.find(' ');
                                            if (extraSpacePos == string::npos)
                                            {
                                                handleLogin(*userIt, username, password);
                                            }
                                            else
                                            {
                                                // Invalid command format (extra space)
                                                sendMessage(*userIt, "Usage: login <username> <password>\n% ");
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Invalid command format
                                        sendMessage(*userIt, "Usage: login <username> <password>\n% ");
                                    }
                                }
                                else
                                {
                                    // Invalid command format
                                    sendMessage(*userIt, "Usage: login <username> <password>\n% ");
                                }
                            }
                            else if (command.substr(0, 6) == "logout")
                            {
                                if (command[6] != '\n')
                                {
                                    sendMessage(*userIt, "Usage: logout\n% ");
                                }
                                else
                                {
                                    handleLogout(*userIt);
                                    sendMessage(*userIt, "% ");
                                }
                            }
                            else if (command.substr(0, 4) == "exit")
                            {
                                if (command[4] != '\n')
                                {
                                    sendMessage(*userIt, "Usage: exit\n% ");
                                }
                                else
                                {
                                    if (userIt->status != "offline")
                                    {
                                        handleLogout(*userIt);
                                    }
                                    // Close the connection
                                    close(i);
                                    FD_CLR(i, &masterSet);
                                    // cout << "socket:" << userIt->socket << " is erased." << endl;
                                    connected_users.erase(userIt);
                                }
                            }
                            else if (command.substr(0, 6) == "whoami")
                            {
                                if (command[6] != '\n')
                                {
                                    sendMessage(*userIt, "Usage: whoami\n% ");
                                }
                                else
                                {
                                    if (userIt->status != "offline")
                                    {
                                        sendMessage(*userIt, userIt->username + "\n% ");
                                    }
                                    else
                                    {
                                        sendMessage(*userIt, "Please login first.\n% ");
                                    }
                                }
                            }
                            else if (command.substr(0, 10) == "set-status")
                            {
                                // only one space after %set-status
                                string status = command.substr(10);
                                // Check for missing or redundant parameter
                                if (status.empty() || status[0] != ' ' or status.find(' ', 1) != string::npos || status[0] == '\n')
                                {
                                    sendMessage(*userIt, "Usage: set-status <status>\n% ");
                                    // cout << "fail set-status request: " << status << "%" << endl;
                                }
                                else
                                {
                                    // Call handleSetStatus function
                                    size_t endPos = status.find_last_not_of("\n\r");
                                    if (endPos != string::npos)
                                    {
                                        status = status.substr(1, endPos);
                                    }
                                    // cout << "set-status request: " << status << "%" << endl;
                                    handleSetStatus(*userIt, status.substr(0)); // Ignore the leading space
                                }
                            }
                            else if (command.substr(0, 9) == "list-user")
                            {
                                if (command[9] != '\n')
                                {
                                    sendMessage(*userIt, "Usage: list-user\n% ");
                                }
                                else
                                {
                                    if (userIt->status != "offline")
                                    {
                                        handleListUser(*userIt);
                                    }
                                    else
                                    {
                                        sendMessage(*userIt, "Please login first.\n% ");
                                    }
                                }
                            }
                            else if (command.substr(0, 15) == "enter-chat-room")
                            {
                                // only one space after %enter-chat-room
                                string roomNumber = command.substr(15);
                                // Check for missing or redundant parameter
                                if (roomNumber.empty() || roomNumber[0] != ' ' or roomNumber.find(' ', 1) != string::npos || roomNumber[0] == '\n')
                                {
                                    sendMessage(*userIt, "Usage: enter-chat-room <number>\n% ");
                                    // cout << "fail enter-chat-room request: " << roomNumber << "%" << endl;
                                }
                                else
                                {
                                    // Call handleEnterChatRoom function
                                    size_t endPos = roomNumber.find_last_not_of("\n\r");
                                    if (endPos != string::npos)
                                    {
                                        roomNumber = roomNumber.substr(1, endPos);
                                    }
                                    // cout << "enter-chat-room request: " << roomNumber << "%" << endl;
                                    // If <number> is not a valid room number(not between 1 to 100) Number <number> is not valid.
                                    if (stoi(roomNumber) < 1 || stoi(roomNumber) > 100)
                                    {
                                        sendMessage(*userIt, "Number " + roomNumber + " is not valid.\n% ");
                                        continue;
                                    }
                                    if (userIt->status == "offline")
                                    {
                                        sendMessage(*userIt, "Please login first.\n% ");
                                        continue;
                                    }
                                    handleEnterChatRoom(*userIt, stoi(roomNumber)); // Ignore the leading space
                                }
                            }
                            else if (command.substr(0, 14) == "list-chat-room")
                            {
                                if (command[14] != '\n')
                                {
                                    sendMessage(*userIt, "Usage: list-chat-room\n% ");
                                }
                                else
                                {
                                    if (userIt->status != "offline")
                                    {
                                        handleListChatRoom(*userIt);
                                    }
                                    else
                                    {
                                        sendMessage(*userIt, "Please login first.\n% ");
                                    }
                                }
                            }
                            else if (command.substr(0, 15) == "close-chat-room")
                            {
                                // only one space after %close-chat-room
                                string roomNumber = command.substr(15);
                                // Check for missing or redundant parameter
                                if (roomNumber.empty() || roomNumber[0] != ' ' or roomNumber.find(' ', 1) != string::npos || roomNumber[0] == '\n')
                                {
                                    sendMessage(*userIt, "Usage: close-chat-room <number>\n% ");
                                    // cout << "fail close-chat-room request: " << roomNumber << "%" << endl;
                                }
                                else
                                {
                                    if (userIt->status == "offline")
                                    {
                                        sendMessage(*userIt, "Please login first.\n% ");
                                        continue;
                                    }
                                    // Call handleCloseChatRoom function
                                    size_t endPos = roomNumber.find_last_not_of("\n\r");
                                    if (endPos != string::npos)
                                    {
                                        roomNumber = roomNumber.substr(1, endPos);
                                    }
                                    // cout << "close-chat-room request: " << roomNumber << "%" << endl;
                                    handleCloseChatRoom(*userIt, stoi(roomNumber)); // Ignore the leading space
                                }
                            }
                            else
                            {
                                // Unknown command
                                sendMessage(*userIt, "Error: Unknown command\n% ");
                            }

                        }
                    }
                }
            }
        }
    }

    return 0;
}
