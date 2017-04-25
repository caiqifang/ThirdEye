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
    var an = document.getElementById('anchors');
    an.addEventListener('click', anchors_click, false);
    var ar = document.getElementById('area');
    ar.addEventListener('click', area_click, false);
    var t = document.getElementById('tag');
    t.addEventListener('click', tag_click, false);

    // init
    var max_x = 800;
    var max_y = 600;
    var x = 450;
    var y = 450;
    json_set.bound_top = 0;
    json_set.bound_left = 0;
    json_set.max_width = 800;
    json_set.max_height = 600;
    json_set.num_anchors = 0;
    json_set.num_tags = 0;
    json_set.anchors_list = new Object();

    // background image
    var imgElement = document.getElementById('my-image');
    var imgInstance = new fabric.Image(imgElement, {
        opacity: 0.4,
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

    // 'add'  elements onto canvas
    canvas.add(imgInstance);

    // refresh
    setInterval(draw,  200);  // unit: ms

    // drawing function
    function draw(){
        // load -  breaking the cache
        document.getElementById('console').innerHTML = JSON.stringify(json_set);
        document.getElementById('output').innerHTML = JSON.stringify(json_data);
        document.getElementById('a_output').innerHTML = JSON.stringify(json_area);
        if(start == true){
            loadJSON('../static/data.json' + '?t=' + Math.random(), load_data);
            canvas.renderAll();
        }
    }

    // ===================    loading  ================
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

    // ==============   GET CALLBACK ===========
    // loading data from data.json
    function load_data(obj){
        var json_rly = JSON.parse(obj.responseText);
        for (var id in json_rly) {
            // skip loop if the property is from prototype
            if (!json_rly.hasOwnProperty(id)) continue;
            var obj = json_rly[id];
            // reset positions
            json_data[id].setLeft(obj.x);
            json_data[id].setTop(obj.y);
        }
    }

    /*
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
    */

    // ==============   POST CALLBACK ===========
    /* function new_data(){
    //  return JSON.stringify({
    //     'x':  Math.random()*json_set.max_width + json_set.bound_left,
    //      'y':  Math.random()*json_set.max_height + json_set.bound_top
    // });
    }*/

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
        var tops = [];
        var lefts = [];
        for (var id in json_set.anchors_list){
            tops.push(json_set.anchors_list[id].top);
            lefts.push(json_set.anchors_list[id].left);
        }
        tops.sort();
        lefts.sort();
        json_set.bound_left = lefts[0];
        json_set.bound_top = tops[0];
        json_set.max_width = lefts[json_set.num_anchors - 1] - lefts[0];
        json_set.max_height = tops[json_set.num_anchors - 1] - tops[0];
        postJSON('/set', new_set);
    }

    function anchors_click(){
        var txt = document.getElementById('message').value;
        if(!checkInput(txt)) return;
        var tri = new fabric.Triangle({
            width: 10, height: 10, fill: 'blue', left: 500, top: 550,
            originY: 'center',
            originX: 'center',
            lockScalingX: true,
            lockScalingY: true
        });
        /*
           json_set.bound_top = 0;
           json_set.bound_left = 0;
           json_set.max_width = 800;
           json_set.max_height = 600;
           json_set.num_anchors = 0;
           json_set.num_tags = 0;
           json_set.anchors_list = new Object();
           */
        json_set.anchors_list[txt] = tri;
        canvas.add(json_set.anchors_list[txt]);
        json_set.num_anchors = Object.keys(json_set.anchors_list).length;
        document.getElementById('console').innerHTML = 'Set Anchors';
        canvas.renderAll();
    }

    function area_click(){
        // create hazard area
        document.getElementById('console').innerHTML = 'Set Area';
        canvas.renderAll();
    }

    function tag_click(){
        var txt = document.getElementById('message').value;
        if(!checkInput(txt)) return;
        // create a rectangle object - > your position
        var tag = new fabric.Rect({
            originY:'center',
            originX:'center',
            left: x,
            top: y,
            fill: 'red',
            width: 10,
            height: 10,
            lockScalingX: true,
            lockScalingY: true
        });
        json_data[txt] = tag;
        canvas.add(json_data[txt]);
        document.getElementById('console').innerHTML = 'Set Tags';
        json_set.num_tags = getSize(json_data);
        canvas.renderAll();
    }

    /* ============ HELPER FUNCTION ========*/
    function getSize(obj){
        var size = 0, key;
        for (key in obj) {
            if (obj.hasOwnProperty(key)) size++;
        }
        return size;
    }
    function checkInput(txt)
    {
        if(isNaN(x) || txt == ''){
            alert("Must input numbers");
            return false;
        }
        return true;
    }
}
