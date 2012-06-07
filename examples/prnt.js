
/*
var fs = require('builtin/fs');
*/

//fs = require('fs');
//ReadLine = require('ReadLine');

function main(host, username, password) {
    var fs = builtin.fs;
    tryPath = "c:\\newdir";
    v8 = builtin.v8;

    print("Usage: ftpclient.js host [username [password]]");

    console.log(); // 2

    fs.mkdir(tryPath);
    fs.isFile(tryPath);

    if (fs.isDir(tryPath)) {
        if (!tryPath.endsWith('/')) {
            tryPath += '/';
        }
        tryPath += 'index.js';
    }
    if (fs.isFile(tryPath)) 
    {
        return tryPath;
    }


    x = 20;
    var script = v8.compileScript('x = {}; x.bar = 10; x.foo = function() { return 10; }; x', 'test');
    //var script = v8.compileScript(context, '10;', 'test');
    for (var xxx = 0; xxx < 100; xxx++) {
        var res = v8.runScript(script);
    }
    //fs.mkdir("c:\\newdir");

    return;
}
