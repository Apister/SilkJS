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
    var debugMode = false;
    var cldprc;
    var children = {};

    // load any user provided JavaScripts
	arguments.each(function(arg) {
		if (arg === '-d') {
            debugMode = true;
			return false;
        }		
	});

	if (debugMode) {
		v8.enableDebugger();
		log('Running in debug mode');
	}
    arguments.each(function(arg) {
        if (arg.endsWith('.js') || arg.endsWith('.coffee')) {
            include(arg);
        }
    });


    // Open/Create a lock file then close.
    var pid;
    var fd = fs.open(Config.lockFile, fs.O_WRONLY|fs.O_CREAT|fs.O_TRUNC, 0644);

    fs.close(fd);

    print(' - main.js PORT : ' + Config.port.toString() + '\r\n');

    // Start non blocking listening.
    var serverSocket = net.listen(Config.port, 75, Config.listenIp);
    
    sem.createM();
    
    //global.logfile = new LogFile(Config.logFile || '/httpd-silkjs.log');
    //
    // Commenting this out for (NOW) because it's crashing but it's not important at this time.
    //
    // ------> global.logfile = new LogFile(Config.logFile || '/httpd-silkjs.log');

    if (debugMode) {
		while (1) {
	        HttpChild.run(serverSocket, process.getpid());
		}
    }

    print(' - main.js 8 : ' + Config.numChildren.toString() + '\r\n');

    for (var i=0; i<Config.numChildren; i++) 
    {
        //pid = process.fork();
        pid = process.allInOne(serverSocket);

        // --------> Here is where the new process starts

        if (pid == 0) {  // This will run when it's a child process.
	    print(' - main.js : (pid == 0) \r\n');
            HttpChild.run(serverSocket, process.getpid());
            process.exit(0);
        }
        else if (pid == -1) {
	    print(' - main.js : (pid == -1) \r\n');
            console.error(process.error());
        }
        else { // The process id of the child will be given in the parent process.
	    print(' - main.js : else \r\n');
            children[pid] = true;
        }
    }

    var logMessage = 'SilkJS HTTP running with ' + Config.numChildren + ' children on port ' + Config.port;
    if (Config.listenIp !== '0.0.0.0') {
        logMessage += ' on IP ' + Config.listenIp;
    }

//    console.log('Silk running with ' + Config.numChildren + ' children on port ' + Config.port);
//    logfile.write('Silk running with ' + Config.numChildren + ' children on port ' + Config.port + '\n');

    console.log(logMessage);
    //logfile.writeln(logMessage);
    while (true) {
        var o = process.wait();
	console.log('Spawning new processes : ' + o.status);

//        if (!children[o.pid]) {
//            console.log('********************** CHILD EXITED THAT IS NOT HTTP CHILD');
//            continue;
//        }

        delete children[o.pid];
//      pid = process.fork();
	for(var i = 0; i < o.status; ++i)
	{
	    //console.log('processes : ' + o.status);
	    pid = process.allInOne(serverSocket);
	}
	
        if (pid == 0) {
            HttpChild.run(serverSocket, process.getpid());
            process.exit(0);
        }
        else if (pid == -1) {
            console.error(process.error());
        }
        else {
            children[pid] = true;
        }
    }
}
