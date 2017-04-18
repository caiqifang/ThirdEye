#!/usr/bin/python           # This is server.py file
import socket
import time, json, os, random, math, sys
import numpy as np

area_path = '/../static/area.json'
set_path = '/../static/set.json'
data_path = '/../static/data.json'
host = 'localhost' # Get local machine name
port = 3838        # Reserve a port for your service.

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def calc_error(num_device, num_tag, dmatrix, setting, point):
    e = 0
    for i in range(num_device -1):  # if more tags,  change HERE!
        cal_dis = calc_dis(obj['anchors_list'][i]['left'],
                         obj['anchors_list'][i]['top'], ini[0], ini[1])
        e = e + (cal_dis - matrix[num_device-1][i])**2
    return e

def locate(result, setting, status):
    scale = calc_dis(obj['anchors_list'][0]['left'],
                     obj['anchors_list'][0]['top'],
                     obj['anchors_list'][1]['left'],
                     obj['anchors_list'][1]['top']) / d_matrix[0][1]
    matrix = d_matrix * scale
    corner = (obj['bound_left'], obj['bound_top'])
    max_bound = (obj['max_width'], obj['max_height'])
    ini = (random.random()*max_bound[0] + corner[0],
            random.random()*max_bound[1] + corner[1])
    error = calc_error(num_device, matrix, obj, ini)
    step = 10
    cnt = 0
    while (error > 1):
        # choose x y to improve
        vector = (0,0)
        if random.random() > 0.5:
            vector = (step, 0)
            #print 'x'
        else:
            vector = (0, step)
            #print 'y'
        ini_p = (ini[0] + vector[0], ini[1] + vector[1])
        # calculate gradiant
        err_change = (calc_error(num_device, matrix, obj, ini_p) - error) / step
        #print 'err_change', err_change
        # choose step size
        step = math.sqrt(abs(err_change)) #2 pixel step
        #print 'step', step
        # update (x,y) point
        if vector[0] > 0:
            ini = (ini[0] - step*np.sign(err_change), ini[1])
        else:
            ini = (ini[0], ini[1] - step*np.sign(err_change))
        # update improve
        #print 'point', ini
        error = calc_error(num_device, matrix, obj, ini)
        #print 'error', error
        cnt = cnt +1
        if cnt > 150:
            break
    # return (x, y) pixal location
    return ini, error

# ============ FILE IO
def load_json(path):
    curr_path = os.getcwd()
    rd = open(curr_path + path, 'r')
    string = rd.read()
    rd.close()
    return json.loads(string)

def write_json(path, msg):
    # input: msg is a (x,y) tuple
    curr_path = os.getcwd()
    wr = open(curr_path + path, 'w')
    data = {'x': msg[0], 'y': msg[1]}
    wr.write(json.dumps(data))
    wr.close()
    return

# =====================  COMMAND PARSING
def parse(msg, status):
    ret = {}
    return ret

def reset(status):
    status['num_beacon'] = None
    status['num_device'] = None
    status['last_point'] = None
    status['scale'] = None
    status['reset'] = True
    status['setting'] = None
    status['area'] = None


def main():
    server = socket.socket()         # Create a socket object
    server.bind((host, port))        # Bind to the port
    server.listen(5)                 # Now wait for client connection.
    status = {}                      # system status
    reset(status)
    valid = False
    while True:
        c, addr = server.accept()     # Establish connection with client.
        print 'Got connection from ', addr, time.ctime()
        client_msg = c.recv(1024)
        # ============ parse client message
        valid, result = parse(client_msg, status)
        # localization calculation ==========
        if status['reset']:
            status['setting'] = load_json(set_path) # load setting
            status['reset'] = False
        if valid:
            #ret, err =  locate(result, status)
            pass
        # ================ write to data.json
        # write_json(data_path, ret)
        # ============== Detect Hazard Entry
        msg = ""
        #if isInArea(status, ret):
            # msg = "SERVER REPLY ALERT"
        c.send(msg)
        c.close()
    # should never get here
    server.close()

if __name__ == '__main__':
    main()
