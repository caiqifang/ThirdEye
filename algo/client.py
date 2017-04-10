import socket               # Import socket module
import time, json, os, math, random
import numpy as np

num_device = 4 #global
set_path = '/../static/set.json'
data_path = '/../static/data.json'

def load_json(path):
    curr_path = os.getcwd()
    rd = open(curr_path + path, 'r')
    string = rd.read()
    rd.close()
    return json.loads(string)

def write_json(path, msg):
    curr_path = os.getcwd()
    wr = open(curr_path + path, 'w')
    data = {'x': msg[0], 'y': msg[1]}
    wr.write(json.dumps(data))
    wr.close()
    print 'write to data'
    return

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def calc_error(num_device, matrix, obj, ini):
    err = sum(abs(matrix[num_device - 1]))
    s = 0
    for i in range(num_device -1):  # if more tags,  change HERE!
        s = s + calc_dis(obj['anchors_list'][i]['left'],
                obj['anchors_list'][i]['top'], ini[0], ini[1])
        return abs(s - err)

def locate(d_matrix, obj):
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
    while (error > 2):
        # choose x y to improve
        vector = (0,0)
        if random.random() > 0.5:
            vector = (5, 0)
        else:
            vector = (0, 5)
        ini_p = (ini[0] + vector[0], ini[1] + vector[1])
        # calculate gradiant
        err_change = (calc_error(num_device, matrix, obj, ini_p) - error)
        # choose step size
        step = 2 #2 pixel step
        # update (x,y) point
        if vector[0] > 0:
            ini = (ini[0] - step*np.sign(err_change), ini[1])
        else:
            ini = (ini[0], ini[1] - step*np.sign(err_change))
        # update improve
        error = calc_error(num_device, matrix, obj, ini)
        print 'in loop debug'
    return ini, error

def main():
    while True:
        s = socket.socket()         # Create a socket object
        host = 'localhost' # Get local machine name
        port = 3838                # Reserve a port for your service.
        s.connect((host, port))
        s.send(str(num_device))
        matrix = np.fromstring(s.recv(1024))
        d_matrix = matrix.reshape((num_device, num_device))
        s.close()
        # localization calculation ==========
        print d_matrix
        obj = load_json(set_path)
        ret, err =  locate(d_matrix, obj)        # return (x, y) pixal location
        print ret
        print err
        # write to data.json
        write_json(data_path, ret)
        # delay
        time.sleep(1)

if __name__ == '__main__':
    main()
