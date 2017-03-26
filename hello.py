from flask import *
app = Flask(__name__)

@app.route("/", methods=['GET', 'POST'])
def hello(name=None):
    if request.method == "POST":
        flash('New entry was successfully')

    return render_template("index.html",name=name)

if __name__ == "__main__":
    app.run()
