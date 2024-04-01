#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <unordered_set>

using namespace std;
// Function to connect to the UNIX domain socket
int connectToServer()
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_un server_address;
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, "/queen.sock", sizeof(server_address.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Function to send a command to the server and receive the response
string sendCommand(int sockfd, const string &command)
{
    if (send(sockfd, command.c_str(), command.size(), 0) == -1)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }

    char buffer[2048]; // Adjust the buffer size as needed
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    return string(buffer, bytes_received);
}

string send_S_Command(int sockfd, const string &command)
{
    if (send(sockfd, command.c_str(), command.size(), 0) == -1)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }
    char buffer[2048]; // Adjust the buffer size as needed
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
    string response(buffer, bytes_received);

    // Remove the "OK:" prefix
    size_t okPos = response.find("OK: ");
    if (okPos != string::npos)
    {
        response.erase(okPos, 4); // Remove "OK:"
    }
    return response;
}

// Function to solve the N-Queen problem and return the optimal placement
bool hasConflict(const pair<int, int> &cell, const vector<pair<int, int>> &queens)
{
    for (const auto &queen : queens)
    {
        if (cell.first == queen.first || cell.second == queen.second ||
            abs(cell.first - queen.first) == abs(cell.second - queen.second))
        {
            return true; // Conflict found
        }
    }
    return false; // No conflict
}
// void solveNQueens(int sockfd, string &puzzle)
// {
//     // Find the existing queens on the board
//     vector<pair<int, int> > existingQueens;
//     int cnt = 0;
//     for (int row = 0; row < 30; ++row)
//     {
//         for (int col = 0; col < 30; ++col)
//         {
//             // cout<<puzzle[row * 30 + col];
//             if (puzzle[row * 30 + col] == 'Q')
//             {
//                 existingQueens.push_back({row, col});
//                 // cout<<"rol: "<<row<<"  col: "<<col<<'\n';
//                 ++cnt;
//             }
//         }
//     }
//     cout << "predefined_cnt: " << cnt << '\n';
//     // Iterate through each row and find an empty cell to place a queen
//     for (int row = 0; row < 30; ++row)
//     {
//         for (int col = 0; col < 30; ++col)
//         {
//             pair<int, int> emptyCell = {row, col};

//             // Check if the cell is empty and not conflicting with existing queens
//             if (puzzle[row * 30 + col] == '.' && !hasConflict(emptyCell, existingQueens))
//             {
//                 // Place the queen in the puzzle
//                 string command = "M " + to_string(emptyCell.first) + " " + to_string(emptyCell.second);
//                 string response = sendCommand(sockfd, command);
//                 // cout << response << endl;

//                 // Update the puzzle after placing the queen
//                 puzzle = send_S_Command(sockfd, "S");

//                 // If there is no conflict, add the queen to the list
//                 if (response.find("OK") != string::npos)
//                 {
//                     existingQueens.push_back(emptyCell); // Update existing queens
//                     ++cnt;
//                     break; // Move to the next row after placing a queen
//                 }
//             }
//         }
//     }
//     // Print the updated puzzle after all moves
//     cout << "result_cnt: " << cnt << '\n';

//     string result = sendCommand(sockfd, "P");
//     cout << "Updated Puzzle:\n"
//          << result << endl;
// }

bool solveNQueens(vector<vector<int>> &board, int row, int n, unordered_set<int> &col_set, unordered_set<int> &pre_row_set, unordered_set<int> &diag_set, unordered_set<int> &anti_diag_set)
{
    if (row == n)
    {
        return true;
    }

    // 如果這一行有預設皇后，直接跳到下一行
    if (pre_row_set.find(row) != pre_row_set.end())
    {
        return solveNQueens(board, row + 1, n, col_set, pre_row_set, diag_set, anti_diag_set);
    }

    for (int col = 0; col < n; ++col)
    {
        if (col_set.find(col) == col_set.end() && diag_set.find(row + col) == diag_set.end() && anti_diag_set.find(row - col) == anti_diag_set.end())
        {
            board[row][col] = 1;

            col_set.insert(col);
            diag_set.insert(row + col);
            anti_diag_set.insert(row - col);

            if (solveNQueens(board, row + 1, n, col_set, pre_row_set, diag_set, anti_diag_set))
            {
                return true;
            }

            board[row][col] = 0;

            col_set.erase(col);
            diag_set.erase(row + col);
            anti_diag_set.erase(row - col);
        }
    }

    return false;
}

void printSolution(const vector<vector<int>> &board, const vector<pair<int, int>> &preplaced_queens)
{
    for (int i = 0; i < board.size(); ++i)
    {
        for (int j = 0; j < board[i].size(); ++j)
        {
            if (find(preplaced_queens.begin(), preplaced_queens.end(), make_pair(i, j)) != preplaced_queens.end())
            {
                cout << "\033[91mQ\033[0m ";
            }
            else if (board[i][j] == 1)
            {
                cout << "\033[94mQ\033[0m ";
            }
            else
            {
                cout << "0 ";
            }
        }
        cout << endl;
    }
}

int main()
{
    // Connect to the server
    int sockfd = connectToServer();
    int n = 30;
    // Initial read of the puzzle
    string initialPuzzle = send_S_Command(sockfd, "S");
    // cout << "Initial Puzzle:\n" << initialPuzzle << endl;
    vector<vector<int>> chess_board(n, vector<int>(n, 0));
    unordered_set<int> pre_row_set;
    unordered_set<int> col_set;
    unordered_set<int> diag_set;
    unordered_set<int> anti_diag_set;
    vector<pair<int, int>> preplaced_queens;
    for (int row = 0; row < 30; ++row)
    {
        for (int col = 0; col < 30; ++col)
        {
            if (initialPuzzle[30 * row + col] == 'Q')
            {
                chess_board[row][col] = 1;
                preplaced_queens.push_back(make_pair(row, col));
            }
        }
    }
    for (const auto &queen : preplaced_queens)
    {
        int row = queen.first;
        int col = queen.second;
        pre_row_set.insert(row);
        col_set.insert(col);
        diag_set.insert(row + col);
        anti_diag_set.insert(row - col);
    }
    if (solveNQueens(chess_board, 0, n, col_set, pre_row_set, diag_set, anti_diag_set))
    {
        printSolution(chess_board, preplaced_queens);
        for (int row = 0; row < 30; ++row)
        {
            for (int col = 0; col < 30; ++col)
            {
                if (chess_board[row][col] != '0')
                {
                    preplaced_queens.push_back(make_pair(row, col));
                }
            }
        }
    }
    else
    {
        cout << "No solution found." << endl;
    }
    for (const auto &queen : preplaced_queens)
    {
        string command = "M " + to_string(queen.first) + " " + to_string(queen.second);
        string response = sendCommand(sockfd, command);

    }

    // Solve the N-Queen problem and get the optimal placement
    // solveNQueens(sockfd, initialPuzzle);

    // Check if the puzzle is correct
    string checkResponse = sendCommand(sockfd, "C");
    cout << checkResponse << endl;

    // Close the socket
    close(sockfd);

    return 0;
}
