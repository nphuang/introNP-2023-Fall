import socket
import subprocess
import threading
import time
import os

def send_receive_message(server_address, message):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
        client_socket.sendto(message.encode(), server_address)
        response, _ = client_socket.recvfrom(1024)
        print(response.decode())
        return response.decode()

def chals(server_address, message):
    socket.setdefaulttimeout(5)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.sendto(message.encode(), server_address)
    client_socket.settimeout(5)
    while True:
        try:
            client_socket.recv(1024)
        except socket.timeout:
            return

def tcpdump():
    tcpdump_process = subprocess.Popen(["tcpdump", "-ni", "any", "-Xxnv", "udp", "and", "port", "10495", "-w", "challenge.pcap"])
    time.sleep(5)
    tcpdump_process.kill()

def process_packets():
    in_file = 'challenge.pcap'
    out_file = 'packet'
    os.system(f'tshark -r {in_file} > {out_file}.txt')

    with open('packet.txt', 'r') as file:
        lines = file.readlines()
    results = []
    target = False
    for line in lines[3:]:
        parts = line.split()
        packet_length = int(parts[-1].split('=')[-1])    
        if packet_length < 30:
            target = not target
        if target == True and packet_length > 30:
            udp_port = int(parts[6])
            results.append(udp_port - 48)

    flag = "".join(map(chr, results))
    return flag

def main():
    server_address = ('127.0.0.1', 10495)
    id = input("Input ID: ")

    tcpdump_thread = threading.Thread(target=tcpdump)
    tcpdump_thread.start()
    time.sleep(1)

    OK_id = send_receive_message(server_address, f'hello {id}')
    challenge_id = OK_id.split()[-1]
    print(f"challenge ID: {challenge_id}")

    chals(server_address, f'chals {challenge_id}')

    flag = process_packets()
    print(flag)

    send_receive_message(server_address, f'verfy {flag}')

if __name__ == "__main__":
    main()
# sudo python3 demo4.py