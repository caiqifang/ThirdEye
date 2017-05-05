#!/usr/bin/python           # This is server.py file
import socket
import time, json, os, random, math, sys

area_path = '/../static/area.json'
set_path = '/../static/set.json'
data_path = '/../static/data.json'
host = 'localhost' # Get local machine name
port = 3838        # Reserve a port for your service.
alert_dic = 3.0    # tag to tag distance alert

def calc_dis(x1, y1, x2, y2):
    ans = math.pow(x1 - x2, 2) + math.pow(y1 - y2, 2)
    return math.sqrt(ans)

def sign(v):
    if v >= 0.0:
        return 1
    return -1

def calc_error(distance, point, key, obj):
    e = 0
    for beacon in distance[key].keys():
        cal_dis = calc_dis(obj['anchors_list'][beacon]['left'],
                         obj['anchors_list'][beacon]['top'], point[0], point[1])
        e = e + (cal_dis - distance[key][beacon])**2
    return e

def find_point(result, status, point, key):
    p = point
    distance = result['distance']
    error = calc_error(distance, p, key, status['setting'])
    step = 10.0
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
        err_change = (calc_error(distance, point_p, key, status['setting']) \
                            - error) / step
        # choose step size
        step = math.sqrt(abs(err_change))
        # update (x,y) point
        if vector[0] > 0:
            p = (p[0] - step*sign(err_change), p[1])
        else:
            p = (p[0], p[1] - step*sign(err_change))
        # update improve
        error = calc_error(distance, p, key, status['setting'])
        cnt = cnt +1
        if cnt > 100:
            break
    return p

def locate(result, status):
    points = {}
    point = ()
    if status['last_point'] == None:
        #first start
        corner = (status['setting']['bound_left'], status['setting']['bound_top'])
        max_bound =(status['setting']['max_width'], status['setting']['max_height'])
        point = ((random.random()*max_bound[0] + corner[0],
                            random.random()*max_bound[1] + corner[1]))
    else:
        point = status['last_point']

    for key in result['distance'].keys():
        ret = find_point(result, status, point, key)
        points[key] = {}
        points[key]['x'] = ret[0]
        points[key]['y'] = ret[1]
        status['last_point'] = ret

    return points

# ============ FILE IO
def load_json(path):
    curr_path = os.getcwd()
    rd = open(curr_path + path, 'r')
    string = rd.read()
    rd.close()
    return json.loads(string)

def write_json(path, pts):
    curr_path = os.getcwd()
    wr = open(curr_path + path, 'w')
    wr.write(json.dumps(pts))
    wr.close()
    return

def parse(msg, status, beacon_dis):
    valid = True
    obj = status['setting']
    result = {}
    result['num_device'] = None
    result['num_tag'] = None
    result['distance'] = {}
    list_str = msg.split(',')
    for string in list_str:
        data = string.split(':')
        if int(data[0]) == 777:
            result['num_device'] = int(data[3])
            result['num_tag'] = int(data[2])
            if status['scale'] == None:
                #parsing beacon to beacon
                scale = calc_dis(obj['anchors_list']['3']['left'],
                     obj['anchors_list']['3']['top'],
                     obj['anchors_list']['5']['left'],
                     obj['anchors_list']['5']['top']) / beacon_dis
                # get scale
                status['scale'] = scale

            elif int(data[1]) == 5:
                if status['scale'] != None:
                    #parsing beacon to tag
                    tag = int(data[4])
                    beacon = int(data[5])
                    tag = max(tag, beacon)
                    beacon = min(tag, beacon)
                    result['distance'][str(tag)] = {}
                    result['distance'][str(tag)][str(beacon)] = float(data[6]) * status['scale']
                else:
                    valid = False
                    print 'no scale data! ERROR'
            else:
                valid = False          #system reset
    #parsing validating
    if valid:
        if len(result['distance'].keys()) != result['num_tag']:
            valid = False
            print 'not valid data'
        else:
            for key in result['distance'].keys():
                if len(result['distance'][key].keys()) != result['num_device']:
                    valid = False
                    print 'not valid data'
    return valid, result

def isInArea(status, points):
    msg = ""
    area = status['area']
    height = area['height']*area['scaleY']
    width = area['scaleX']*area['width']
    top = area['top']
    left = area['left']
    keys = points.keys()
    for i in points:
        dis_x = points[i]['x']
        dis_y = points[i]['y']
        msg += i
        msg += ':'
        if (dis_x >= left and dis_x <= (left+width)
                and dis_y >= top and dis_y <= (top+height)):
            msg += '1'
        else:
            msg += '0'
        msg += ','
    return msg[:-1]
# "ID:value,ID:value"

def reset(status):
    status['last_point'] = None
    status['scale'] = None
    status['setting'] = load_json(set_path)
    status['area'] = load_json(area_path)

def main(argv):
    server = socket.socket()         # Create a socket object
    server.bind((host, port))        # Bind to the port
    server.listen(5)                 # Now wait for client connection.
    status = {}                      # system status
    reset(status)
    valid = False
    beacon_dis = float(argv[0])
    while True:
        try:
            c, addr = server.accept()     # Establish connection with client.
            print 'Got connection from ', addr, time.ctime()
            try:
                client_msg = c.recv(1024)
                while True:
                    print 'Server: client_msg:\n', client_msg
                    # parse client message
                    valid, result = parse(client_msg[1:], status, beacon_dis)
                    # localization calculation
                    msg = "6:0"
                    if valid:
                        ret = locate(result, status)
                        # write to data.json
                        write_json(data_path, ret)
                        # Detect Alert
                        msg = isInArea(status, ret)
                    else:
                        print 'Server: invalid information'
                    c.send(msg)
                    print 'Server: waiting new message'
                    client_msg = c.recv(1024)
            except:
                print 'client socket and calculation error -  socket close'
                c.close()
        except:
            print 'Cant Accept Connection'
    # should never get here
    server.close()

if __name__ == '__main__':
    main(sys.argv[1:])
