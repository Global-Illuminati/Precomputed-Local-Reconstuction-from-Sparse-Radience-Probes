//TODO: handle materials returned from the parse function

function MatrixLoader() {
    //Empty constructor so far...
}

MatrixLoader.prototype = {
    constructor: MatrixLoader(),

    //TODO: break loader into separate file and send callbacks to load function as params
    load: function(url, onload, typed_array) {
        var xhr = new XMLHttpRequest();
        //Send date as request parameter to avoid caching issues
        xhr.open('GET', url + "?t=" + new Date().getTime(), true);
        var scope = this;
        xhr.responseType = 'arraybuffer';
        xhr.onload = function() {
            if(xhr.status == 200)
                onload(scope.read(xhr.response,typed_array));
            else 
                console.error('matrix loader could not load the requested file:',url);
        }
        xhr.send();
    },

    read: function(buffer,typed_array) {
        var dataview = new DataView(buffer);
        var w = dataview.getInt32(0, true);
        var h = dataview.getInt32(4, true);
        var ret = new typed_array(buffer,8);
        if(w*h != ret.length) console.error('matrix dimensions and data length is inconsistent');
        return {data:ret,width:w,height:h};
    },
}