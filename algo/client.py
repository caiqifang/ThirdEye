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
    return

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def calc_error(num_device, matrix, obj, ini):
    e = 0
    for i in range(num_device -1):  # if more tags,  change HERE!
        cal_dis = calc_dis(obj['anchors_list'][i]['left'],
                         obj['anchors_list'][i]['top'], ini[0], ini[1])
        e = e + (cal_dis - matrix[num_device-1][i])**2
    return e
'''
def calc_error(num_device, matrix, obj, ini):
    err = sum((matrix[num_device - 1]) ** 2)
    print 'err in calc' , err
    s = 0
    for i in range(num_device -1):  # if more tags,  change HERE!
        cal_dis = (calc_dis(obj['anchors_list'][i]['left'],
                         obj['anchors_list'][i]['top'], ini[0], ini[1]))**2
        print cal_dis
        s = s + cal_dis
    return abs(s - err)
'''

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
    #print ini
    error = calc_error(num_device, matrix, obj, ini)
    step = 10
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
        #print d_matrix
        obj = load_json(set_path)
        ret, err =  locate(d_matrix, obj)        # return (x, y) pixal location
        print ret
        print err
        # write to data.json
        write_json(data_path, ret)
        # delay
        time.sleep(0.5)

if __name__ == '__main__':
    main()
