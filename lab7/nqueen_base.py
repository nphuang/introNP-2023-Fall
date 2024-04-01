n = 27
# 三個預設皇后的位置
preplaced_queens = [(3, 0), (7, 1), (0, 2)]

def is_safe(board, row, col, n):
    # 檢查同一直排是否有皇后
    for i in range(n):
        if board[i][col] == 1:
            return False

    # 檢查左上到右下對角線是否有皇后
    for i, j in zip(range(row, -1, -1), range(col, -1, -1)):
        if board[i][j] == 1:
            return False

    # 檢查右上到左下對角線是否有皇后
    for i, j in zip(range(row, -1, -1), range(col, n)):
        if board[i][j] == 1:
            return False

    return True

def solve_nqueens(board, row, n):
    if row == n:
        # 所有皇后都已經放置，找到一個解
        return True
    
    if row in [x[0] for x in preplaced_queens]:
        # 如果這一行有預設皇后，直接跳到下一行
        return solve_nqueens(board, row + 1, n)

    for col in range(n):
        if is_safe(board, row, col, n):
            board[row][col] = 1

            # 遞歸放置下一行的皇后
            if solve_nqueens(board, row + 1, n):
                return True

            # 如果不能找到解，回溯
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
chess_board = [[0] * n for _ in range(n)]

# 放置預設皇后
for queen in preplaced_queens:
    row, col = queen
    chess_board[row][col] = 1

# 解N-Queens問題
if solve_nqueens(chess_board, 0, n):
    print_solution(chess_board, preplaced_queens)
else:
    print("No solution found.")
