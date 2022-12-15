#!/usr/bin/env python

import sys
import socket

client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)  # UDP

# Uncomment this if you plan to broadcast from this script
#client.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

# Set the socket to non blocking, allows the program to continue and not wait for data the whole time
client.setblocking(1)

# Bind to all interfaces and to port 2255
client.bind(('', 2255))

def check_data():
    try:
        # Data received
        data, addr = client.recvfrom(1024)
        #print("received message: %s from %s" % (data,addr))

        # return the decoded bytes as a string
        return data.decode()
    # If no data is received just return None
    except socket.error:
        return None

def main():
    # Main loop
    while True:
        # Check for UDP data
        line = check_data()
        # If there is data split it and print it to console
        if line:
            split_line = line.split('|')
            print(split_line)

        # Continue with main loop
        # print("...")

if __name__ == '__main__':
    try:
        main()
    # CTRL + C pressed so exit gracefully
    except KeyboardInterrupt:
        print('Interrupted.')
        sys.exit()