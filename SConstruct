
# Construct for SilkJS
env = Environment()   # Create an environmnet
env.Append(CPPPATH = [  'v8/include', 
						'v8/src', 
						'src', 
						'C:\Program Files (x86)\Oracle\Berkeley DB 11gR2 5.3.15\include',
						'C:\Program Files\MySQL\MySQL Server 5.5\include',
						'src\dependencies\libgd2_2.0.36',
						'src\dependencies\sqlite-autoconf-3071100',
						'src\dependencies\libssh2-1.2.8\include',
						'src\dependencies\curl-7.25.0\include',
						'src\dependencies\mysql-5.1.58\include',
						'src\dependencies\mm-1.4.2',
						'src\dependencies'])

# Add back when we are dealing with it.
						#'src\dependencies\libmemcached-0.44',
						#'src\dependencies\libmemcached-0.44\libmemcached',


env.Append(CXXFLAGS = ['-DWIN32', '/GR-', '/Gy'])
env.Append(CCFLAGS = ['-DWIN32'])
env.Append(CPPDEFINES = ['WIN32', 'NONDLL', 'USE_WINSOCK', 'CURL_STATICLIB'])
# env.Append(LINKFLAGS =  ['/INCREMENTAL:NO', '/NXCOMPAT', '/IGNORE:4221', '/MACHINE:X64'])
env.Append(LINKFLAGS =  ['/INCREMENTAL:NO', '/NXCOMPAT', '/IGNORE:4221', '/VERBOSE:LIB'])
env.Append(CCPDBFLAGS = ['/Zi'])

env.Append(LIBS = ['ws2_32', 'Advapi32.lib', 'Mswsock.lib', 'wldap32.lib', 'Psapi.lib', 'winmm', 'v8', 'sqlite3', 'bgd_a', 'libjpeg', 'libpng', 'libcurl', 'zlib', 'libssh', 'libeay32', 'libmysql', 'mmlib'])

env.Append(LIBPATH = [	'v8',
						'src\dependencies\sqlite-autoconf-3071100', 
						'src\dependencies\libgd2_2.0.36', 
						'src\dependencies\jpeg-8b',
						'src\dependencies\lpng1510',
						'src\dependencies\zlib-1.2.3',
						'src\dependencies\libssh2-1.2.8',
						'src\dependencies\curl-7.25.0',
						'src\dependencies\mm-1.4.2',
						'src\dependencies\openssl-0.9.8k_WIN32\lib',
						'C:\Program Files\MySQL\MySQL Server 5.5\lib'])

# Taken from the Unix build
# CORE=	main.o base64.o global.o console.o process.o net.o fs.o buffer.o v8.o http.o md5.o popen.o linenoise.o
core = ["src/main.cpp", "src/base64.cpp", "src/global.cpp", "src/console.cpp", "src/process.cpp", 
		"src/net.cpp", "src/fs.cpp", "src/buffer.cpp", "src/http.cpp", "src/md5.cpp", 
		"src/v8.cpp", "src/popen.cpp", "src/linenoise.cpp"]

# Taken from the Unix build
# OBJ=	mysql.o memcached.o gd.o ncurses.o sem.o logfile.o sqlite3.o xhrhelper.o curl.o ssh2.o sftp.o ftp.o ftplib.o editline.o
obj = [ core,	"src/mysql.cpp", "src/gd.cpp", "src/ncurses.cpp", 
				"src/sem.cpp", "src/logfile.cpp", "src/sqlite3.cpp", "src/xhrhelper.cpp", 
				"src/curl.cpp", "src/ssh2.cpp", "src/sftp.cpp", "src/ftp.cpp", "src/ftplib.cpp", 
				"src/editline.cpp" ]

srclst = [core, obj]

env.Program(target = "silkJS", source = srclst)

# To add at a later date. The function calls in these files have no direct or similar windows calls 
# to them so i will need to revist after the project is done.
# "src/ncurses.cpp", DONE
# "src/memcached.cpp", 
