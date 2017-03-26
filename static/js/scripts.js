window.onload = function(){
    // create a wrapper around native canvas element
    var canvas = new fabric.Canvas('main');
    // var SelfReloadJSON = require('self-reload-json');

    // load json files
    var json_set = new Object();
    var json_data = new Object();

    // init
    json_data.x = 50;
    json_data.y = 50;

    // create a rectangle object - > your position
    var rect = new fabric.Rect({
        left: json_data.x,
        top: json_data.y,
        fill: 'red',
        width: 10,
        height: 10
    });

    // create anchors
    var tri1 = new fabric.Triangle({
        width: 10, height: 10, fill: 'blue', left: 50, top: 50
    });

    var tri2 = new fabric.Triangle({
        width: 10, height: 10, fill: 'blue', left: 750, top: 50
    });

    var tri3 = new fabric.Triangle({
        width: 10, height: 10, fill: 'blue', left: 50, top: 550
    });



    // create hazard area





    // "add"  elements onto canvas
    canvas.add(rect);
    canvas.add(tri1);
    canvas.add(tri2);
    canvas.add(tri3);

    // refresh
    setInterval(draw, 3000);

    // drawing function
    function draw(){
        // load
        loadJSON("../static/data.json", load_data);

        if(json_data.x > -1){
            // reset positions
            rect.set({ left: json_data.x, top: json_data.y });
        }
        canvas.renderAll()
    }

    // loading
    function loadJSON(file, callback){
      var xobj = new XMLHttpRequest();
      //xobj.overrideMimeType("application/json");
      xobj.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == "200") {
        // Required use of an anonymous callback as .open will NOT return a
        // //value but simply returns undefined in asynchronous mode
            callback(this);
        }
      };
      xobj.open("GET", file, true);
      xobj.send();
    }

    function load_data(obj){
        var json_real = JSON.parse(obj.responseText);
        json_data.x = json_real.x;
        json_data.y = json_real.y;
    }

}
