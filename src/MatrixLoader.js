//TODO: handle materials returned from the parse function

function MatrixLoader() {
    //Empty constructor so far...
}

MatrixLoader.prototype = {
    constructor: MatrixLoader(),

    //TODO: break loader into separate file and send callbacks to load function as params
    load: function(url, onload) {
        var xhr = new XMLHttpRequest();
        //Send date as request parameter to avoid caching issues
        xhr.open('GET', url + "?t=" + new Date().getTime(), true);
        var scope = this;
        xhr.responseType = 'arraybuffer';
        xhr.onload = function() {
            if(xhr.status == 200)
                onload(scope.read(xhr.response));
            else 
                console.error('matrix loader could not load the requested file:',url);
        }
        xhr.send();
    },

    read: function(buffer) {
        var dataview = new DataView(buffer);
        console.log(buffer);
        var w = dataview.getInt32(0, true);
        var h = dataview.getInt32(4, true);
        console.log(w,h);
        var ret = new Float32Array((buffer.byteLength-8) / 4);
        if(w*h != ret.length) console.error('matrix dimensions and data length is inconsistent');
        for (var i = 0; i < ret.length; i++) {
            ret[i] = dataview.getFloat32(i * 4+8, true);
        }
        
        return {data:ret,width:w,height:h};
    },
}