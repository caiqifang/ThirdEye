#!/usr/bin/python           # This is server.py file
import socket               # Import socket module
import time, json, os, random, math
import numpy as np

set_path = '/../static/set.json'
data_path = '/../static/data.json'
start = False
point = ()

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def load_json(path):
    curr_path = os.getcwd()
    rd = open(curr_path + path, 'r')
    string = rd.read()
    rd.close()
    return json.loads(string)
'''
def write_json(path, msg):
    curr_path = os.getcwd()
    wr = open(curr_path + path, 'w')
    data = {'x': msg[0], 'y': msg[1]}
    wr.write(json.dumps(data))
    wr.close()
    print 'write to data'
    return
'''
def generate_matrix(num_device):
    global start
    global point
    obj = load_json(set_path)
    corner = (obj['bound_left'], obj['bound_top'])
    max_bound = (obj['max_width'], obj['max_height'])

    if (not start):
        point = (random.random()*max_bound[0] + corner[0],
                random.random()*max_bound[1] + corner[1])
        start = True
    else:
        point = (corner[0] + (point[0] - corner[0] + 1) % max_bound[0],
                corner[1] + (point[1] - corner[1] + 1) % max_bound[1])
    print point
    matrix = np.zeros(num_device * num_device)
    matrix = matrix.reshape(num_device, num_device)
    for i in range(num_device-1):
        print i, 'beacons:', (obj['anchors_list'][i]['left'],
                                obj['anchors_list'][i]['top'])
    for i in range(num_device-1):
        for j in range(num_device-1):
            matrix[i][j] = calc_dis(obj['anchors_list'][i]['left'],
                    obj['anchors_list'][i]['top'],
                    obj['anchors_list'][j]['left'],
                    obj['anchors_list'][j]['top'])
    for i in range(num_device - 1):
        dis = calc_dis(obj['anchors_list'][i]['left'],
                        obj['anchors_list'][i]['top'],
                        point[0], point[1])
        matrix[i][num_device-1] = dis
        matrix[num_device -1][i] = dis
    return matrix

def main():
    server = socket.socket()         # Create a socket object
    host = 'localhost' # Get local machine name
    port = 3838            # Reserve a port for your service.
    server.bind((host, port))        # Bind to the port
    server.listen(5)                 # Now wait for client connection.
    num_device = 4
    while True:
        c, addr = server.accept()     # Establish connection with client.
        print 'Got connection from', addr
        client_msg = c.recv(1024)
        print 'Message:' , client_msg
        num_device = int(client_msg)
        # pull distance information
        #m = '0 1 2 1.1547;1 0 1.7321 0.57735; \
                #2 1.7321 0 1.1547;1.1547 0.57735 1.1547 0'
        # random point
        matrix = generate_matrix(num_device) #np.matrix(m)
        matrix = matrix.astype(float)
        call_driver(num_device)
        print matrix
        msg = matrix.tostring()
        c.send(msg)
        c.close()
    # should never get here
    server.close()

def call_driver(num_device):
    client = socket.socket()
    host = 'localhost'
    port = 1024
    client.connect((host,port))
    client.send(str(num_device))
    msg = client.recv(1024)
    client.close()
    return msg

if __name__ == '__main__':
    main()
