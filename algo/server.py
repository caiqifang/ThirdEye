#!/usr/bin/python           # This is server.py file
import socket
import time, json, os, random, math, sys

area_path = '/../static/area.json'
set_path = '/../static/set.json'
data_path = '/../static/data.json'
host = 'localhost' # Get local machine name
port = 3838        # Reserve a port for your service.

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def sign(v):
    if v >= 0.0:
        return 1
    return -1

def calc_error(num_device, num_tag, dmatrix, point, i, obj):
    e = 0
    for d in range(num_tag, num_tag + num_device):
        cal_dis = calc_dis(obj['anchors_list'][d]['left'],
                         obj['anchors_list'][d]['top'], point[0], point[1])
        e = e + (cal_dis - dmatrix[i][d])**2
    return e

def find_point(result, status, point, i):
    p = point
    num_device = result['num_device']
    num_tag = result['num_tag']
    matrix = result['distance']
    error = calc_error(num_device, num_tag, matrix, p, i)
    step = 10
    cnt = 0
    while (error > 1):
        # choose x y to improve
        vector = (0,0)
        if random.random() > 0.5:
            vector = (step, 0)
        else:
            vector = (0, step)
        point_p = (p[0] + vector[0], p[1] + vector[1])
        # calculate gradiant
        err_change = (calc_error(num_device, num_tag, matrix, point_p, i) \
                            - error) / step
        # choose step size
        step = math.sqrt(abs(err_change))
        # update (x,y) point
        if vector[0] > 0:
            p = (p[0] - step*sign(err_change), p[1])
        else:
            p = (p[0], p[1] - step*sign(err_change))
        # update improve
        error = calc_error(num_device, num_tag, matrix, p, i)
        cnt = cnt +1
        if cnt > 100:
            break
    return p

def locate(result, status):
    num_device = result['num_device']
    num_tag = result['num_tag']
    point = []
    if status['last_point'] == None:
        #first start
        corner = (status['setting']['bound_left'], status['setting']['bound_top'])
        max_bound =(status['setting']['max_width'], status['setting']['max_height'])
        for i in num_tag:
            point.append((random.random()*max_bound[0] + corner[0],
                            random.random()*max_bound[1] + corner[1]))
    else:
        point = status['last_point']

    for i in num_tag:
        point[i] = find_point(result, status, point[i], i)

    status['last_point'] = point
    return point

# ============ FILE IO
def load_json(path):
    curr_path = os.getcwd()
    rd = open(curr_path + path, 'r')
    string = rd.read()
    rd.close()
    return json.loads(string)

def write_json(path, pts):
    # input: pts is a list of (x,y) tuple   [(x,y), (x,y)]
    curr_path = os.getcwd()
    wr = open(curr_path + path, 'w')
    data = {}
    for i in len(pts):
        data[i] = pts[i]
    wr.write(json.dumps(data))
    wr.close()
    return

# =====================  COMMAND PARSING  TODO
def parse(msg, status):
    result['num_device'] = None
    result['num_tag'] = None
    result['distance'] = None

    #parsing beacon to beacon
    scale = calc_dis(obj['anchors_list'][0]['left'],
                     obj['anchors_list'][0]['top'],
                     obj['anchors_list'][1]['left'],
                     obj['anchors_list'][1]['top']) / d_matrix[0][1]
        # get scale

    #parsing beacon to tag
        # get distance matrix

    #parsing reset
    valid = False
    ret = {}
    return valid, ret

def isInArea(status, points):
    # as for now, assume single tags
    area = status['area']
    height = area['height']*area['scaleY']
    width = area['scaleX']*area['width']
    y = area['top']
    x = area['left']


    return False

def reset(status):
    status['last_point'] = None
    status['scale'] = None
    status['setting'] = load_json(set_path)
    status['area'] = load_json(area_path)

def main():
    server = socket.socket()         # Create a socket object
    server.bind((host, port))        # Bind to the port
    server.listen(5)                 # Now wait for client connection.
    status = {}                      # system status
    reset(status)
    valid = False
    while True:
        try:
            c, addr = server.accept()     # Establish connection with client.
            print 'Got connection from ', addr, time.ctime()
            try:
                client_msg = c.recv(1024)
                while True:
                    # parse client message
                    valid, result = parse(client_msg, status)
                    # localization calculation
                    msg = ""
                    if valid:
                        ret = locate(result, status)
                        # write to data.json
                        write_json(data_path, ret)
                        # Detect Alert
                        if isInArea(status, ret):
                            msg = "1"
                    c.send(msg)
                    client_msg = c.recv(1024)
            except:
                print 'client socket and calculation error -  socket close'
                c.close()
        except:
            print 'Cant Accept Connection'
    # should never get here
    server.close()

if __name__ == '__main__':
    main()
