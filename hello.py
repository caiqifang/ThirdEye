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
    return "Record New Data"

@app.route("/set", methods=['GET', 'POST'])
def new_set():
    if request.method == 'POST':
        curr_path = os.getcwd()
        j = json.loads(request.data)
        data_path = "/static/set.json"
        wr = open(curr_path + data_path, 'w')
        wr.write(json.dump(j))
    return "Record New Data"


if __name__ == "__main__":
    app.run()
