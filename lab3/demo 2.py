import socket
from queue import Queue
global last_direction
last_direction = ''
def connect_to_server(host, port):
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((host, port))
        return client_socket
    except ConnectionRefusedError:
        print("Error: Connection to the server failed.")
        exit(1)

def read_server_response(client_socket):
    response = ""
    bingo_received = False

    while True:
        data = client_socket.recv(1024).decode()
        response += data

        if "Enter your move(s)>" in response:
            break
        if "BINGO!" in response:
            # Set a flag to indicate that "BINGO!" has been received
            bingo_received = True
            break

    # If "BINGO!" was received, continue reading until a newline character is encountered
    if bingo_received:
        while True:
            data = client_socket.recv(1024).decode()
            response += data
            if '\n' in data:
                break

    return response

def extract_maze(response): # for 1 and 2
    lines = response.split('\n')
    maze = [list(line) for line in lines[9:-3]]
    return maze
def extract_visible_maze_3(response):
    lines = response.split('\n')
    maze_lines = lines[1:-2]
    visible_maze = []
    for line in maze_lines:
        parts = line.split(": ")
        if len(parts) == 2:
            row_number, content = parts
            try:
                row_number = int(row_number.strip())
                content = content.strip()
                content_characters = list(content)
                visible_maze.append([row_number] + content_characters)
            except ValueError:
                continue

    return visible_maze
def extract_visible_maze_4(response):
    lines = response.split('\n')
    maze_lines = lines[1:-2]
    visible_maze = []
    for line in maze_lines:
        parts = line.split(": ")
        if len(parts) == 2:
            row_number, content = parts
            try:
                row_number = int(row_number.strip())
                content = content.strip()
                content_characters = list(content)
                visible_maze.append(content_characters)
            except ValueError:
                continue

    return visible_maze

def find_start_exit(maze): # for 1/2/3
    for i in range(len(maze)):
        for j in range(len(maze[i])):
            if maze[i][j] == '*':
                start = (i, j)
            elif maze[i][j] == 'E':
                exit = (i, j)
    return start, exit
def solve_maze_bfs(maze, start, exit): # for 1/2/3
    q = Queue()
    q.put((start, ""))
    visited = set()

    while not q.empty():
        (x, y), path = q.get()
        visited.add((x, y))

        if (x, y) == exit:
            return path

        for dx, dy, direction in [(0, -1, 'A'), (0, 1, 'D'), (-1, 0, 'W'), (1, 0, 'S')]:
            new_x, new_y = x + dx, y + dy
            if 0 <= new_x < len(maze) and 0 <= new_y < len(maze[0]) and maze[new_x][new_y] != '#' and (new_x, new_y) not in visited:
                q.put(((new_x, new_y), path + direction))

    return None



def find_start(visible_maze):
    if visible_maze[3][5] == '*': # try center position
        return 3, 5

    for row_number, row_content in enumerate(visible_maze):
        if '*' in row_content:
            row = row_number
            col = row_content.index('*')
            return row, col
    return None

def move_directions(last_direction):
    if last_direction == 'W':
        return ['D', 'W', 'A', 'S']
    elif last_direction == 'D':
        return ['S', 'D', 'W', 'A']
    elif last_direction == 'S':
        return ['A', 'S', 'D', 'W']
    elif last_direction == 'A':
        return ['W', 'A', 'S', 'D']
def find_path_with_strategy(visible_maze, start): # 4
    direction = [(0, 1), (1, 0), (0, -1), (-1, 0)]
    global last_direction
    x, y = start
    next_move = ''
    if last_direction:
        move_list = move_directions(last_direction)
        for move in move_list:
            if move == 'W':
                dx, dy = (-1, 0)
            elif move == 'S':
                dx, dy = (1, 0)
            elif move == 'A':
                dx, dy = (0, -1)
            elif move == 'D':
                dx, dy = (0, 1)
            new_x, new_y = x + dx, y + dy
            if( 0 <= new_x < len(visible_maze) and
            0 <= new_y < len(visible_maze[3]) and
            visible_maze[new_x][new_y] != '#'):
                last_direction = move
                return move
    else: # first move
        for dx, dy in direction:
            new_x, new_y = x + dx, y + dy
            if (0 <= new_x < len(visible_maze) and
                0 <= new_y < len(visible_maze[3]) and
                visible_maze[new_x][new_y] != '#'):
                move = ['D', 'S', 'A', 'W'][direction.index((dx, dy))]
                last_direction = move
                return move
def solve_maze_limited_view(client_socket, visible_maze, start): #4
    #use dfs or bfs to create path to leave the visible maze
    while True:
        move = find_path_with_strategy(visible_maze, start)
        #send path to server
        if move is not None:
            # Send the path to the server
            print(move)
            move += "\n"
            client_socket.send(move.encode())
            response = read_server_response(client_socket)
            if "BINGO!" in response:
                print(response)
                return
            # print(response)
            visible_maze = extract_visible_maze_4(response)
            # print(visible_maze)
            start = find_start(visible_maze)
        else:
            print("Error: No path found.")
            return




def main():
    host = 'inp.zoolab.org'
    port_id = input("1/2/3/4:")
    if port_id == "1":
        port = 10301
    elif port_id == "2":
        port = 10302
    elif port_id == "3":
        port = 10303
    else: port = 10304
    if port_id == "1" or port_id =="2": 
        client_socket = connect_to_server(host, port)
        response = read_server_response(client_socket)
        print(response)

        maze = extract_maze(response)
        # print(maze)
        start, exit = find_start_exit(maze)
        path = solve_maze_bfs(maze, start, exit)
        path+='\n'
        if path:
            print(path)
            path+="\n"
            client_socket.send(path.encode())
            response = read_server_response(client_socket)
            print(response)
        else:
            print("Error: No path found.")

        client_socket.close()
    elif port_id == "3":
        client_socket = connect_to_server(host, port)
        response = read_server_response(client_socket)
        print(response)
        client_socket.send("\n".encode())
        response = read_server_response(client_socket)
        print(response)
        visible_maze = extract_visible_maze_3(response)
        # print(visible_maze)
        # move to top of maze
        while visible_maze[0][0] < 0: 
            client_socket.send("K\n".encode())
            response = read_server_response(client_socket)  
            # print(response)
            visible_maze = extract_visible_maze_3(response)
        while visible_maze[0][0] > 0: 
            client_socket.send("I\n".encode())
            response = read_server_response(client_socket)  
            # print(response)
            visible_maze = extract_visible_maze_3(response)
        # move to left of maze
        while len(visible_maze[0]) < 12:
            client_socket.send("L\n".encode())
            response = read_server_response(client_socket) 
            visible_maze = extract_visible_maze_3(response)
        
        while len(visible_maze[0])  == 12:
            client_socket.send("J\n".encode())
            response = read_server_response(client_socket)  
            # print(response)
            visible_maze = extract_visible_maze_3(response)
        client_socket.send("L\n".encode())
        response = read_server_response(client_socket) 
        # print(response)
        visible_maze = extract_visible_maze_3(response)
        # start to press I_J_K_L to build 101*101 maze array
        complete_maze = [[' ' for _ in range(101)] for _ in range(101)]
        
        x, y = 0, 0
        while True:
            while y <= 88:
                for i in range(7):
                    for j in range(11):
                        complete_maze[x + i][y + j] = visible_maze[i][j+1]
                y+=11
                client_socket.send("LLLLLLLLLLL\n".encode())
                response = read_server_response(client_socket) 
                visible_maze = extract_visible_maze_3(response)
            for i in range(7):
                for j in range(2):
                    complete_maze[x + i][y + j] = visible_maze[i][j+1]
            for i in range(9):
                client_socket.send("JJJJJJJJJJJ\n".encode())
                response = read_server_response(client_socket) 
            client_socket.send("KKKKKKK\n".encode())
            response = read_server_response(client_socket) 
            visible_maze = extract_visible_maze_3(response)
            # print(visible_maze)
            y = 0
            x+=7
            if x == 98: # last 3 rows
                while y <= 88:
                    for i in range(3):
                        for j in range(11):
                            complete_maze[x + i][y + j] = visible_maze[i][j+1]
                    y+=11
                    client_socket.send("LLLLLLLLLLL\n".encode())
                    response = read_server_response(client_socket) 
                    visible_maze = extract_visible_maze_3(response)
                for i in range(3):
                    for j in range(2):
                        complete_maze[x + i][y + j] = visible_maze[i][j+1]
                break
        

        start, exit = find_start_exit(complete_maze)
        path = solve_maze_bfs(complete_maze, start, exit)
        path+='\n'
        client_socket.send(path.encode())
        response = read_server_response(client_socket)
        print(response)
        client_socket.close()

    else:
        client_socket = connect_to_server(host, port)

        response = read_server_response(client_socket)
        print(response)
        client_socket.send("\n".encode())
        response = read_server_response(client_socket)
        print(response)
        visible_maze = extract_visible_maze_4(response)
        start = find_start(visible_maze)
        solve_maze_limited_view(client_socket, visible_maze, start)

        client_socket.close()


if __name__ == '__main__':
    main()
