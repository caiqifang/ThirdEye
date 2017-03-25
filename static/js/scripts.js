window.onload = function(){
    // create a wrapper around native canvas element
    var canvas = new fabric.Canvas('main');
    var SelfReloadJSON = require('self-reload-json');

    // load json files
    var json_set = new Object();

    var json_data = new SelfReloadJSON("../static/data.json");

    // create a rectangle object - > your position
    var rect = new fabric.Rect({
        left: 100,
        top: 100,
        fill: 'red',
        width: 10,
        height: 10
    });

    // create anchors



    // create hazard area





    // "add"  elements onto canvas
    canvas.add(rect);

    // refresh
    setInterval(draw, 5000);

    // drawing function
    function draw(){
        if(json_data.x > -1){
            // reset positions
            rect.set({ left: json_data.x, top: json_data.y });
        }
        canvas.renderAll()
    }

    // loading
    function loadJSON(file, callback){
      var xobj = new XMLHttpRequest();
      xobj.overrideMimeType("application/json");
      xobj.open('GET', file, true);
      xobj.onreadystatechange = function () {
        if (xobj.readyState == 4 && xobj.status == "200") {
        // Required use of an anonymous callback as .open will NOT return a
        // //value but simply returns undefined in asynchronous mode
            callback(xobj.responseText);
        }
      };
      xobj.send(null);
    }

    function load(file, result){
        loadJSON(file, function(response){
            var json_real = JSON.parse(response);
            result.x = json_real.x;
            result.y = json_real.y;
        });
    }

}
