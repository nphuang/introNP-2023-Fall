#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>

using namespace std;

vector<pair<int, int> > preplaced_queens;

bool is_safe(vector<vector<int> >& board, int row, int col, int n) {
    for (int i = 0; i < n; ++i) {
        if (board[i][col] == 1 || board[row][i] == 1) {
            return false;
        }
    }

    for (int i = row, j = col; i >= 0 && j >= 0; --i, --j) {
        if (board[i][j] == 1) {
            return false;
        }
    }

    for (int i = row, j = col; i >= 0 && j < n; --i, ++j) {
        if (board[i][j] == 1) {
            return false;
        }
    }

    return true;
}

int count_conflicts(vector<vector<int> >& board, int n) {
    int conflicts = 0;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (board[i][j] == 1 && !is_safe(board, i, j, n)) {
                conflicts++;
            }
        }
    }

    return conflicts;
}

bool solve_nqueens(vector<vector<int> >& board, int n, int max_iter) {
    srand(time(0));

    int current_conflicts = count_conflicts(board, n);
    int temperature = 10000;
    double cooling_rate = 0.99;

    for (int iter = 0; iter < max_iter; ++iter) {
        int rand_row = rand() % n;
        int rand_col = rand() % n;

        int old_conflicts = current_conflicts;
        board[rand_row][rand_col] = 1 - board[rand_row][rand_col];
        int new_conflicts = count_conflicts(board, n);

        int delta = new_conflicts - old_conflicts;
        if (delta > 0 && exp(-delta / temperature) < ((double)rand() / RAND_MAX)) {
            // Revert the change if not accepted
            board[rand_row][rand_col] = 1 - board[rand_row][rand_col];
        } else {
            current_conflicts = new_conflicts;
        }

        temperature *= cooling_rate;

        if (current_conflicts == 0) {
            return true; // Found a solution
        }
    }

    return false;
}

void print_solution(vector<vector<int> >& board, vector<pair<int, int> >& preplaced_queens) {
    for (int i = 0; i < board.size(); ++i) {
        for (int j = 0; j < board[i].size(); ++j) {
            if (make_pair(i, j) == preplaced_queens[0] || make_pair(i, j) == preplaced_queens[1] || make_pair(i, j) == preplaced_queens[2]) {
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
    int n = 8;
    preplaced_queens.push_back(make_pair(3, 0));
    preplaced_queens.push_back(make_pair(7, 1));
    preplaced_queens.push_back(make_pair(0, 2));

    vector<vector<int> > chess_board(n, vector<int>(n, 0));

    // Initialize with preplaced queens
    for (const auto& queen : preplaced_queens) {
        chess_board[queen.first][queen.second] = 1;
    }

    // Solve N-Queens problem using simulated annealing
    if (solve_nqueens(chess_board, n, 1000000)) {
        print_solution(chess_board, preplaced_queens);
    } else {
        cout << "No solution found." << endl;
    }

    return 0;
}
