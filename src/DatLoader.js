//TODO: handle materials returned from the parse function

function DatLoader() {
    //Empty constructor so far...
}

DatLoader.prototype = {
    constructor: DatLoader(),

    //TODO: break loader into separate file and send callbacks to load function as params
    load: function(url, onload) {
        var xhr = new XMLHttpRequest();
        //Send date as request parameter to avoid caching issues
        xhr.open('GET', url + "?t=" + new Date().getTime(), true);
        var scope = this;
        xhr.onload = function() {
            onload(scope.parse(this.response));
        }
        xhr.send();
    },

    parse: function(input) {
        console.time('dat loader');
        var lines = input.split('\n');
        var fst_line = lines[0].split(' ');
        var num_outer = fst_line[0];    
        var num_inner = fst_line[1];
        var ret = [];
        for (var i = 0; i < num_outer; i++) {
            ret.push(lines[i+1].split(' ').filter(s => s.trim()).map(function(element){
                return parseFloat(element);
            }));
        }
        console.timeEnd('dat loader');
        return ret;
    },
}