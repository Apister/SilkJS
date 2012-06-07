/**
 * @module builtin/net
 * 
 * ### Synopsis
 * SilkJS builtin net object.
 * 
 * ### Description
 * The builtin/net object provides low-level access to the OS networking functions.
 * 
 * ### Usage
 * var net = require('builtin/net');
 * 
 * ### See Also:
 * Operating system man pages
 */
#include "SilkJS.h"

#define USE_CORK
#ifdef __APPLE__
//#define TCP_CORK TCP_NOPUSH
#define TCP_CORK TCP_NODELAY
#endif

#define		PIPE_NAME		"\\\\.\\pipe\\processpipe"

//#undef USE_CORK

// net.nonblock(sock)
// net.cork(flag)
// net.select(fd_array)

static char remote_addr[16];

static JSVAL net_getSocketDescriptor(JSARGS args) {
	int					sock_client;
	int					retvalue;
	DWORD				dwBytes, 
						dwMode;
	HANDLE				hPipe;
	WSAPROTOCOL_INFO	protInfo;
	WSADATA				wsaData = {0};

	WSAStartup(MAKEWORD(2, 2), &wsaData);
printf("Connect to named pipe and get socket\r\n");

	while(1) {
		/* I open in read mode the namedpipe created by the parent process */

		hPipe = CreateFile(PIPE_NAME,   // pipe name
							GENERIC_READ |  // read and write access
							GENERIC_WRITE,
							0,              // no sharing
							NULL,           // default security attributes
							OPEN_EXISTING,  // opens existing pipe
							0,              // default attributes
							NULL);          // no template file

		if (hPipe != INVALID_HANDLE_VALUE) {
			break;
		}

		if ( GetLastError() != ERROR_PIPE_BUSY ) {
			return Integer::New(-1);
		}

		/* if all instances are busy I'll wait 10000 ms */
		if ( !WaitNamedPipe(PIPE_NAME, 10000) ) {
			return Integer::New(-1);
		}
	}

	/* I set the pipe in read mode */
	dwMode = PIPE_READMODE_BYTE;

	/* I update the pipe */
	SetNamedPipeHandleState(hPipe,
							&dwMode,  // new mode
							NULL,
							NULL);

	/* I read the client socket sent me by the parent process */
	retvalue = ReadFile(hPipe, &sock_client, sizeof(sock_client), &dwBytes, NULL);

	/* I read the protocol information structure sent me by the parent process */
	retvalue = ReadFile(hPipe, &protInfo, sizeof(protInfo), &dwBytes, NULL);

	/* I create a new socket with the structure just read */
	sock_client = WSASocket(AF_INET, SOCK_STREAM, 0, &protInfo, 0, WSA_FLAG_OVERLAPPED);

printf("sock_client : %d\r\n", sock_client);	

	DWORD dw = GetLastError();
printf("gsd Last ERRor : %d\r\n", dw);	

	if ( sock_client == INVALID_SOCKET ) {
printf("gsd : %d\r\n", __LINE__);
		return Integer::New(-1);
	}

	return Integer::New(sock_client);
}

static JSVAL net_duplicateSocket(JSARGS args) {
	HandleScope			scope;
	BOOL				ret = 0;
	WSAPROTOCOL_INFO	*protInfo = new WSAPROTOCOL_INFO[1];

    Local<External>wrap = Local<External>::Cast(args[0]);
	int sock_client = args[1]->IntegerValue();
    PROCESS_INFORMATION	*piProc = (PROCESS_INFORMATION *) wrap->Value();
printf("Duplicate socket\r\n");
	/* I duplicate the socket */
	ret = WSADuplicateSocket(sock_client, piProc->dwProcessId, protInfo);

	return scope.Close(External::New(protInfo));
}

/**
 * @function net.connect
 * 
 * ### Synopsis
 * var sock = net.connect(host, port);
 * 
 * This function creates a socket and connects to the specified host and port.
 * 
 * @param {string} host - name of host to connect to
 * @param {int} port - port number to connect to
 * @return {int} sock - file descriptor or false if error occurred.
 */
static JSVAL net_connect (JSARGS args) {
    HandleScope scope;
    String::Utf8Value host(args[0]);
    int port = args[1]->IntegerValue();
    struct hostent *h = gethostbyname(*host);

    if (h == NULL) {
        /* gethostbyname returns NULL on error */
#ifdef WIN32
		perror("gethostbyname failed");
#else
        herror("gethostbyname failed");
#endif
        return False();
    }

    struct sockaddr_in sock_addr;

    /* memcpy(dest, src, length) */
    memcpy(&sock_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    /* copy the address to the sockaddr_in struct. */

    /* set the family type (PF_INET) */
    sock_addr.sin_family = h->h_addrtype;

    /*
     * addr->sin_port = port won't work because they are different byte
     * orders
     */
    sock_addr.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return False();
    }
    if (connect(fd, (struct sockaddr *) &sock_addr, sizeof (struct sockaddr_in)) < 0) {
        /* connect returns -1 on error */
        perror("connect(...) error");
        close(fd);
        return False();
    }
    return scope.Close(Integer::New(fd));
}

/**
 * @function net.listen
 * 
 * ### Synopsis
 * 
 * var sock = net.listen(port);
 * var sock = net.listen(port, backlog);
 * var sock = net.listen(port, backlog, ip);
 * 
 * This function creates a TCP SOCK_STREAM socket, binds it to the specified port, and does a listen(2) on the socket.
 * 
 * Connections to the socket from the outside world can be accepted via net.accept().
 * 
 * The backlog argument specifies the maximum length for the queue of pending connections.  If the queue fills and another connection attempt is made, the client will likely receive a "connection refused" error.
 * 
 * The ip argument specifies what IP address to listen on.  By default, it will be 0.0.0.0 for "listen on any IP."  If you set this to a different value, only that IP will be listened on, and the socket will not be reachable via localhost (for example).
 * 
 * @param {int} port - port number to listen on
 * @param {int} backlog - length of pending connection queue
 * @param {string} ip - ip address to listen on
 * @return {int} sock - file descriptor of socket in listen mode
 * 
 * ### Exceptions
 * This function throws an exception of the socket(), bind(), or listen() OS calls fail.
 */
static JSVAL net_listen (JSARGS args) {
    HandleScope scope;
    int port = args[0]->IntegerValue();
    int backlog = 30;
    if (args.Length() > 1) {
        backlog = args[1]->IntegerValue();
    }
    int listenAddress = INADDR_ANY;
#ifdef WIN32
	char *listenAddressString = (char *)"0.0.0.0";
	WSADATA wsaData = {0};
    int iResult = 0;

	int sock = INVALID_SOCKET;
    int iFamily = AF_UNSPEC;
    int iType = 0;
    int iProtocol = 0;

	iFamily = AF_INET;
    iType = SOCK_STREAM;
    iProtocol = 0;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
		char str_ret[128];
        scanf(str_ret, L"%d\n", iResult);
		return ThrowException(String::Concat(String::New("socket() Error: "), String::New(str_ret)));
    }

#else
    char *listenAddressString = (char *)'0.0.0.0';
#endif

    if (args.Length() > 2) {
        String::AsciiValue addr(args[2]);
        listenAddressString = *addr;
        listenAddress = inet_addr(*addr);
    }

#ifdef WIN32
    sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		char str_int[128];

		scanf(str_int, "%d", 0);
        return ThrowException(String::Concat(String::New("socket() Error: "), String::New(str_int)));
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
        return ThrowException(String::Concat(String::New("socket() Error: "), String::New(strerror(errno))));
    }
#endif

    {
        int on = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));
    }

    struct sockaddr_in my_addr;
#ifdef WIN32
	memset(&my_addr, 0, sizeof(my_addr));
#else
    bzero(&my_addr, sizeof (my_addr));
#endif
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
//    printf("listenAddress: '%s' %08x\n", listenAddressString, listenAddress);

    my_addr.sin_addr.s_addr = listenAddress; // htonl(listenAddress);

    if (bind(sock, (struct sockaddr *) &my_addr, sizeof (my_addr))) {
        return ThrowException(String::Concat(String::New("bind()Error: "), String::New(strerror(errno))));
    }

    if (listen(sock, backlog)) {
        return ThrowException(String::Concat(String::New("listen() Error: "), String::New(strerror(errno))));
    }
    return scope.Close(Integer::New(sock));
}

/**
 * @function net.accept
 * 
 * ### Synopsis
 * 
 * var sock = net.accept(listen_socket);
 * 
 * This function waits until there is an incoming connection to the listen_socket and returns a new socket directly connected to the client.
 * 
 * The IP address of the connecting client is stored by this function.  It may be retrieved by calling net.remote_addr().
 * 
 * @param {int} listen_socket - socket already in listen mode
 * @return {int} client_socket - socket connected to a client
 * 
 * ### Notes
 * There is an old and well-known issue known as the "thundering herd problem" involving the OS accept() call.  The problem may occur if there are a large number of processes calling accept() on the same listen_socket. 
 * 
 * The solution is to use some sort of semaphore such as fs.flock() or fs.lockf() around net.accept().
 * 
 * See http://en.wikipedia.org/wiki/Thundering_herd_problem for a brief description of the problem.
 */
static JSVAL net_accept (JSARGS args) {
    HandleScope scope;
    struct sockaddr_in their_addr;

    int sock = args[0]->IntegerValue();

    socklen_t sock_size = sizeof (struct sockaddr_in);

#ifdef WIN32
	int accpt_sock;
	memset(&their_addr, 0, sizeof(their_addr));
#else
    bzero(&their_addr, sizeof (their_addr));
#endif
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    //	struct timeval timeout;
    //	timeout.tv_sec = 5;
    //	timeout.tv_usec = 0;
    switch (select(sock + 1, &fds, NULL, NULL, NULL)) {
        case -1:
            perror("select");
            return ThrowException(String::Concat(String::New("Read Error: "), String::New(strerror(errno))));
        case 0:
            printf("select timed out\n");
            return Null();
    }

    while (1) {
        accpt_sock = accept(sock, (struct sockaddr *) &their_addr, &sock_size);

#ifdef WIN32
		if (accpt_sock == INVALID_SOCKET) {
			perror("accept failed with error: ");
			printf("%ld\r\n", WSAGetLastError());
			//closesocket(sock);
			//WSACleanup();
		} else break;
#else
        if (accpt_sock > 0) {
            break;
        }
        else {
            perror("accept");
        }
#endif
    }

    //	int yes = 1;
    //#ifdef USE_CORK
    //	setsockopt( sock, IPPROTO_TCP, TCP_CORK, (char *)&yes, sizeof(yes) );
    //#else
    //	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(yes) );
    //#endif
    //	{
    //		int x;
    //		x = fcntl(sock, F_GETFL, 0);
    //		fcntl(sock, F_SETFL, x | O_NONBLOCK);
    //	}
    strcpy(remote_addr, inet_ntoa(their_addr.sin_addr));
    return scope.Close(Integer::New(accpt_sock));
}

/**
 * @function net.remote_addr
 * 
 * ### Synopsis
 * 
 * var remote_ip = net.remote_addr();
 * 
 * This function returns the IP address of the last client to connect via net.accept().
 * 
 * @return {string} remote_ip - ip address of client
 */
static JSVAL net_remote_addr (JSARGS args) {
    HandleScope scope;
    return scope.Close(String::New(remote_addr));
}

/**
 * @function net.cork
 * 
 * ### Synopsis
 * 
 * net.cork(sock, flag);
 * 
 * This function sets or clears the Linux TCP_CORK flag on the specified socket, based upon the value of flag.
 * 
 * ### Description
 * 
 * There are two issues to consider when writing to a TCP socket.
 * 
 * TCP implements the Nagle Algorithm which is on by default for sockets.  The idea is that for connections like telnet, sending a packet per keystroke is wasteful.  The Nagle Algorithm causes a 250ms delay before sending a packet after write, in case additional writes (keystrokes) might occur.  A 250ms delay from net.write() to actual data going out to the network is a performance killer for high transaction applications (e.g. HTTP).
 * 
 * Turning off Nagle eliminates the 250ms delay, but in an HTTP type application, the headers will get sent in the first (and maybe additional) packets, then the data in succeeding packets.  It is ideal to pack all the headers and data into full packets before teh data is written.
 * 
 * Linux implements TCP_CORK to solve this issue.  If set, don't send out partial frames. All queued partial frames are sent when the option is cleared again. This is useful for prepending headers before calling sendfile(2), or for throughput optimization. As currently implemented, there is a 200 millisecond ceiling on the time for which output is corked by TCP_CORK. If this ceiling is reached, then queued data is automatically transmitted. This option can be combined with TCP_NODELAY only since Linux 2.5.71. This option should not be used in code intended to be portable.
 * 
 * OSX does not implement TCP_CORK, so we do our best with TCP_NODELAY (disable Nagle).
 * 
 * @param {int} sock - socket to perform TCP_CORK on
 * @param {boolean} flag - true to turn on TCP_CORK, false to turn it off
 */
static JSVAL net_cork (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    int flag = args[1]->IntegerValue();
#ifdef WIN32
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
#else
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof (flag));
#endif
    return Undefined();
}

/**
 * @function net.close
 * 
 * ### Synopsis
 * 
 * net.close(sock);
 * 
 * This function closes a network socket and frees any memory it uses.  You should call this when done with your sockets, or you may suffer a memory leak.
 * @param {int} socket to close
 */
static JSVAL net_close (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
#ifdef WIN32
	closesocket(fd);
#else
    close(fd);
#endif
    return Undefined();
}

/**
 * @function net.read
 * 
 * ### Synopsis
 * 
 * var s = net.read(sock, length);
 * 
 * This function reads a string of the specified maximum length from the specified socket.  The actual length of the string returned may be less than length characters.
 * 
 * If no data can be read for 5 seconds, the function returns null.
 * 
 * @param {int} sock - socket file descriptor to read from.
 * @param {int} length - maximum length of string to read.
 * @return {string} s - string that was read from the socket, or null if the string could not be read.
 * 
 * ### Exceptions
 * This function throws an exception if there is a read error with the error message.
 */
static JSVAL net_read (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    long size = args[1]->IntegerValue();

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    switch (select(fd + 1, &fds, NULL, NULL, &timeout)) {
        case -1:
            perror("select");
            return ThrowException(String::Concat(String::New("Read Error: "), String::New(strerror(errno))));
        case 0:
            printf("Read timed out\n");
            return Null();
    }

#ifdef WIN32
	char *buf = new char[size];
#else
    char buf[size];
#endif
    long count = read(fd, buf, size);
    if (count < 0) {
        return ThrowException(String::Concat(String::New("Read Error: "), String::New(strerror(errno))));
    }
    else if (count == 0) {
        return Null();
    }
    Handle<String>s = String::New(buf, count);
#ifdef WIN32
	delete buf;
#endif
    return scope.Close(s);
}

/**
 * @function net.write
 * 
 * ### Synopsis
 * 
 * var written = net.write(sock, s, size);
 * 
 * This function writes size characters from string s to the specified socket.
 * 
 * @param {int} sock - file descriptor of socket to write to.
 * @param {string} s - the string to write.
 * @param {int} length - number of bytes to write.
 * @return {int} written - number of bytes actually written.
 * 
 * ### Exceptions
 * This function throws an exception with a string describing any write errors.
 */
static JSVAL net_write (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    String::Utf8Value buf(args[1]);
    long size = args[2]->IntegerValue();
    long written = 0;
    char *s = *buf;

    while (size > 0) {
#ifdef WIN32
		long count = send(fd, s, size, 0);
#else
        long count = write(fd, s, size);
#endif

        if (count < 0) {
            return ThrowException(String::Concat(String::New("Write Error: "), String::New(strerror(errno))));
        }

        size -= count;
        s += count;
        written += count;
        ;
    }

    return scope.Close(Integer::New(written));
}

/**
 * @function net.writeBuffer
 * 
 * ### Synopsis
 * 
 * var written = net.writeBuffer(sock, buffer);
 * 
 * This function attempts to write the given buffer to the specified socket.
 * 
 * A buffer is an object to JavaScript that provides growable string functionality.  While it's true that JavaScript strings are growable in their own right, sometimes buffering data in C++ space is more optimal if you can avoid an extra string copy.
 * 
 * Buffers also have the flexibility to write a base64 encoded string to its memory as binary data.
 * 
 * @param {int} sock - file descriptor of socket to write to.
 * @param {object} buffer - buffer object to write to the socket.
 * @return {int} written - number of bytes written.
 * 
 * ### Exceptions
 * 
 * This function throws an exception if there is a write error.
 * 
 * ### Notes
 * 
 * After the buffer is written, TCP_CORK is toggled off then on to force the data to be written to the network.
 * 
 * ### See also
 * builtin/buffer
 * 
 */
static JSVAL net_writebuffer (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    Local<External>wrap = Local<External>::Cast(args[1]);
    Buffer *buf = (Buffer *) wrap->Value();

    long size = buf->length();
    long written = 0;
    unsigned char *s = buf->data();
    while (size > 0) {
#ifdef WIN32
		long count = send(fd, (const char *)s, size, 0);
#else
        long count = write(fd, s, size);
#endif
        if (count < 0) {
            return ThrowException(String::Concat(String::New("Write Error: "), String::New(strerror(errno))));
        }
        size -= count;
        s += count;
        written += count;
        ;
    }
#ifdef WIN32
    int flag = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof (flag));
    flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof (flag));
#else
    int flag = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof (flag));
    flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof (flag));
#endif
    return scope.Close(Integer::New(written));
}

/**
 * @function net.sendfile
 * 
 * ### Synopsis
 * 
 * net.sendFile(sock, path);
 * net.sendFile(sock, path, offset);
 * net.sendFile(sock, path, offset, size);
 * 
 * This function calls the OS sendfile() function to send a complete or partial file to the network entirely within kernel space.  It is a HUGE speed win for HTTP and FTP type servers.
 * 
 * @param {int} sock - file descriptor of socket to send the file to.
 * @param {string} path - file system path to file to send.
 * @param {int} offset - offset from beginning of file to send (for partial).  If omitted, the entire file is sent.
 * @param {int} size - number of bytes of the file to send.  If omitted, the remainder of the file is sent (or all of it).
 * 
 * ### Exceptions
 * An exception is thrown if the file cannot be opened or if there is a sendfile(2) OS call error.
 */
static JSVAL net_sendfile (JSARGS args) {
    HandleScope handle_scope;
    int sock = args[0]->IntegerValue();
    String::AsciiValue filename(args[1]);
    off_t offset = 0;
    if (args.Length() > 2) {
        offset = args[2]->IntegerValue();
    }
    size_t size;
    if (args.Length() > 3) {
        size = args[3]->IntegerValue();
    }
    else {
        struct stat buf;
        if (stat(*filename, &buf)) {
            printf("%s\n", *filename);
            perror("SendFile stat");
            return handle_scope.Close(False());
        }
        size = buf.st_size - offset;
    }
#ifdef WIN32
	HANDLE fd = CreateFile(*filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
#else
    int fd = open(*filename, O_RDONLY);
    if (fd < 0) {
#endif
        return ThrowException(String::Concat(String::New("sendFile open Error: "), String::New(strerror(errno))));
    }

    while (size > 0) {
#ifdef __APPLE__
        off_t count = size;
        if (sendfile(fd, sock, offset, &count, NULL, 0) == -1) {
            close(fd);
            return ThrowException(String::Concat(String::New("sendFile Error: "), String::New(strerror(errno))));
        }
#else
#ifdef WIN32
		SetFilePointer(fd, offset, NULL, FILE_BEGIN);
		TransmitFile(sock, fd, size, 0, NULL, NULL,0);
		ssize_t count = size;
        if (count == -1) {
            CloseHandle(fd);
            return ThrowException(String::Concat(String::New("sendFile Error: "), String::New(strerror(errno))));
        }
#else
        ssize_t count = sendfile(sock, fd, &offset, size);
        if (count == -1) {
            close(fd);
            return ThrowException(String::Concat(String::New("sendFile Error: "), String::New(strerror(errno))));
        }
#endif
#endif
        size -= count;
        offset += count;
    }
#ifdef WIN32
	CloseHandle(fd);
    int flag = 0;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof (flag));
    flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof (flag));
#else
    close(fd);
    int flag = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof (flag));
    flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof (flag));
#endif

    return Undefined();
}

void init_net_object () {
    HandleScope scope;

    Handle<ObjectTemplate>net = ObjectTemplate::New();
    net->Set(String::New("connect"), FunctionTemplate::New(net_connect));
    net->Set(String::New("listen"), FunctionTemplate::New(net_listen));
    net->Set(String::New("accept"), FunctionTemplate::New(net_accept));
    net->Set(String::New("remote_addr"), FunctionTemplate::New(net_remote_addr));
    net->Set(String::New("cork"), FunctionTemplate::New(net_cork));
    net->Set(String::New("close"), FunctionTemplate::New(net_close));
    net->Set(String::New("read"), FunctionTemplate::New(net_read));
    net->Set(String::New("write"), FunctionTemplate::New(net_write));
    net->Set(String::New("writeBuffer"), FunctionTemplate::New(net_writebuffer));
    net->Set(String::New("sendFile"), FunctionTemplate::New(net_sendfile));

	net->Set(String::New("duplicateSocket"), FunctionTemplate::New(net_duplicateSocket));
	net->Set(String::New("getSocketDescriptor"), FunctionTemplate::New(net_getSocketDescriptor));

    builtinObject->Set(String::New("net"), net);
}
