import socket               # Import socket module
import time, json, os, math, random
import numpy as np

def main():
    server = socket.socket()         # Create a socket object
    host = 'localhost' # Get local machine name
    port = 1024            # Reserve a port for your service.
    server.bind((host, port))        # Bind to the port
    server.listen(5)                 # Now wait for client connection.
    num_device = 4
    while True:
        c, addr = server.accept()     # Establish connection with client.
        print '==============  DRIVE ==============', addr
        client_msg = c.recv(1024)
        print 'Message:' , client_msg
        num_device = int(client_msg)
        # pull distance information
        msg = 'new_matrix'
        c.send(msg)
        c.close()
    # should never get here
    server.close()

if __name__ == '__main__':
    main()
