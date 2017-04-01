window.onload = function(){
    // create a wrapper around native canvas element
    var canvas = new fabric.Canvas('main');

    // load json files
    var json_set = new Object();
    var json_data = new Object();
    var start = false;

    // events
    var s = document.getElementById('start');
    s.addEventListener('click', start_click, false);
    var anchors = document.getElementById('anchors');
    anchors.addEventListener('click', anchors_click, false);
    var anchors = document.getElementById('anchors');
    anchors.addEventListener('click', anchors_click, false);

    // init
    json_data.x = 50;
    json_data.y = 50;

    // background image
    var imgElement = document.getElementById('my-image');
    var imgInstance = new fabric.Image(imgElement, {
            opacity: 0.85,
            hasControls: false,
            lockMovementY: true,
            lockMovementX: true,
            selectable: false
    });

    // create a rectangle object - > your position
    var rect = new fabric.Rect({
        originY:'center',
        originX:'center',
        left: json_data.x,
        top: json_data.y,
        fill: 'red',
        width: 5,
        height: 5
    });

    // create anchors
    var tri1 = new fabric.Triangle({
        width: 5, height: 5, fill: 'blue', left: 50, top: 50,
        originY: 'center',
        originX: 'center'
    });

    var tri2 = new fabric.Triangle({
        width: 5, height: 5, fill: 'blue', left: 750, top: 50,
        originY: 'center',
        originX: 'center'
    });

    var tri3 = new fabric.Triangle({
        width: 5, height: 5, fill: 'blue', left: 50, top: 550,
        originY: 'center',
        originX: 'center'
    });


    // create hazard area





    // 'add'  elements onto canvas
    canvas.add(imgInstance);
    canvas.add(rect);
    canvas.add(tri1);
    canvas.add(tri2);
    canvas.add(tri3);

    // refresh
    setInterval(draw,  100);

    // drawing function
    function draw(){
        // load -  breaking the cache
        loadJSON('../static/data.json' + '?t=' + Math.random(), load_data);
        if(json_data.x > -1){
            // reset positions
            rect.set({ left: Math.round(json_data.x),
                        top: Math.round(json_data.y) });
        }
        if(start == true){
            canvas.renderAll()
            postJSON('/data', new_data);
        }
    }

    // loading
    function loadJSON(file, callback){
        var xobj = new XMLHttpRequest();
        //xobj.overrideMimeType('application/json');
        xobj.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == '200') {
        // Required use of an anonymous callback as .open will NOT return a
        // //value but simply returns undefined in asynchronous mode
                callback(this);
            }
        };
        xobj.open('GET', file, true);
        xobj.send();
    }

    function postJSON(file, callback){
        var xhr = new XMLHttpRequest();
        xhr.open('POST', file, true);
        xhr.setRequestHeader('Content-type', 'application/json');
        //xhr.onreadystatechange = function() {
        //    if(xhr.readyState == 4 && xhr.status == 200) {
        //            alert(xhr.responseText);
        //    }
        //}
        var data = callback();
        xhr.send(data);
    }

    // Callback function
    function load_data(obj){
        var json_real = JSON.parse(obj.responseText);
        json_data.x = json_real.x;
        json_data.y = json_real.y;
    }

    function new_data(){
        return JSON.stringify({ 'x' : Math.random()*800,
                                'y' : Math.random()*600 });
    }

    function new_set(){

    }

    function start_click(){
        start = true;
        document.getElementById('console').innerHTML = 'Program Started';
    }

}
