preplaced_queens = [(3, 0), (9, 10), (26, 13)]

def solve_nqueens(board, row, n, col_mask, diag_mask, anti_diag_mask):
    if row == n:
        return True

    if row in [x[0] for x in preplaced_queens]:
        return solve_nqueens(board, row + 1, n, col_mask, diag_mask, anti_diag_mask)

    for col in range(n):
        if (col_mask & (1 << col)) == 0 and (diag_mask & (1 << (row + col))) == 0 and (anti_diag_mask & (1 << (row - col + n - 1))) == 0:
            board[row][col] = 1

            new_col_mask = col_mask | (1 << col)
            new_diag_mask = diag_mask | (1 << (row + col))
            new_anti_diag_mask = anti_diag_mask | (1 << (row - col + n - 1))

            if solve_nqueens(board, row + 1, n, new_col_mask, new_diag_mask, new_anti_diag_mask):
                return True

            board[row][col] = 0

    return False

def print_solution(board, preplaced_queens):
    for i, row in enumerate(board):
        for j, cell in enumerate(row):
            if (i, j) in preplaced_queens:
                print("\033[91mQ\033[0m", end=" ")  # 使用 ANSI escape code 標記為紅色
            elif cell == 1:
                print("\033[94mQ\033[0m", end=" ")  # 使用 ANSI escape code 標記為藍色
            else:
                print("0", end=" ")
        print()

# 初始化30*30的棋盤
n = 30
chess_board = [[0] * n for _ in range(n)]

# 初始化位遮罩
initial_col_mask = 0
initial_diag_mask = 0
initial_anti_diag_mask = 0

# 放置預設皇后
for queen in preplaced_queens:
    row, col = queen
    chess_board[row][col] = 1
    initial_col_mask |= (1 << col)
    initial_diag_mask |= (1 << (row + col))
    initial_anti_diag_mask |= (1 << (row - col + n - 1))

# 解N-Queens問題
if solve_nqueens(chess_board, 0, n, initial_col_mask, initial_diag_mask, initial_anti_diag_mask):
    print_solution(chess_board, preplaced_queens)
else:
    print("No solution found.")
