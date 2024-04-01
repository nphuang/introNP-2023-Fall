def solve_nqueens(board, row, n, col_set, diag_set, anti_diag_set):
    if row == n:
        return True
    
    if row in [x[0] for x in preplaced_queens]:
        return solve_nqueens(board, row + 1, n, col_set, diag_set, anti_diag_set)

    for col in range(n):
        if col not in col_set and row + col not in diag_set and row - col not in anti_diag_set:
            board[row][col] = 1

            col_set.add(col)
            diag_set.add(row + col)
            anti_diag_set.add(row - col)

            if solve_nqueens(board, row + 1, n, col_set, diag_set, anti_diag_set):
                return True

            board[row][col] = 0

            col_set.remove(col)
            diag_set.remove(row + col)
            anti_diag_set.remove(row - col)

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

# 初始化集合
col_set = set()
diag_set = set()
anti_diag_set = set()
preplaced_queens = [(3, 0), (9, 10), (26, 13)]

# 放置預設皇后
for queen in preplaced_queens:
    row, col = queen
    chess_board[row][col] = 1
    col_set.add(col)
    diag_set.add(row + col)
    anti_diag_set.add(row - col)

# 解N-Queens問題
if solve_nqueens(chess_board, 0, n, col_set, diag_set, anti_diag_set):
    print_solution(chess_board, preplaced_queens)
else:
    print("No solution found.")
