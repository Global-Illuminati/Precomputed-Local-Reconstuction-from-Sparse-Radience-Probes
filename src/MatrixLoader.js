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
                onload(scope.read(xhr.response,typed_array,url));
            else 
                console.error('matrix loader could not load the requested file:',url);
        }
        xhr.send();
    },

    read: function(buffer,typed_array,url) {
        var dataview = new DataView(buffer);
        var c = dataview.getInt32(0, true);
        var r = dataview.getInt32(4, true);
        var ret = new typed_array(buffer,8);
        if(c*r != ret.length) console.error('matrix dimensions and data length is inconsistent. Tried to load:' + url);
        return {col_major_data:ret,cols:c,rows:r};
    },
}