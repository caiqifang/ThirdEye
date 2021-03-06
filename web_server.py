from flask import *
import json, os
app = Flask(__name__)

@app.route("/")
def index(name=None):
    return render_template("index.html",name=name)

@app.route("/data", methods=['GET', 'POST'])
def new_data():
    if request.method == 'POST':
        curr_path = os.getcwd()
        j = json.loads(request.data)
        data_path = "/static/data.json"
        wr = open(curr_path + data_path, 'w')
        wr.write(json.dumps(j))
        print json.dumps(j)
        wr.close()
    return "Record New Data"

@app.route("/set", methods=['GET', 'POST'])
def new_set():
    if request.method == 'POST':
        curr_path = os.getcwd()
        j = json.loads(request.data)
        data_path = "/static/set.json"
        wr = open(curr_path + data_path, 'w')
        wr.write(json.dumps(j))
        print json.dumps(j)
        wr.close()
    return "Record New set"

@app.route("/area", methods=['GET', 'POST'])
def new_area():
    if request.method == 'POST':
        curr_path = os.getcwd()
        j = json.loads(request.data)
        data_path = "/static/area.json"
        wr = open(curr_path + data_path, 'w')
        wr.write(json.dumps(j))
        print json.dumps(j)
        wr.close()
    return "Record new area"

if __name__ == "__main__":
    app.run()
