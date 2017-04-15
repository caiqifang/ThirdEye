window.onload = function(){
    // create a wrapper around native canvas element
    var canvas = new fabric.Canvas('main');

    // Json files
    var json_area = new Object();
    var json_set = new Object();
    var json_data = new Object();
    var start = false;
    var i; // counter

    // events
    var s = document.getElementById('start');
    s.addEventListener('click', start_click, false);
    var anchors = document.getElementById('anchors');
    anchors.addEventListener('click', anchors_click, false);
    var area = document.getElementById('area');
    area.addEventListener('click', area_click, false);

    // init
    var max_x = 800;
    var max_y = 600;
    json_data.x = 450;
    json_data.y = 450;
    json_set.bound_top = 0;
    json_set.bound_left = 0;
    json_set.max_width = 800;
    json_set.max_height = 600;
    json_set.num_anchors = 0;
    json_set.anchors_list = [];

    // background image
    var imgElement = document.getElementById('my-image');
    var imgInstance = new fabric.Image(imgElement, {
        opacity: 0.85,
        hasControls: false,
        lockMovementY: true,
        lockMovementX: true,
        selectable: false
    });
    imgInstance.set({
        top: 0,
        left: 0,
        scaleY: canvas.height / imgInstance.height,
        scaleX: canvas.width / imgInstance.width
    });



    // create a rectangle object - > your position
    var tag = new fabric.Rect({
        originY:'center',
        originX:'center',
        left: json_data.x,
        top: json_data.y,
        fill: 'yellow',
        width: 5,
        height: 5,
        lockScalingX: true,
        lockScalingY: true
    });

    if(json_set.anchors_list.length == 0){
        // create anchors
        json_set.anchors_list.push( new fabric.Triangle({
            width: 5, height: 5, fill: 'red', left: 500, top: 550,
            originY: 'center',
            originX: 'center',
            lockScalingX: true,
            lockScalingY: true
        }));

        json_set.anchors_list.push( new fabric.Triangle({
            width: 5, height: 5, fill: 'green', left: 650, top: 550,
            originY: 'center',
            originX: 'center',
            lockScalingX: true,
            lockScalingY: true
        }));

        json_set.anchors_list.push( new fabric.Triangle({
            width: 5, height: 5, fill: 'blue', left:750, top: 550,
            originY: 'center',
            originX: 'center',
            lockScalingX: true,
            lockScalingY: true
        }));
    }

    // create hazard area

    // check pre exist points and load
    // loadJSON('../static/set.json' + '?t=' + Math.random(), load_set);

    // 'add'  elements onto canvas
    canvas.add(imgInstance);

    for (i = 0; i < json_set.anchors_list.length; i++){
        canvas.add(json_set.anchors_list[i]);
    }

    canvas.add(tag);

    // refresh
    setInterval(draw,  200);  // unit: ms

    // drawing function
    function draw(){
        // load -  breaking the cache
        document.getElementById('console').innerHTML = JSON.stringify(json_set);
        if(start == true){
            loadJSON('../static/data.json' + '?t=' + Math.random(), load_data);
            if(json_data.x > -1){
                // reset positions
                tag.set({ left: Math.round(json_data.x),
                    top: Math.round(json_data.y) });
            }
            canvas.renderAll()
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

    // ==============   GET CALLBACK ===========
    // loading data from data.json
    function load_data(obj){
        var json_real = JSON.parse(obj.responseText);
        json_data.x = json_real.x;
        json_data.y = json_real.y;
    }

    // loading set from json
    function load_set(obj){
        var json = JSON.parse(obj.responseText);
        json_set.anchors_list = json.anchors_list;
        json_set.bound_top = json.bound_top;
        json_set.bound_left = json.bound_left;
        json_set.max_width = json.max_width;
        json_set.max_height = json.max_height;
        json_set.num_anchors = json.num_anchors;
        json_set.anchors_list = json.anchors_list;
    }

    // ==============   POST CALLBACK ===========
    function new_data(){
        //  return JSON.stringify({
        //     'x':  Math.random()*json_set.max_width + json_set.bound_left,
        //      'y':  Math.random()*json_set.max_height + json_set.bound_top
        // });
    }

    function new_set(){
        return JSON.stringify(json_set);
    }

    function new_area(){
        return JSON.stringify(json_area);
    }

    // ========== EVENT FUNCTION

    function start_click(){
        start = true;
        document.getElementById('console').innerHTML = 'Program Started';
    }

    function anchors_click(){
        document.getElementById('console').innerHTML = 'Set Anchors';
        json_set.num_anchors = json_set.anchors_list.length;
        var tops = [];
        var lefts = [];
        for (i = 0; i < json_set.num_anchors; i++){
            tops.push(json_set.anchors_list[i].top);
            lefts.push(json_set.anchors_list[i].left);
        }
        tops.sort();
        lefts.sort();
        json_set.bound_left = lefts[0];
        json_set.bound_top = tops[0];
        json_set.max_width = lefts[json_set.num_anchors - 1] - lefts[0];
        json_set.max_height = tops[json_set.num_anchors - 1] - tops[0];
        postJSON('/set', new_set);
    }

    function area_click(){
        document.getElementById('console').innerHTML = 'Set Area';
    }
}
