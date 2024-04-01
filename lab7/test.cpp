#include <iostream>
#include <vector>
#include <unordered_set>

using namespace std;

// N 皇后問題的求解函數
bool solveNQueens(vector<vector<int> >& board, int row, int n, unordered_set<int>& col_set, unordered_set<int>& pre_row_set, unordered_set<int>& diag_set, unordered_set<int>& anti_diag_set) {
    if (row == n) {
        return true;
    }

    // 如果這一行有預設皇后，直接跳到下一行
    if (pre_row_set.find(row) != pre_row_set.end()) {
        return solveNQueens(board, row + 1, n, col_set,pre_row_set, diag_set, anti_diag_set);
    }

    for (int col = 0; col < n; ++col) {
        if (col_set.find(col) == col_set.end() && diag_set.find(row + col) == diag_set.end() && anti_diag_set.find(row - col) == anti_diag_set.end()) {
            board[row][col] = 1;

            col_set.insert(col);
            diag_set.insert(row + col);
            anti_diag_set.insert(row - col);

            if (solveNQueens(board, row + 1, n, col_set,pre_row_set, diag_set, anti_diag_set)) {
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

// 打印解法
void printSolution(const vector<vector<int> >& board, const vector<pair<int, int> >& preplaced_queens) {
    for (int i = 0; i < board.size(); ++i) {
        for (int j = 0; j < board[i].size(); ++j) {
            if (find(preplaced_queens.begin(), preplaced_queens.end(), make_pair(i, j)) != preplaced_queens.end()) {
                cout << "\033[91mQ\033[0m ";
            } else if (board[i][j] == 1) {
                cout << "\033[94mQ\033[0m ";
            } else {
                cout << "0 ";
            }
        }
        cout << endl;
    }
}

int main() {
    int n = 27;

    // 初始化棋盤
    vector<vector<int> > chess_board(n, vector<int>(n, 0));

    // 初始化集合
    unordered_set<int> pre_row_set;
    unordered_set<int> col_set;
    unordered_set<int> diag_set;
    unordered_set<int> anti_diag_set;

    // 預設皇后位置
    vector<pair<int, int> > preplaced_queens;
    preplaced_queens.push_back(make_pair(3, 0));
    preplaced_queens.push_back(make_pair(7, 1));
    preplaced_queens.push_back(make_pair(0, 2));

    // 放置預設皇后
    for (const auto& queen : preplaced_queens) {
        int row = queen.first;
        int col = queen.second;
        chess_board[row][col] = 1;
        pre_row_set.insert(row);
        col_set.insert(col);
        diag_set.insert(row + col);
        anti_diag_set.insert(row - col);
    }

    // 解 N 皇后問題
    if (solveNQueens(chess_board, 0, n, col_set, pre_row_set, diag_set, anti_diag_set)) {
        printSolution(chess_board, preplaced_queens);
    } else {
        cout << "No solution found." << endl;
    }

    return 0;
}
