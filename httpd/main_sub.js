#!/usr/local/bin/silkjs
// httpd/main.js

print_r = require('builtin/print_r');

console = require('console');
fs = require('fs');
sem = require('builtin/sem');
LogFile = require('LogFile');
net = require('builtin/net');
process = require('builtin/process');
v8 = require('builtin/v8');
http = require('builtin/http');
buffer = require('builtin/buffer');
uglify = require('UglifyJS/uglify-js');
//include('lib/phpjs.js');
include('lib/forEach.js');
//include('lib/Exceptions.js');
include('lib/Util.js');
Json = require('Json');
include('lib/Jst.js');
include('lib/Showdown.js');
MySQL = require('MySQL');
var SSH = require('SSH');
////
include('httpd/config.js');
include('httpd/request.js');
include('httpd/response.js');
include('httpd/child.js');

function main() 
{
    // Here I need to retrieve the common socket information.
    var clientSocket;
    clientSocket = net.getSocketDescriptor();

    print(' - main_sub.js 11 : ' + clientSocket + '\r\n');

    HttpChild.run(clientSocket, process.getpid());
}
