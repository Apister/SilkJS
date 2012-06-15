/**
 * @module builtin/process
 * 
 * ### Synopsis
 * SilkJS builtin process object.
 * 
 * ### Description
 * The builtin/process object provides constants and methods to directly access the underlying operating system's process-oriented functions.  
 * 
 * ### Usage
 * var process = require('builtin/process');
 * 
 * ### See Also
 * Operating system man pages
 * 
 */

#include "SilkJS.h"
#include "v8.h"
#ifndef WIN32
	#include "v8-read-only/src/v8.h"
	#include <pwd.h>
#endif

#define		PIPE_NAME		"\\\\.\\pipe\\processpipe"
#define		BUFSIZE			1024
#define		PIPE_TIMEOUT	5000

#ifdef WIN32
	HANDLE	hPidArray[100];
	WORD	wPidcnt = 0;
#endif

// TODO:
// getcwd()
// chdir()
// signals
// atexit, on_exit
// popen/exec/etc.

//static JSVAL process_store_pid(JSARGS args) {
//	Local<External>wrap = Local<External>::Cast(args[0]);
//	PROCESS_INFORMATION *piProc = (PROCESS_INFORMATION *) wrap->Value();
//}

static JSVAL process_allInOne(JSARGS args) {
	HANDLE				hPipe;
	BOOL				ret = 0;
	HandleScope			scope;
	PROCESS_INFORMATION piProc;
	STARTUPINFO			siStartInfo;
	WSAPROTOCOL_INFO	protInfo;
	OVERLAPPED	ol		= {0,0,0,0,NULL};
	DWORD				dwBytes;

	SOCKET sock_client	= args[0]->IntegerValue();
	/* I create a named pipe for communication with the spawned process */
	hPipe = CreateNamedPipe(PIPE_NAME,                	// pipe name
							PIPE_ACCESS_DUPLEX |		// read/write access
							FILE_FLAG_OVERLAPPED,
							PIPE_TYPE_BYTE |          	// message type pipe
							PIPE_READMODE_BYTE |      	// message-read mode
							PIPE_WAIT,                	// blocking mode
							PIPE_UNLIMITED_INSTANCES, 	// max. instances
							BUFSIZE,                  	// output buffer size
							BUFSIZE,                  	// input buffer size
							PIPE_TIMEOUT,				// client time-out
							NULL);                    	// default security attribute


	if ( hPipe == INVALID_HANDLE_VALUE ) {
		return Null();
	}

	GetStartupInfo(&siStartInfo);  
	/* I create a new process calling the "test.exe" executable */ 
	{
		char *buffer = new char[256];
		int bufferlen = 256;
		GetCurrentDirectory(bufferlen, buffer);
		delete buffer;
	}

	ret = CreateProcess(NULL, ".\\silkjs.exe httpd\\main_sub.js",  
						NULL, NULL,		/* security attributes process/thread */ 
						TRUE,			/* inherit handle */
						0,				/* fdwCreate */ 
						NULL,			/* lpvEnvironment */ 
						".",			/* lpszCurDir */ 
						&siStartInfo,	/* lpsiStartInfo */ 
						&piProc);  

//	printf("CreateProcess dwProcessId = %d\n", piProc.dwProcessId);

	if ( ret == 0 ) {
		return Null();
	}  

	ret = WSADuplicateSocket(sock_client, piProc.dwProcessId, &protInfo);

	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	/* I connect to the named pipe... */
	ret = ConnectNamedPipe(hPipe, &ol);

	if ( ret == 0 ) {
		DWORD le = GetLastError();

		switch( le ) {
			case ERROR_PIPE_CONNECTED:
				ret = TRUE;
				break;
			case ERROR_IO_PENDING:
				if( WaitForSingleObject(ol.hEvent, PIPE_TIMEOUT) == WAIT_OBJECT_0 ) {
					DWORD dwIgnore;
					ret = GetOverlappedResult(hPipe, &ol, &dwIgnore, FALSE);
				} else {
					CancelIo(hPipe);
				}
				break;
		}
	}

	CloseHandle(ol.hEvent);

	if ( ret == 0 ) {
		return Null();
	}

	/* I write the socket descriptor to the named pipe */
	if ( WriteFile(hPipe, &sock_client, sizeof(sock_client), &dwBytes, NULL) == 0 ) {
		return Null();
	}

	/* I write the protocol information structure to the named pipe */
	if ( WriteFile(hPipe, &protInfo, sizeof(protInfo), &dwBytes, NULL) == 0 ) {
		return Null();
	}

	CloseHandle(hPipe);

	hPidArray[wPidcnt++] = piProc.hProcess;

	//printf("Created process [%d] = %d\n", (wPidcnt-1), hPidArray[(wPidcnt-1)]);

    return Integer::New(piProc.dwProcessId);
}

static JSVAL process_copyDescriptor(JSARGS args) {
	BOOL		ret = 0;
	DWORD		dwBytes;

	OVERLAPPED	ol		= {0,0,0,0,NULL};
	HANDLE		hPipe	= (HANDLE)args[0]->IntegerValue();
	SOCKET sock_client	= args[1]->IntegerValue();
    Local<External>wrap = Local<External>::Cast(args[2]);
	WSAPROTOCOL_INFO	*protInfo = (WSAPROTOCOL_INFO *) wrap->Value();

printf("Connect to named pipe and write\r\n");
	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	/* I connect to the named pipe... */
	ret = ConnectNamedPipe(hPipe, &ol);

printf("cd - ret : %d\r\n", ret);

	if ( ret == 0 ) {
		DWORD le = GetLastError();
printf("cd - le : %d\r\n", le);
		switch( le ) {
			case ERROR_PIPE_CONNECTED:
printf("cd : %d\r\n", __LINE__);
				ret = TRUE;
				break;
			case ERROR_IO_PENDING:
printf("cd : %d\r\n", __LINE__);
				if( WaitForSingleObject(ol.hEvent, PIPE_TIMEOUT) == WAIT_OBJECT_0 ) {
					DWORD dwIgnore;
					ret = GetOverlappedResult(hPipe, &ol, &dwIgnore, FALSE);
				} else {
					CancelIo(hPipe);
				}
				break;
		}
	}

	CloseHandle(ol.hEvent);
printf("cd : %d\r\n", __LINE__);
	if ( ret == 0 ) {
printf("cd : %d\r\n", __LINE__);
		return Undefined();
	}

	/* I write the socket descriptor to the named pipe */
	if ( WriteFile(hPipe, &sock_client, sizeof(sock_client), &dwBytes, NULL) == 0 ) {
printf("WriteFile : %d\r\n", __LINE__);
		return Undefined();
	}

	/* I write the protocol information structure to the named pipe */
	if ( WriteFile(hPipe, protInfo, sizeof(WSAPROTOCOL_INFO), &dwBytes, NULL) == 0 ) {
printf("WriteFile : %d\r\n", __LINE__);
		return Undefined();
	}

	CloseHandle(hPipe);

	return Undefined();
}

static JSVAL process_createNamedPipe(JSARGS args) {
	HANDLE				hPipe;

	/* I create a named pipe for communication with the spawned process */
	hPipe = CreateNamedPipe(PIPE_NAME,                	// pipe name
							PIPE_ACCESS_DUPLEX |		// read/write access
							FILE_FLAG_OVERLAPPED,
							PIPE_TYPE_BYTE |          	// message type pipe
							PIPE_READMODE_BYTE |      	// message-read mode
							PIPE_WAIT,                	// blocking mode
							PIPE_UNLIMITED_INSTANCES, 	// max. instances
							BUFSIZE,                  	// output buffer size
							BUFSIZE,                  	// input buffer size
							PIPE_TIMEOUT,				// client time-out
							NULL);                    	// default security attribute

printf("Create Named Pipe\r\n");
printf("cnp : %d[%d]\r\n", __LINE__, hPipe);
	if ( hPipe == INVALID_HANDLE_VALUE ) {
printf("cnp : %d\r\n", __LINE__);
		return Integer::New(-1);
	}
printf("cnp : %d\r\n", __LINE__);
	return Integer::New((int)hPipe);
}

static JSVAL process_createProcess(JSARGS args) {
	BOOL				ret = 0;
	HandleScope			scope;
	PROCESS_INFORMATION *piProc = new PROCESS_INFORMATION[1];
	STARTUPINFO			siStartInfo;

	GetStartupInfo(&siStartInfo);  
	/* I create a new process calling the "test.exe" executable */ 
	{
		char *buffer = new char[256];
		int bufferlen = 256;
		GetCurrentDirectory(bufferlen, buffer);
		delete buffer;
	}

	ret = CreateProcess(NULL, ".\\silkjs.exe httpd\\main_sub.js",  
						NULL, NULL,		/* security attributes process/thread */ 
						TRUE,			/* inherit handle */
						0,				/* fdwCreate */ 
						NULL,			/* lpvEnvironment */ 
						".",			/* lpszCurDir */ 
						&siStartInfo,	/* lpsiStartInfo */ 
						piProc);  

	if ( ret == 0 ) {
		return Null();
	}  

	return scope.Close(External::New(piProc));
}


/**
 * @function process.error
 * 
 * ### Synopsis
 * Returns string version of last OS error.
 * 
 * ### Usage:
 * var message = process.error();
 * 
 * @return {string} message - error message.
 */
static JSVAL process_error (JSARGS args) {
    HandleScope scope;
    return scope.Close(String::New(strerror(errno)));
}

/**
 * @function process.kill
 * 
 * ### Synopsis
 * 
 * var success = process.kill(pid);
 * 
 * Send the SIGKILL signal to the specified process (by pid).
 * 
 * @param {int} pid - process ID (pid) of process to kill.
 * @return {int} success - 0 on success, 1 if an error occurred.
 */
#ifdef WIN32
static JSVAL process_kill (JSARGS args) {
    HandleScope scope;
    DWORD pid = args[0]->IntegerValue();
	UINT uExitCode;
	HANDLE phandle = OpenProcess(0x00010000L, false, pid);
	BOOL bret = TerminateProcess(phandle, uExitCode);
    return scope.Close(Integer::New(!bret));
}
#else
static JSVAL process_kill (JSARGS args) {
    HandleScope scope;
    pid_t pid = args[0]->IntegerValue();
    return scope.Close(Integer::New(kill(pid, SIGKILL)));
}
#endif

/**
 * @function process.getpid
 * 
 * ### Synopsis
 * 
 * var my_pid = process.getpid();
 * 
 * Returns the pid of the current process.
 * 
 * @return {int} my_pid - process ID (pid) of the current process.
 */
#ifdef WIN32
static JSVAL process_getpid (JSARGS args) {
    HandleScope scope;
    return scope.Close(Integer::New(GetCurrentProcessId()));
}
#else
static JSVAL process_getpid (JSARGS args) {
    HandleScope scope;
    return scope.Close(Integer::New(getpid()));
}
#endif
/**
 * @function process.fork
 * 
 * ### Synopsis
 * 
 * var pid = process.fork();
 * 
 * Create a new process.
 * 
 * ### Description
 * 
 * Fork() causes creation of a new process.  The new process (child process) is an exact copy of the calling process (parent process) except for the following:
 * 
 * 1. The child process has a unique process ID.
 * 
 * 2. The child process has a different parent process ID (i.e., the process ID of the parent process).
 * 
 * 3. The child process has its own		copy of the parent's descriptors. These descriptors reference the same underlying objects, so that, 
 *    for instance, file pointers in file objects are shared between the child and the parent, so that an lseek(2) on a descriptor in the 
 *    child process can affect a subsequent read or write by the parent.  This descriptor copying is also used by the shell to establish standard 
 *    input and output for newly created processes as well as to set up pipes.
 * 
 * 4. The child processes resource utilizations are set to 0; see the man page for setrlimit(2).
 * 
 * @return 0 to the child, the pid of the created process to the parent.  If an error occurred, -1 is returned.
 */
#ifdef WIN32
// Going to punt for now.
static JSVAL process_fork (JSARGS args) {
    extern Persistent<Context> context;
    context->Exit();

	// Windows does not do fork() seemlisly enough to implement.
    //pid_t pid = fork();

    context->Enter();
    return Integer::New(0);
}
#else
static JSVAL process_fork (JSARGS args) {
    extern Persistent<Context> context;
    context->Exit();
    pid_t pid = fork();
    context->Enter();
    return Integer::New(pid);
}
#endif

/**
 * @function process.exit
 * 
 * ### Synopsis
 * 
 * process.exit(status);
 * 
 * Terminate the current process or program, returning status to the parent or shell.
 * 
 * @param {int} status - status code to return to parent or shell.
 * @return NEVER
 */
static JSVAL process_exit (JSARGS args) {
    exit(args[0]->IntegerValue());
    return Undefined();
}
/**
 * @function process.sleep
 * 
 * ### Synopsis
 * process.sleep(seconds);
 * 
 * Suspend execution of the current process for specified number of seconds.
 * 
 * @param {int} seconds - number of seconds to suspend.
 */
static JSVAL process_sleep (JSARGS args) {
#ifdef WIN32
	Sleep(args[0]->IntegerValue());
#else
    sleep(args[0]->IntegerValue());
#endif
    return Undefined();
}

/**
 * @function process.usleep
 * 
 * ### Synopsis
 * process.usleep(microseconds);
 * 
 * Suspend execution of the current process for specified number of microseconds.
 * 
 * @param {int} microseconds - number of microseconds to suspend.
 */
static JSVAL process_usleep (JSARGS args) {
#ifdef WIN32
	// Sorry there is not equivilant. This is due to the fact that most machines Windows 
	// runs on have hardware limits in the 1-10ms range. PC Computer Hardware is cheap. 
	// You need to have dedicated hardware to keep accurate time
	// This is the closest we can get
	Sleep(0);
#else
    usleep(args[0]->IntegerValue());
#endif
    return Undefined();
}

/**
 * @function process.wait
 * 
 * ### Synopsis
 * 
 * var o = process.wait();
 * 
 * Wait for process termination.
 * 
 * ### Description
 * 
 * This function suspends executio of its calling process until one of its child processes terminates.  The function returns an object of the form:
 * 
 * + pid: the pid of the child process that terminated
 * + status: the status returned by the child process
 * 
 * @return {object} o - information about the process that terminated.
 * 
 * ### See Also
 * process.exit()
 */
#ifdef WIN32
// Again there is not a straight forward way of doing this so I'm going to punt for now.
static JSVAL process_wait (JSARGS args) {
	DWORD	dwCnt;
	DWORD	lpExitCode;
    HANDLE	h;
	DWORD	dwProsStoped = 0;

	//printf("WaitForMultipleObjects = %d\n", wPidcnt);

    dwCnt = WaitForMultipleObjects(wPidcnt, hPidArray, FALSE, INFINITE);

	//printf("rez = %d\n", dwCnt);

	//for(DWORD dwidx = 0; dwidx < wPidcnt; ++dwidx) { printf("B hPidArray[%d] = %d\n", dwidx, hPidArray[dwidx]); }
	//printf("B wPidcnt = %d\n", wPidcnt);

	// There could be more then one process that has had an event notification so we should check each one when it shuts down.
	if (wPidcnt > 1)
	{
		++dwProsStoped;
		memmove(&hPidArray[dwCnt], &hPidArray[dwCnt+1], (dwCnt)*sizeof(HANDLE));
	}
	wPidcnt--;

	for(dwCnt = 0; dwCnt < wPidcnt; )
	{
		if (GetExitCodeProcess(hPidArray[dwCnt], &lpExitCode) != STILL_ACTIVE)
		{
			++dwProsStoped;
			memmove(&hPidArray[dwCnt], &hPidArray[dwCnt+1], (dwCnt)*sizeof(HANDLE));
			wPidcnt--;
		} else ++dwCnt;
	}

	//printf("A wPidcnt = %d\n", wPidcnt);
	
    Handle<Object>o = Object::New();
    o->Set(String::New("pid"), Integer::New(dwCnt));
    o->Set(String::New("status"), Integer::New(dwProsStoped));
    return o;
}
#else
static JSVAL process_wait (JSARGS args) {
    int status;
    pid_t childPid = waitpid(-1, &status, 0);
    if (childPid == -1) {
        perror("wait ");
    }
    Handle<Object>o = Object::New();
    o->Set(String::New("pid"), Integer::New(childPid));
    o->Set(String::New("status"), Integer::New(status));
    return o;
}
#endif
/**
 * @function process.exec
 * 
 * ### Synopsis
 * 
 * var output = process.exec(command_line);
 * 
 * Execute a Unix command, returning the output of the command (it's stdout) as a JavaScript string.
 * 
 * ### Description
 * 
 * This function calls popen() with the specified command line and reads its output until end of file.  Under the hood, a fork() and exec() is performed which is not particularly fast.  Still it can be useful to execute shell commands.
 * 
 * @param {string} command_line - a Unix command to execute
 * @return {string} output - the output of the command executed.
 */
static JSVAL process_exec (JSARGS args) {
    String::AsciiValue cmd(args[0]);
    string s;
    char buf[2048];
#ifdef WIN32
	FILE *fp = _popen(*cmd, "r");
#else
	FILE *fp = popen(*cmd, "r");
#endif
    int fd = fileno(fp);
    while (ssize_t size = read(fd, buf, 2048)) {
        s.append(buf, size);	
    }
#ifdef WIN32
	feof(fp);
#else
    pclose(fp);
#endif
    return String::New(s.c_str(), s.size());
}

/**
 * @function process.getuid
 * 
 * ### Synopsis
 * 
 * var uid = process.getuid();
 * 
 * Get the real user ID of the calling process.
 * 
 * @return {int} uid - the user ID of the calling process.
 */
static JSVAL process_getuid (JSARGS args) {
#ifdef WIN32
	HANDLE Thandle;
	DWORD DAmask, size;
	SID SidInfo;
	DAmask = TOKEN_READ;

	if (OpenProcessToken(GetCurrentProcess(), DAmask, &Thandle)) {
		perror("error: could not open token\n");
	}
/*
	if (GetTokenInformation(Thandle, TokenLogonSid, &SidInfo, sizeof(SidInfo), &size) == 0) {
		perror("error: get token information failed\n");
	}
	else 
	{
		perror("error: get token information failed\n");
	}
*/
	// So what do i put in this value????
	return Integer::New(0);

#else
    return Integer::New(getuid());
#endif
}

/**
 * @function process.env
 * 
 * ### Synopsis
 * 
 * var env = process.env();
 * 
 * Get a hash of key/value pairs representing the environment of the calling process.
 * 
 * Typical kinds of environment variables you'll see returned by this function are:
 * HOME - user's home directory.
 * PATH - the sequence of directories, separated by colons, searched for command execution.
 * ...
 * 
 * @return {object} env - hash of key/value environment variables.
 */
static JSVAL process_env (JSARGS args) {
    extern char **environ;
    int size = 0;
    while (environ[size]) size++;
    Handle<Object>env = Object::New();
    char *home = NULL;
    for (int i = 0; i < size; ++i) {
        const char* key = environ[i];
        const char* val = strchr(key, '=');
        const int klen = val ? val - key : strlen(key);
        if (val[0] == '=') val = val + 1;
        const int vlen = val ? strlen(val) : 0;
        env->Set(String::New(key, klen), String::New(val, vlen));
        if (!strcmp(key, "HOME")) {
            home = (char *) val;
        }
    }
    if (!home) {
#ifdef WIN32
		 home = getenv("HOMEPATH");
#else
        struct passwd *pw = getpwuid(getuid());
        home = pw->pw_dir;
#endif
        env->Set(String::New("HOME"), String::New(home));
    }
    return env;
}

/**
 * @function process.rusage
 * 
 * ### Synopsis
 * 
 * var o = process.rusage();
 * 
 * Get resource usage information for current process.
 * 
 * The object returned is of the form:
 * 
 * + time: total user + system CPU time used.
 * + utime: user CPU time used (float, in seconds).
 * + stime: system CPU time used (float, in seconds).
 * + maxrss: maximum resident set size.
 * + ixrss: integral shared memory size.
 * + idrss: integral unshared data size.
 * + isrss: integral unshared stack size.
 * + minflt: page reclaimes (soft page faults).
 * + majflt: page faults (hard page faults).
 * + nswap: swaps.
 * + inblock: block input operations.
 * + oublock: block output operations.
 * + msgsnd: IPC messages sent.
 * + msgrcv: IPC messages received.
 * + nsignals: signals received.
 * + nvcsw: voluntary context switches.
 * + nivcsw: involuntary context switches.
 */
static double timeval2sec (const timeval& t) {
    double f = (double) t.tv_sec + t.tv_usec / 1000000.0;
    f = long(f * 1000000.0 + .5);
    f /= 1000000;
    return f;
}

static timeval addTime (timeval& t1, timeval& t2) {
    timeval t;
    t.tv_sec = t1.tv_sec + t2.tv_sec;
    t.tv_usec = t1.tv_usec + t2.tv_usec;

    if (t.tv_usec >= 1000000) {
        t.tv_sec += 1;
        t.tv_usec -= 1000000;
    }

    return t;
}

static JSVAL process_rusage (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	// I'm going to punt again.
	PROCESS_MEMORY_COUNTERS pmc;

	HANDLE hProcess = GetCurrentProcess();
//	GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc));
    JSOBJ o = Object::New();
/*
    o->Set(String::New("time"), Number::New(0));
    o->Set(String::New("utime"), Number::New(0));
    o->Set(String::New("stime"), Number::New(0));
    o->Set(String::New("maxrss"), Integer::New(0));
    o->Set(String::New("ixrss"), Integer::New(0));					
    o->Set(String::New("idrss"), Integer::New(0));
    o->Set(String::New("isrss"), Integer::New(0));
    o->Set(String::New("minflt"), Integer::New(0));
    o->Set(String::New("majflt"), Integer::New(0));
    o->Set(String::New("nswap"), Integer::New(0));
    o->Set(String::New("inblock"), Integer::New(0));
    o->Set(String::New("oublock"), Integer::New(0));
    o->Set(String::New("msgend"), Integer::New(0));
    o->Set(String::New("msgrcv"), Integer::New(0));
    o->Set(String::New("nsignals"), Integer::New(0));
    o->Set(String::New("nvcsw"), Integer::New(0));
    o->Set(String::New("nivcsw"), Integer::New(0));
	*/
#else
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    JSOBJ o = Object::New();
    o->Set(String::New("time"), Number::New(timeval2sec(addTime(r.ru_utime, r.ru_stime))));
    o->Set(String::New("utime"), Number::New(timeval2sec(r.ru_utime)));
    o->Set(String::New("stime"), Number::New(timeval2sec(r.ru_stime)));
    o->Set(String::New("maxrss"), Integer::New(r.ru_maxrss));
    o->Set(String::New("ixrss"), Integer::New(r.ru_ixrss));
    o->Set(String::New("idrss"), Integer::New(r.ru_idrss));
    o->Set(String::New("isrss"), Integer::New(r.ru_isrss));
    o->Set(String::New("minflt"), Integer::New(r.ru_minflt));
    o->Set(String::New("majflt"), Integer::New(r.ru_majflt));
    o->Set(String::New("nswap"), Integer::New(r.ru_nswap));
    o->Set(String::New("inblock"), Integer::New(r.ru_inblock));
    o->Set(String::New("oublock"), Integer::New(r.ru_oublock));
    o->Set(String::New("msgend"), Integer::New(r.ru_msgsnd));
    o->Set(String::New("msgrcv"), Integer::New(r.ru_msgrcv));
    o->Set(String::New("nsignals"), Integer::New(r.ru_nsignals));
    o->Set(String::New("nvcsw"), Integer::New(r.ru_nvcsw));
    o->Set(String::New("nivcsw"), Integer::New(r.ru_nivcsw));
#endif
    return scope.Close(o);
}

/**
 * @function process.getlogin
 * 
 * ### Synopsis
 * 
 * var username = process.getlogin();
 * 
 * Get a string containing the name of the user logged in on the controlling terminal of the process.
 * @return {string} username - name of user or false if error.
 */
static JSVAL process_getlogin (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	DWORD wsize = 256;
	char s[256+1	];
	if (GetUserName(((LPSTR)&s), &wsize) == 0)
		printf("The local username is %s.", s);
	else
	  printf("GetUserName error.");
#else
    char *s = getlogin();
#endif
    if (!s) {
        return scope.Close(False());
    }
    return scope.Close(String::New(s));
}

void init_process_object () {
    HandleScope scope;

    Handle<ObjectTemplate>process = ObjectTemplate::New();
    process->Set(String::New("env"), FunctionTemplate::New(process_env));
    process->Set(String::New("error"), FunctionTemplate::New(process_error));
    process->Set(String::New("kill"), FunctionTemplate::New(process_kill));
    process->Set(String::New("getpid"), FunctionTemplate::New(process_getpid));
    process->Set(String::New("fork"), FunctionTemplate::New(process_fork));
    process->Set(String::New("exit"), FunctionTemplate::New(process_exit));
    process->Set(String::New("sleep"), FunctionTemplate::New(process_sleep));
    process->Set(String::New("usleep"), FunctionTemplate::New(process_usleep));
    process->Set(String::New("wait"), FunctionTemplate::New(process_wait));
    process->Set(String::New("exec"), FunctionTemplate::New(process_exec));
    process->Set(String::New("getuid"), FunctionTemplate::New(process_getuid));
    process->Set(String::New("rusage"), FunctionTemplate::New(process_rusage));
    process->Set(String::New("getlogin"), FunctionTemplate::New(process_getlogin));

	process->Set(String::New("createProcess"), FunctionTemplate::New(process_createProcess));

	// Named pipe calls
	process->Set(String::New("createNamedPipe"), FunctionTemplate::New(process_createNamedPipe));
	process->Set(String::New("copyDescriptor"), FunctionTemplate::New(process_copyDescriptor));
	process->Set(String::New("allInOne"), FunctionTemplate::New(process_allInOne));
	
	
    builtinObject->Set(String::New("process"), process);
}
