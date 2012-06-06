/**
 * @module builtin/fs
 * 
 * ### Synopsis
 * SilkJS builtin fs object.
 * 
 * ### Description
 * The builtin/fs object provides constants and methods to directly access the underlying operating system's file system functions.  
 * 
 * ### Usage
 * var fs = require('builtin/fs');
 * 
 * ### See Also
 * Operating system man pages
 * 
 */
#include "SilkJS.h"
#include "posixstat.h"
#ifndef WIN32
#include <sys/mman.h>
#endif
// TODO:

/**
 * @function fs.error
 * 
 * ### Synopsis
 * Returns string version of last OS error.
 * 
 * ### Usage:
 * var message = fs.error();
 * 
 * @return {string} message - error message.
 */
static JSVAL fs_error (JSARGS args) {
    HandleScope scope;
    return scope.Close(String::New(strerror(errno)));
}

/**
 * @function fs.chdir
 * 
 * ### Synopsis
 * 
 * Change current working directory.
 * 
 * ### Usage:
 * var success = fs.chdir(dirPath);
 * 
 * @param {string} dirPath path in filesystem to set directory to.
 * @return {int} success - 0 on success, or -1 if error occurred.
 */
static JSVAL fs_chdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value dir(args[0]->ToString());
	int iretval = 0;

#ifdef WIN32
	iretval = _chdir(*dir);
#else
	iretval = chdir(*dir);
#endif

    return scope.Close(Integer::New(iretval));
}

/**
 * @function fs.getcwd
 * 
 * ### Synopsis
 * 
 * var path = fs.getcwd();
 * 
 * Get current working directory.
 * 
 * @return {string} current working directory path or null if error occurred.
 */
static JSVAL fs_getcwd (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	char *cwd = _getcwd(NULL, 0);
#else
    char *cwd = getcwd(NULL, 0);
#endif

    Handle<String>s = String::New(cwd);
    delete[] cwd;
    return scope.Close(s);
}

/**
 * @function fs.open
 * 
 * ### Synopsis
 * var fd = (filename, flags, mode);
 * 
 * @param {string} filename - name of file to open.
 * @param {int} flags - flags passed to open(2).
 * @param {int} mode - file permissions for file.
 * @return {int} fd - OS file handle, or -1 if error occurred.
 */
static JSVAL fs_open (JSARGS args) {
    HandleScope scope;
    String::Utf8Value filename(args[0]->ToString());
	int iretval;
    int flags = args[1]->IntegerValue();
#ifdef WIN32
	// I'm not sure how the file permissions match up with the Unix version.
	// the mode may need to be remapped.
	int mode = args[2]->IntegerValue();  
	iretval = _open(*filename, flags, mode);
#else
    mode_t mode = args[2]->IntegerValue();
	iretval = open(*filename, flags, mode);
#endif
    return scope.Close(Integer::New(iretval));
}

/**
 * @function fs.close
 * 
 * ### Synopsis
 * 
 * var success = fs.close(fd);
 * 
 * Close an already open file descriptor.
 * 
 * @param {int} fd - the file descriptor to close.
 * @return {int} success - 0 on success, or -1 if error occurred.
 * 
 */
static JSVAL fs_close (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
	int iretval;

#ifdef WIN32
	iretval = _close(fd);
#else
	iretval = close(fd);
#endif
    return scope.Close(Integer::New(iretval));
}

/**
 * @function fs.flock
 * 
 * ### Synopsis
 * 
 * var success = fs.flock(fd, operation);
 * 
 * Apply or remove an advisory lock on the open file specified by fd.
 * 
 * ### Operations
 * The operation parameter may be one of the following values:
 * fs.LOCK_SH - Obtain a shared lock.  More than one process may hold a shared lock for a file at a given time.
 * fs.LOCK_EX - Obtain an exclusive lock.  Only one process may hold an exclusive lock at a given time.
 * fs.LOCK_UN - Release an existing lock held by this process.
 *
 * Windows Operations
 * The operation parameter may be one of the following values on windows:
 * _LK_LOCK  - Obtain an exclusive lock.  Only one process may hold an exclusive lock at a given time.
 * _LK_UNLCK - Release an existing lock held by this process.
 * 
 * A call to fs.flock() will block if an incompatible lock is held by another process.
 * 
 * @param {int} fd - file descriptor of open file to lock or unlock.
 * @param {int} operation - see details above, and constants section.
 * @return {int} success - 0 on success, or -1 if error occurred.
 * 
 * 
 */
static JSVAL fs_flock (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    int operation = args[1]->IntegerValue();
	int iretval;
#ifdef WIN32
	// On Unix, flock() is advisory , meaning that the OS is free to ignore it. On Windows, flock() is 
	// mandatory, meaning that files are locked by the OS whether you ask for it or not! 
	iretval = (_locking(fd, operation, _filelength(fd)) == 0)? 0 : -1;
#else
	iretval = flock(fd, operation);
#endif
    return scope.Close(Integer::New(iretval));
}

/**
 * @function fs.lockf
 * 
 * ### Synopsis
 * 
 * var success = fs.lockf(fd, operation);
 * 
 * Acquire, test, or release a POSIX lock on an open file.
 * 
 * ### Operations
 * The operation parameter may be one of the following:
 * fs.F_LOCK - set an exclusive lock on the specified file.
 * fs.F_TLOCK - same as F_LOCK but call never blocks and returns an error instead if the file is already locked.
 * fs.F_ULOCK - unlock the specified file.
 * fs.F_TEST - test the lock: return 0 if the file is unlocked or locked by this process, or -1 if another process holds a lock.
 *
 * Windows Operations Will be the same as flock().
 * The operation parameter may be one of the following values on windows:
 * _LK_LOCK    - Obtain an exclusive lock.  Only one process may hold an exclusive lock at a given time.
 * _LK_UNLCK   - Release an existing lock held by this process.
 * _LK_NBRLCK  - same as _LK_LOCK but call never blocks and returns an error instead if the file is already locked.
 * 
 * @param {int} fd - file descriptor of open file to lock or unlock.
 * @param {int} operation - see details above, and constants section.
 * @return {int} success - 0 on success, or -1 if error occurred.
 * 
 * ### Notes
 * lockf() is not fully implemented by SilkJS.  The function allows locking of regions of files.  We don't typically seek() and modify bits of files in place.  This is something to revisit if binary/b specification is implemented.
 */
static JSVAL fs_lockf (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();
    int operation = args[1]->IntegerValue();
	int iretval;
#ifdef WIN32
	// On Unix, flock() is advisory , meaning that the OS is free to ignore it. On Windows, flock() is 
	// mandatory, meaning that files are locked by the OS whether you ask for it or not! 
	iretval = (_locking(fd, operation, _filelength(fd)) == 0)? 0 : -1;
#else
	iretval = lockf(fd, operation, 0);
#endif
    return scope.Close(Integer::New(iretval));
}

/**
 * @function fs.rename
 * 
 * ### Synopsis
 * 
 * var success = fs.rename(oldpath, newpath);
 * 
 * Renames a file or directory, moving it between directories if required.  
 * 
 * This is the equivalent of the shell `mv` command.
 * 
 * @param {string} oldpath - name of existing file or directory to be moved
 * @param {string} newpath - path to where the file or directory is to be moved
 * @return {int} success - 0 on success, or -1 if error occurred
 */
static JSVAL fs_rename (JSARGS args) {
    HandleScope scope;
    String::Utf8Value oldpath(args[0]->ToString());
    String::Utf8Value newpath(args[1]->ToString());
    return scope.Close(Integer::New(rename(*oldpath, *newpath)));
}


int _wintruncate(const char *path, off_t length)
{
	int fh, result = 0;
	unsigned int nbytes = BUFSIZ;

  /* This replicates the use of truncate */
#ifdef WIN32
  if (_sopen_s(&fh, path, _O_RDWR, _SH_DENYWR, _S_IREAD) != 0)
#else
  if (_sopen_s(&fh, path, _O_RDWR) == 0)
#endif
  {
     if (( result = _chsize(fh, length)) != 0)
		result = -1;
     _close(fh);
  } else result = -1;
  return result;
}

/**
 * @function fs.truncate
 * 
 * ### Synopsis
 * var success = fs.truncate(fd, length);
 * var success = fs.truncate(path, length);
 * 
 * The specified file will be truncated to a size of precisely length bytes.
 * 
 * @param {int} fd - file descriptor of open file to truncate... or
 * @param {string} path - full path to file to truncate.
 * @param {int} length - resulting length of file after truncate.
 * @return {int} success - 0 on success, or -1 if error occurred.
 */
static JSVAL fs_truncate (JSARGS args) {
    HandleScope scope;
	int iresult;
    off_t len = args[1]->IntegerValue();
    if (args[0]->IsString()) {
        String::Utf8Value path(args[0]->ToString());
#ifdef WIN32
		iresult = _wintruncate(*path, len);
#else
		iresult = truncate(*path, len);
#endif
        return scope.Close(Integer::New(iresult));
    }
    else {
        int fd = args[0]->IntegerValue();
#ifdef WIN32
		iresult = _chsize(fd, len);
#else
		iresult = ftruncate(fd, len);
#endif
        return scope.Close(Integer::New(iresult));
    }
}

int _winfchmod(const char *path, int mode)
{
	int fh, result = 0;
	unsigned int nbytes = BUFSIZ;

  /* This replicates the use of truncate */
#ifdef WIN32
	_chmod(path, mode);
#else
  if (_sopen_s(&fh, path, _O_RDWR) == 0)
  {
     if (( result = chmod(fh, mode)) != 0)
		result = -1;
     _close(fh);
  } else result = -1;
#endif
  return result;
}

/**
 * @function fs.chmod
 * 
 * ### Synopsis
 * var success = fs.chmod(fd, mode);
 * var success = fs.chmod(path, mode);
 * 
 * Changes the permissions of the specified file.
 * 
 * ### Modes
 * The mode parameter may be one or more of the following values, or'ed together (a bit mask):
 * fs.S_ISUID - set UID bit
 * fs.S_ISGID - set group id bit
 * fs.S_ISVTX - sticky bit
 * fs.S_IRUSR - owner has read permission
 * fs.S_IWUSR - owner has write permission
 * fs.S_IXUSR - owner has execute permission
 * fs.S_IRGRP - group has read permission
 * fs.S_IWGRP - group has write permission
 * fs.S_IXGRP - group has execute permission
 * fs.S_IROTH - others have read permission
 * fs.S_IWOTH - others have write permission
 * fs.S_IXOTH - others have execute permission
 * 
 * @param {int} fd - file descriptor of open file to change permissions for... or
 * @param {string} path - full path to file to change permissions for.
 * @param {int} mode - the resulting file permissions.  See description of modes above.
 * @return {int} success - 0 on success, or -1 if error occurred.
 */
static JSVAL fs_chmod (JSARGS args) {
    HandleScope scope;
	int iresult;
#ifdef WIN32
	int mode = args[1]->IntegerValue();
#else
    mode_t mode = args[1]->IntegerValue();
#endif

    if (args[0]->IsString()) {
        String::Utf8Value path(args[0]->ToString());
        return scope.Close(Integer::New(chmod(*path, mode)));
    }
    else {
        int fd = args[0]->IntegerValue();
#ifdef WIN32
	// Need to make an equivalent function.
	iresult = 0;
#else
	iresult = fchmod(fd, mode);
#endif
        return scope.Close(Integer::New(iresult));
    }
}

#ifdef WIN32
static JSOBJ format_stat (struct _stat64 &buf) {
#else
static JSOBJ format_stat (struct stat &buf) {
#endif
    HandleScope scope;
    JSOBJ o = Object::New();
    o->Set(String::New("dev"), Integer::New(buf.st_dev));
    o->Set(String::New("ino"), Integer::New(buf.st_ino));
    o->Set(String::New("mode"), Integer::New(buf.st_mode));
    o->Set(String::New("nlink"), Integer::New(buf.st_nlink));
    o->Set(String::New("uid"), Integer::New(buf.st_uid));
    o->Set(String::New("gid"), Integer::New(buf.st_gid));
    o->Set(String::New("rdev"), Integer::New(buf.st_rdev));
    o->Set(String::New("size"), Integer::New(buf.st_size));
#ifndef WIN32
    o->Set(String::New("blksize"), Integer::New(buf.st_blksize));
    o->Set(String::New("blocks"), Integer::New(buf.st_blocks));
#endif
    o->Set(String::New("atime"), Integer::New(buf.st_atime));
    o->Set(String::New("mtime"), Integer::New(buf.st_mtime));
    o->Set(String::New("ctime"), Integer::New(buf.st_ctime));
    return scope.Close(o);
}

/**
 * @function fs.stat
 * 
 * ### Synopsis
 * 
 * var o = fs.stat(path);
 * 
 * Get a structure describing a file's status.
 * 
 * ### Status structure
 * The object returned by this function has the following members:
 * dev: ID of device containing the file.
 * ino: inode number.
 * mode: file permissions/protection.
 * nlink: number of hard links.
 * uid - user ID of owner.
 * gid: group ID of owner.
 * rdev: device ID (if special file)
 * size: total size of file in bytes.
 * blksize: block size for file system I/O.
 * blocks: number of 512B blocks allocated.
 * atime: timestamp of last access.
 * mtime: timestamp of last modification.
 * ctime: timestamp of last status change.
 * 
 * @param {string} path - path to file to get status for.
 * @return {object} o - status structure described above, or null if error occurred.
 * 
 * ### Notes
 * It is a bit more expensive to call this function if you are only interested in one of the fields.  This is because the entire result status object must be constructed.  SilkJS provides faster convenience methods to obtain the size, type, etc., of a file path.
 */
static JSVAL fs_stat (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
#ifdef WIN32
    struct _stat64 buf;
	if (_stat64(*path, &buf) == -1) {
#else
    struct stat buf;
	if (stat(*path, &buf) == -1) {
#endif
        return scope.Close(Null());
    }
    return scope.Close(format_stat(buf));
}

/**
 * @function fs.lstat
 * 
 * ### Synopsis
 * var o = fs.lstat(path);
 * 
 * This function is identical to fs.stat() except that if path is a symbolic link, then the status of the link itself is returned rather than the file it refers to. 
 * 
 * @param {string} path - path to file to get status for.
 * @return {object} o - status structure described for fs.stat(), or null if error occurred.
 */
static JSVAL fs_lstat (const Arguments & args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

#ifdef WIN32
	struct _stat64 buf;
	if (_stat64(*path, &buf) == -1) {
#else
    struct stat buf;
    if (lstat(*path, &buf) == -1) {
	
#endif
        return scope.Close(False());
    }
    return scope.Close(format_stat(buf));
}

/**
 * @function fs.lstat
 * 
 * ### Synopsis
 * var o = fs.fstat(fd);
 * 
 * This function is identical to fs.stat() except that the file get status for is specified by a file descriptor.
 * 
 * @param {int} fd - file descriptor of an open file to get status for.
 * @return {object} o - status structure described for fs.stat(), or null if error occurred.
 */
static JSVAL fs_fstat (JSARGS args) {
    HandleScope scope;
    int fd = args[0]->IntegerValue();

#ifdef WIN32
	struct _stati64 buf;
    if (_fstat64(fd, &buf) == -1) {
#else
    struct stat buf;
    if (fstat(fd, &buf) == -1) {
#endif
        return scope.Close(False());
    }
    return scope.Close(format_stat(buf));
}

/**
 * function fs.exists
 * 
 * ### Synopsis
 * 
 * var doesExist = fs.exists(path);
 * 
 * This function tests to see if the specified path exists in the file system.
 * 
 * Calling this function is faster than calling fs.stat() to determine if a file or directory exists.
 * 
 * @param {string} path - path of file
 * @return {boolean} doesExist - true if file exists, false if not.
 */
static JSVAL fs_exists (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    struct stat buf;
    if (stat(*path, &buf)) {
        return scope.Close(False());
    }
    return scope.Close(True());
}

/**
 * function fs.isFile
 * 
 * ### Synopsis
 * 
 * var is_a_file = fs.isFile(path);
 * 
 * This function tests to see if the specified path exists and is a regular file in the file system.
 * 
 * Calling this function is faster than calling fs.stat() to determine if a path is a file.
 * 
 * @param {string} path - path of file
 * @return {boolean} is_a_file - true if path exists and is a regular file, false if not.
 */
static JSVAL fs_isfile (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    struct stat buf;
    if (stat(*path, &buf)) {
        return scope.Close(False());
    }
    if (S_ISREG(buf.st_mode)) {
        return scope.Close(True());
    }
    else {
        return scope.Close(False());
    }
}

/**
 * function fs.isDir
 * 
 * ### Synopsis
 * 
 * var is_a_directory = fs.isDir(path);
 * 
 * This function tests to see if the specified path exists and is a directory in the file system.
 * 
 * Calling this function is faster than calling fs.stat() to determine if a path is a directory.
 * 
 * @param {string} path - path of file
 * @return {boolean} is_a_directory - true if path exists and is a directory, false if not.
 */
static JSVAL fs_isdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    struct stat buf;
    if (stat(*path, &buf)) {
        return scope.Close(False());
    }
    if (S_ISDIR(buf.st_mode)) {
        return scope.Close(True());
    }
    else {
        return scope.Close(False());
    }
}

/**
 * function fs.fileSize
 * 
 * ### Synopsis
 * 
 * var size = fs.fileSize(path);
 * 
 * This function returns the size of a file in bytes.
 * 
 * Calling this function is faster than calling fs.stat() to get the size of a file.
 * 
 * @param {string} path - path of file
 * @return {int} size - size of file in bytes, or false if an error occurred.
 */
static JSVAL fs_filesize (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    struct stat buf;
    if (stat(*path, &buf)) {
        return scope.Close(False());
    }
    return scope.Close(Integer::New(buf.st_size));
}

/**
 * function fs.fileModified
 * 
 * ### Synopsis
 * 
 * var timestamp = fs.fileModified(path);
 * 
 * This function returns the timestamp when item in the file system with the given path was last modified.
 * 
 * Calling this function is faster than calling fs.stat() to get the modification time of a file.
 * 
 * @param {string} path - path of file
 * @return {int} timestamp - modification time as timestamp, or false if an error occurred.
 */
static JSVAL fs_mtime (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    struct stat buf;
    if (stat(*path, &buf)) {
        return scope.Close(False());
    }
    return scope.Close(Integer::New(buf.st_mtime));
}

/**
 * @function fs.link
 * 
 * ### Synopsis
 * 
 * var success = fs.link(oldpath, newpath);
 * 
 * Atomically create a hard link in the filesystem.  The file specified by newpath argument will be created linked to the file specified by oldpath.
 * 
 * Both newpath and oldpath must be located in the same file system.  If oldpath is a symbolic link, newpath will refer to the file referred to by the symbolic link.
 * 
 * @param {string} oldpath - path in filesystem to existing file
 * @param {string} newpath - path in filesysstem where the hard link will be created.
 * @return {int} success - 0 if successful, -1 if an error occurred.
 */
static JSVAL fs_link (JSARGS args) {
    HandleScope scope;
	int iresult = 0;
    String::Utf8Value oldpath(args[0]->ToString());
    String::Utf8Value newpath(args[1]->ToString());

#ifdef WIN32
	if (CreateHardLink(*oldpath, 
					   *newpath, 
					   NULL // reserved, must be NULL
					  ) == 0)
		iresult = GetLastError();

#else
	iresult = link(*oldpath, *newpath)
#endif
    return scope.Close(Integer::New(iresult));
}

/**
 * @function fs.symlink
 * 
 * ### Synopsis
 * 
 * var success = fs.symlink(oldpath, newpath);
 * 
 * A symbolic link newpath is created to oldpath.  As far as symbolic links are concerned, oldpath doesn't have to exist, and the files need not be on the same file system.
 * 
 * @param {string} oldpath - destination for soft/symbolic link.
 * @param {string} newpath - path to where the link will physically be created
 * @return {int} success - 0 if successful, -1 if an error occurred.
 */
static JSVAL fs_symlink (JSARGS args) {
    HandleScope scope;
	int iresult;
    String::Utf8Value oldpath(args[0]->ToString());
    String::Utf8Value newpath(args[1]->ToString());

#ifdef WIN32
	// As per the comment
	if (!CreateSymbolicLinkA(*oldpath, *newpath, 0x0))
		iresult = GetLastError();
#else
	iresult = symlink(*oldpath, *newpath);
#endif
    return scope.Close(Integer::New(iresult));
}

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/**
 * @function fs.readlink
 * 
 * ### Synopsis
 * 
 * var actual_path = fs.readlink(path_of_link);
 * 
 * This function returns the content of the symbolic link path_of_link.
 * 
 * @param {string} path_of_link - path in the file system of the link to be read
 * @return {string} actual_path - path that the link points to, or false if error.
 */
static JSVAL fs_readlink (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

    char pathBuf[PATH_MAX];
#ifdef WIN32
	WIN32_FIND_DATA lpFindFileData;
	HANDLE hFind;
	hFind = FindFirstFile(*path, &lpFindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
        return scope.Close(False());
    }
	return scope.Close(String::New(lpFindFileData.cFileName));
#else
    ssize_t size = readlink(*path, pathBuf, PATH_MAX);
    if (size < 0) {
        return scope.Close(False());
    }
    return scope.Close(String::New(pathBuf));
#endif
}

/**
 * @function fs.realpath
 * 
 * ### Synopsis
 * 
 * var real_path = fs.realpath(path);
 * 
 * This function returns the canonicalized absolute path name of the specified path.
 * 
 * All symbolic links, extra "/" characters, and references to "/./" and "/../" are resolved in the returned path.  Both absolute and relative paths are resolved.  All components of the input path must exist when this function is called.
 * 
 * @param {string} path - path to resolve to absolute path.
 * @return {string} real_path - absolute path, or false if an error occurred.
 */
static JSVAL fs_realpath (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());

#ifdef WIN32
	char *absolutePath = NULL;
	int isize = GetFullPathName(*path, 0, absolutePath, NULL);
	absolutePath = (char*) calloc(isize+1, sizeof(char));
	GetFullPathName(*path, isize, absolutePath, NULL);
#else
    char *absolutePath = realpath(*path, NULL);
#endif
    if (!absolutePath) {
        return scope.Close(False());
    }
    Handle<String>s = String::New(absolutePath);
    free(absolutePath);
    return scope.Close(s);
}

/**
 * @function fs.unlink
 * 
 * ### Synopsis
 * 
 * var success = fs.unlink(path);
 * 
 * This function removes the specified entry in the file system.  If the path is to a hard link, the destination file is not removed.
 * 
 * A file is technically not completely removed if other processes have the file open.  The file will be removed when those processes close the file.
 * 
 * @param {string} path - path of file system entry to be removed.
 * @return {boolean} success - true if the link/file was removed, false if an error occurred.
 */
static JSVAL fs_unlink (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    if (unlink(*path) == -1) {
        return scope.Close(False());
    }
    return scope.Close(True());
}

/**
 * @function fs.rmdir
 * 
 * ### Synopsis
 * 
 * var success = fs.rmdir(path);
 * 
 * This function removes the specified directory from the file system.  The directory must not have any entries other than "." and "..".
 * 
 * @param {string} path - path to directory to be removed.
 * @return {boolean} success - true if the directory was removed, false if there was an error.
 */
static JSVAL fs_rmdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    if (rmdir(*path) == -1) {
        return scope.Close(False());
    }
    return scope.Close(True());
}

/**
 * @function fs.mkdir
 * 
 * ### Synopsis
 * 
 * var success = fs.mkdir(path, mode);
 * var success = fs.mkdir(path);
 * 
 * The directory specified by path is created with the given mode.  If mode is not provided, then 0700 is used.  The directory will be owned by the process's effective user ID, and the directory's group ID is set to that of the parent directory in which it is created.
 * 
 * ### Modes
 * The mode parameter may be one or more of the following values, or'ed together (a bit mask):
 * fs.S_IRUSR - owner has read permission
 * fs.S_IWUSR - owner has write permission
 * fs.S_IXUSR - owner has execute permission
 * fs.S_IRGRP - group has read permission
 * fs.S_IWGRP - group has write permission
 * fs.S_IXGRP - group has execute permission
 * fs.S_IROTH - others have read permission
 * fs.S_IWOTH - others have write permission
 * fs.S_IXOTH - others have execute permission
 * 
 * Note that only these 9 bits are used.  The function's behavior when attempting to set other bits is undefined.
 * 
 * @param {string} path - absolute or relative path of directory to be created.
 * @param {int} mode - some combination of the mode mask bits.  See above.
 * @return {boolean} success - true if the directory was created, false if an error occurred.
 */
static JSVAL fs_mkdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
#ifdef WIN32
    if (mkdir(*path) == -1) {
#else
    mode_t mode = 0755;
    if (args.Length() > 1) {
        mode = args[1]->IntegerValue();
    }
    if (mkdir(*path, mode) == -1) {
#endif
        return scope.Close(False());
    }
    return scope.Close(True());
}

/**
 * @function fs.readDir
 * 
 * ### Synopsis
 * 
 * var filenames = fs.readDir(path);
 * 
 * This function returns an array of file (or directory) names in the specified path (directory).  
 * 
 * Only the file names are returned, no other information.  The special entries "." and ".." are not returned.
 * 
 * @param {string} path - absolute or relative path to a directory to get the contents of.
 * @return {array} filenames - array of {string} filenames, or null if the directory oculd not be opened.
 */
#ifdef WIN32
static JSVAL fs_readdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
	WIN32_FIND_DATA FData;

	HANDLE hFind = FindFirstFile(*path, &FData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return scope.Close(Null());
    }
    Handle<Array>a = Array::New();
    int ndx = 0;
	do {
		
        if (strcmp(FData.cFileName, ".") && strcmp(FData.cFileName, "..")) {
            a->Set(ndx++, String::New(FData.cFileName));
		}
    } while (FindNextFile(hFind, &FData));

    FindClose(hFind);
    return scope.Close(a);
}
#else
static JSVAL fs_readdir (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    DIR *d = opendir(*path);
    if (!d) {
        return scope.Close(Null());
    }
    Handle<Array>a = Array::New();
    int ndx = 0;
    while (struct dirent * e = readdir(d)) {
        if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
            a->Set(ndx++, String::New(e->d_name));
        }
    }
    closedir(d);
    return scope.Close(a);
}
#endif

/**
 * @function fs.readFile
 * 
 * ### Synopsis
 * 
 * var contents = fs.readFile(filename);
 * 
 * This function reads the entire contents of the specified file into a string.
 * 
 * @param {string} filename - name of file to read.
 * @return {string} contents - content of the file, or null if an error occurred.
 * 
 * ### Notes
 * This function reads 1024 bytes from the file at a time.  This is not optimal for big files, but minimizes the amount of memory used by the process.
 */
#ifdef WIN32
static JSVAL fs_readfile (JSARGS args) {
    HandleScope scope;
	OVERLAPPED olap;
    String::Utf8Value path(args[0]->ToString());
    //int fd = open(*path, O_RDONLY);
	HANDLE fh = CreateFile(*path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        return scope.Close(Null());
    }

	// Get the file size limits.
	LARGE_INTEGER LI;
	memset(&LI, 0, sizeof(LI));
	BOOL retval = ::GetFileSizeEx(fh, &LI);
	if(retval == 0) {
		return scope.Close(Null());
	}

	// Set overlapped structure.
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = NULL;
	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.Pointer = NULL;
	ov.Internal = 0;
	ov.InternalHigh = 0;

    //flock(fd, LOCK_SH);
	LockFileEx(fh, 0, 0, LI.LowPart, LI.HighPart, &ov);

    //lseek(fd, 0, 0);
	SetFilePointer(fh, 0, NULL, FILE_BEGIN);
    std::string s;

    //	long size = lseek(fd, 0, 2); lseek(fd, 0, 0);
    //	printf("size = %ld\n", size);
    char buf[1024];
    ssize_t count;
	DWORD iread;

    //while ((count = read(fd, buf, 1024))) {
	while( ReadFile(fh, &buf, 1024, &iread, NULL) && (iread != 0) ) {
        s = s.append(buf, iread);
    }

    //	if (read(fd, buf, size) != size) {
    //		return scope.Close(Null());
    //	}
    //flock(fd, LOCK_UN);
	UnlockFileEx(fh, 0, LI.LowPart, LI.HighPart, &ov);
    //close(fd);

	CloseHandle(fh);
    Handle<String>ret = String::New(s.c_str(), s.size());
    return scope.Close(ret);
}
#else
static JSVAL fs_readfile (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    int fd = open(*path, O_RDONLY);
    if (fd == -1) {
        return scope.Close(Null());
    }
    flock(fd, LOCK_SH);
    lseek(fd, 0, 0);
    std::string s;
    //	long size = lseek(fd, 0, 2); lseek(fd, 0, 0);
    //	printf("size = %ld\n", size);
    char buf[1024];
    ssize_t count;
    while ((count = read(fd, buf, 1024))) {
        s = s.append(buf, count);
    }
    //	if (read(fd, buf, size) != size) {
    //		return scope.Close(Null());
    //	}
    flock(fd, LOCK_UN);
    close(fd);
    Handle<String>ret = String::New(s.c_str(), s.size());
    return scope.Close(ret);
}
#endif
/**
 * @function fs.readFile64
 * 
 * ### Synopsis
 * 
 * var contents = fs.readFile64(filename);
 * 
 * This function reads the entire contents of the specified file into a base64 encoded string.  JavaScript does not have a native binary type, so we have to use strings.
 * 
 * There are a number of API methods implemented for SilkJS that deal with base64 encoding of binary data.  The converse of this function, for example, is fs.writeFile64().
 * 
 * @param {string} filename - name of file to read.
 * @return {string} contents - content of the file, or null if an error occurred.
 * 
 * ### Notes
 * At some point, binary/b will be implemented in SilkJS and additional methods for dealing with binary data will be implemented.
 */
#ifdef WIN32
static JSVAL fs_readfile64 (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    //int fd = open(*path, O_RDONLY);
	HANDLE fh = CreateFile(*path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        printf("%s\n%s\n", *path, strerror(errno));
        return scope.Close(Null());
    }
    //flock(fd, LOCK_SH);
	// Get the file size limits.
	LARGE_INTEGER LI;
	memset(&LI, 0, sizeof(LI));
	BOOL ret = ::GetFileSizeEx(fh, &LI);
	if(ret == 0) {
		return scope.Close(Null());
	}

	// Set overlapped structure.
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = NULL;
	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.Pointer = NULL;
	ov.Internal = 0;
	ov.InternalHigh = 0;

	LockFileEx(fh, 0, 0, LI.LowPart, LI.HighPart, &ov);

    //long size = lseek(fd, 0, 2);
	long size = SetFilePointer(fh, 2, NULL, FILE_BEGIN);
    unsigned char* buf = (unsigned char*) new unsigned char[size];
	DWORD iread;
    //lseek(fd, 0, 0);
	SetFilePointer(fh, 0, NULL, FILE_BEGIN);
    //if (read(fd, buf, size) != size) {
	ReadFile(fh, buf, size, &iread, NULL);
	if (iread != size) {
        return scope.Close(Null());
    }
    //close(fd);
	CloseHandle(fh);
    string encoded = Base64Encode(buf, size);
    Handle<String>s = String::New(encoded.c_str(), encoded.size());
    return scope.Close(s);
}
#else
static JSVAL fs_readfile64 (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    int fd = open(*path, O_RDONLY);
    if (fd == -1) {
        printf("%s\n%s\n", *path, strerror(errno));
        return scope.Close(Null());
    }
    flock(fd, LOCK_SH);
    long size = lseek(fd, 0, 2);
    lseek(fd, 0, 0);
    unsigned char buf[size];
    if (read(fd, buf, size) != size) {
        return scope.Close(Null());
    }
    close(fd);
    string encoded = Base64Encode(buf, size);
    Handle<String>s = String::New(encoded.c_str(), encoded.size());
    return scope.Close(s);
}
#endif

/**
 * @function fs.writeFile
 * 
 * ### Synopsis
 * 
 * var success = fs.writeFile(filename, contents);
 * var success = fs.writeFile(filename, contents, mode);
 * 
 * This function creates or overwrites the file specified by filename with the given contents and mode.  If mode is not provided, 0644 is used.
 * 
 * ### Modes
 * The mode parameter may be one or more of the following values, or'ed together (a bit mask):
 * fs.S_ISUID - set UID bit
 * fs.S_ISGID - set group id bit
 * fs.S_ISVTX - sticky bit
 * fs.S_IRUSR - owner has read permission x
 * fs.S_IWUSR - owner has write permission x
 * fs.S_IXUSR - owner has execute permission
 * fs.S_IRGRP - group has read permission  x
 * fs.S_IWGRP - group has write permission
 * fs.S_IXGRP - group has execute permission
 * fs.S_IROTH - others have read permission x
 * fs.S_IWOTH - others have write permission
 * fs.S_IXOTH - others have execute permission
 * 
 * @param {string} filename - name of file to read.
 * @param {string} contents - content of the file.
 * @param {int} mode - mode that the file will have after the contents are written.  See above.
 */
#ifdef WIN32
static JSVAL fs_writefile (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    String::Utf8Value data(args[1]->ToString());
    ssize_t size;
    if (args.Length() > 2) {
        size = args[2]->IntegerValue();
    }
    else {
        size = strlen(*data);
    }
    //mode_t = 0644;
	int mode;
    if (args.Length() > 3) {
        mode = args[3]->IntegerValue();
    }
    //int fd = open(*path, O_WRONLY | O_CREAT | O_TRUNC, mode);
	HANDLE fh = CreateFile(*path, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == NULL) {
        return scope.Close(False());
    }
	// Get the file size limits.
	LARGE_INTEGER LI;
	memset(&LI, 0, sizeof(LI));
	BOOL ret = ::GetFileSizeEx(fh, &LI);
	if(ret == 0) {
		return scope.Close(Null());
	}

	// Set overlapped structure.
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = NULL;
	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.Pointer = NULL;
	ov.Internal = 0;
	ov.InternalHigh = 0;
    //flock(fd, LOCK_EX);
	LockFileEx(fh, 0, 0, LI.LowPart, LI.HighPart, &ov);

	DWORD iwritten;
    //if (write(fd, *data, size) != size) {
	WriteFile(fh, *data, size, &iwritten, NULL);
	if (iwritten != size) {
        //flock(fd, LOCK_UN);
		UnlockFileEx(fh, 0, LI.LowPart, LI.HighPart, &ov);
        //close(fd);
		CloseHandle(fh);
        return scope.Close(False());
    }
    //flock(fd, LOCK_UN);
	UnlockFileEx(fh, 0, LI.LowPart, LI.HighPart, &ov);
    //close(fd);
	CloseHandle(fh);
    return scope.Close(True());
}
#else
static JSVAL fs_writefile (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    String::Utf8Value data(args[1]->ToString());
    ssize_t size;
    if (args.Length() > 2) {
        size = args[2]->IntegerValue();
    }
    else {
        size = strlen(*data);
    }
    mode_t mode = 0644;
    if (args.Length() > 3) {
        mode = args[3]->IntegerValue();
    }
    int fd = open(*path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1) {
        return scope.Close(False());
    }
    flock(fd, LOCK_EX);
    if (write(fd, *data, size) != size) {
        flock(fd, LOCK_UN);
        close(fd);
        return scope.Close(False());
    }
    flock(fd, LOCK_UN);
    close(fd);
    return scope.Close(True());
}
#endif

/**
 * @function fs.writeFile64
 * 
 * ### Synopsis
 * 
 * var success = fs.writeFile64(filename, contents);
 * var success = fs.writeFile64(filename, contents, mode);
 * 
 * This function creates or overwrites the file specified by filename with the given contents and mode.  If mode is not provided, 0644 is used.
 *
 * The contents parameter is a base64 encoded string.  It is decoded to binary data as it is written to the file. 
 * 
 * There are a number of API methods implemented for SilkJS that deal with base64 encoding of binary data.  The converse of this function, for example, is fs.writeFile64().
 * 
 * ### Modes
 * The mode parameter may be one or more of the following values, or'ed together (a bit mask):
 * fs.S_ISUID - set UID bit
 * fs.S_ISGID - set group id bit
 * fs.S_ISVTX - sticky bit
 * fs.S_IRUSR - owner has read permission
 * fs.S_IWUSR - owner has write permission
 * fs.S_IXUSR - owner has execute permission
 * fs.S_IRGRP - group has read permission
 * fs.S_IWGRP - group has write permission
 * fs.S_IXGRP - group has execute permission
 * fs.S_IROTH - others have read permission
 * fs.S_IWOTH - others have write permission
 * fs.S_IXOTH - others have execute permission
 * 
 * @param {string} filename - name of file to read.
 * @param {string} contents - content of the file.
 * @param {int} mode - mode that the file will have after the contents are written.  See above.
 * 
 * ### Notes
 * At some point, binary/b will be implemented in SilkJS and additional methods for dealing with binary data will be implemented.
 */
#ifdef WIN32
static JSVAL fs_writefile64(JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    String::Utf8Value data(args[1]->ToString());
    int mode = 0644;

    if (args.Length() > 2) {
        mode = args[2]->IntegerValue();
    }

	DWORD iwritten;
    unsigned char *buf = new unsigned char[data.length()];
    int decoded = decode_base64(buf, *data);
    //int fd = open(*path, O_WRONLY | O_CREAT | O_TRUNC, mode);
	HANDLE fh = CreateFile(*path, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == NULL) {
        delete [] buf;
        return scope.Close(False());
    }
    //if (write(fd, buf, decoded) != decoded) {
	WriteFile(fh, buf, decoded, &iwritten, NULL);
	if (iwritten != decoded) {
        //close(fd);
		CloseHandle(fh);
        delete [] buf;
        return scope.Close(False());
    }
    //close(fd);
	CloseHandle(fh);
    delete [] buf;
    return scope.Close(True());
}
#else
static JSVAL fs_writefile64 (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    String::Utf8Value data(args[1]->ToString());
    mode_t mode = 0644;
    if (args.Length() > 2) {
        mode = args[2]->IntegerValue();
    }

    unsigned char *buf = new unsigned char[data.length()];
    int decoded = decode_base64(buf, *data);
    int fd = open(*path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1) {
        delete [] buf;
        return scope.Close(False());
    }
    if (write(fd, buf, decoded) != decoded) {
        close(fd);
        delete [] buf;
        return scope.Close(False());
    }
    close(fd);
    delete [] buf;
    return scope.Close(True());
}
#endif

/**
 * @function fs.copyFile
 * 
 * ### Synsopsis
 * 
 * var success = fs.copyFile(destination, source);
 * var success = fs.copyFile(destination, source, mode);
 * 
 * Copy file from source to destination.  If mode is omitted, destination file perms will be 0644.
 * 
 * @param {string) destination - destination path (file to be written).
 * @param (string} source - source path (file to be copied).
 * @param {int} mode - file permissions for destination after copy.
 * @return {boolean} success - true if file was copied successfully, false if an error occurred.
 */
#ifdef WIN32
static JSVAL fs_copyFile (JSARGS args) {
    HandleScope scope;
    String::Utf8Value destination(args[0]->ToString());
    String::Utf8Value source(args[1]->ToString());
    int mode = 0644;

    if (args.Length() > 2) {
        mode = args[2]->IntegerValue();
    }

    //int fdin, fdout;
    char *csrc, *cdst;
	HANDLE hsrc, hdst;
    struct stat statbuf;
    int		esave;
	DWORD iwritten;

    /* open the input file */
	HANDLE fdin = CreateFile(*source, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fdin != INVALID_HANDLE_VALUE) {
        return scope.Close(False());
    }

    /* open/create the output file */
	HANDLE fdout = CreateFile(*destination, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fdout == INVALID_HANDLE_VALUE) {
		CloseHandle(fdin);
        return scope.Close(False());
    }

	// Get the file size limits.
	DWORD fsize;
	fsize = GetFileSize(fdin, NULL);
	if(fsize == 0) {
        esave = errno;
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
	}

	SetFilePointer(fdout, fsize, NULL, FILE_BEGIN);

	WriteFile(fdout, "", 1, &iwritten, NULL);
	if (iwritten != 1) {
        esave = errno;
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
	}

    /* mmap the input file */
	if ((hsrc = CreateFileMapping(fdin, NULL, PAGE_READONLY, 0, fsize, NULL)) == NULL) {
        esave = errno;
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
    }
	if ((csrc = (char *) MapViewOfFile(hsrc, FILE_MAP_ALL_ACCESS, 0, 0, fsize)) == NULL)
	{
        esave = errno;
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
	}

    /* mmap the output file */
	if ((hdst = CreateFileMapping(fdout, NULL, PAGE_READWRITE, 0, fsize, NULL)) == NULL) {
        esave = errno;
		UnmapViewOfFile(csrc);
		CloseHandle(hsrc);
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
    }

	if ((cdst = (char *) MapViewOfFile(hdst, FILE_MAP_ALL_ACCESS, 0, 0, fsize)) == NULL)
	{
        esave = errno;
		UnmapViewOfFile(csrc);
		CloseHandle(hsrc);
        CloseHandle(fdin);
        CloseHandle(fdout);
        errno = esave;
        return scope.Close(False());
	}

    /* this copies the input file to the output file */
	CopyMemory(cdst, cdst, fsize);
    //memcpy(hdst, hsrc, statbuf.st_size);
	UnmapViewOfFile(hsrc);
	UnmapViewOfFile(hdst);
    CloseHandle(fdin);
    CloseHandle(fdout);
    //munmap(hsrc, statbuf.st_size);
    //munmap(hdst, statbuf.st_size);
    CloseHandle(fdin);
    CloseHandle(fdout);

    return scope.Close(True());
}
#else
static JSVAL fs_copyFile (JSARGS args) {
    HandleScope scope;

    String::Utf8Value destination(args[0]->ToString());
    String::Utf8Value source(args[1]->ToString());
    mode_t mode = 0644;
    if (args.Length() > 2) {
        mode = args[2]->IntegerValue();
    }

    int fdin, fdout;
    char *src, *dst;
    struct stat statbuf;
    int esave;

    /* open the input file */
    if ((fdin = open(*source, O_RDONLY)) < 0) {
	if (fh != NULL)
        return scope.Close(False());
    }

    /* open/create the output file */
    if ((fdout = open(*destination, O_RDWR | O_CREAT | O_TRUNC, mode)) < 0) {
	if (ofh == NULL) {
        close(fdin);
		fclose(fdin);
        return scope.Close(False());
    }

    /* find size of input file */
    if (fstat(fdin, &statbuf) < 0) {
        esave = errno;
        close(fdin);
        close(fdout);
        errno = esave;
        return scope.Close(False());
    }

    /* go to the location corresponding to the last byte */
    if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1) {
        esave = errno;
        close(fdin);
        close(fdout);
        errno = esave;
        return scope.Close(False());
    }

    /* write a dummy byte at the last location */
    if (write(fdout, "", 1) != 1) {
        esave = errno;
        close(fdin);
        close(fdout);
        errno = esave;
        return scope.Close(False());
    }

    /* mmap the input file */
    if ((src = (char *) mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0)) == (caddr_t) - 1) {
        esave = errno;
        close(fdin);
        close(fdout);
        errno = esave;
        return scope.Close(False());
    }

    /* mmap the output file */
    if ((dst = (char *) mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0)) == (caddr_t) - 1) {
        esave = errno;
        munmap(src, statbuf.st_size);
        close(fdin);
        close(fdout);
        errno = esave;
        return scope.Close(False());
    }

    /* this copies the input file to the output file */
    memcpy(dst, src, statbuf.st_size);
    munmap(src, statbuf.st_size);
    munmap(dst, statbuf.st_size);
    close(fdin);
    close(fdout);

    //    int infd = open(*src, O_RDONLY);
    //    if (infd < 1) {
    //        return scope.Close(False());
    //    }
    //    
    //    int outfd = open(*dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    //    if (outfd == -1) {
    //        close(infd);
    //        return scope.Close(False());
    //    }
    //    const ssize_t SIZE = 128 * 1024;
    //    unsigned char buf[SIZE];
    //    ssize_t count, actual;
    //    while ((count = read(infd, buf, SIZE))) {
    //        if (count < 0) {
    //            close(infd);
    //            close(outfd);
    //            return scope.Close(False());
    //        }
    //        actual = write(outfd, buf, count);
    //        if (actual < 0) {
    //            close(infd);
    //            close(outfd);
    //            return scope.Close(False());
    //        }
    //    }
    //    close(infd);
    //    close(outfd);

    return scope.Close(True());
}
#endif

/**
 * @function fs.md5
 * 
 * ### Synopsis
 * 
 * var hex = fs.md5(path);
 * 
 * This function generates the MD5 hash of the contents of the specified file.
 * 
 * @param {string} path - path of file
 * @return {string} hex - md5 hash
 */
static JSVAL fs_md5 (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path(args[0]->ToString());
    unsigned char buf[1024];
    int bytes;

    MD5_CTX mdContext;
    MD5Init(&mdContext);

    FILE *fp = fopen(*path, "rb");
    if (fp == NULL) {
        return scope.Close(False());
    }
    while ((bytes = fread(buf, 1, 1024, fp)) != 0) {
        MD5Update(&mdContext, buf, bytes);
    }
    fclose(fp);
    MD5Final(&mdContext);
    MD5Digest(&mdContext, (char *) buf);
    return scope.Close(String::New((char *) buf));
}

/**
 * @function fs.cmp
 * 
 * ### Synopsis
 * 
 * var identical = fs.cmp(file1, file2);
 * 
 * This function does a binary comparison of two files.
 * 
 * @param {string} file1 - path to first file to compare.
 * @param {string} file2 - path to second file to compare.
 * @return {boolean{ identical - true if the files are identical, or a string describing the difference if not.
 */
#ifdef WIN32
static JSVAL fs_cmp (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path1(args[0]->ToString());
    String::Utf8Value path2(args[1]->ToString());
    //int fd;
    long size1, size2;
    char buf[1024];
	DWORD iread;

    //fd = open(*path1, O_RDONLY);
	FILE* fd = fopen(*path1, "r");
    //if (fd < 0) {
	if (fd == NULL) {
        sprintf(buf, "Could not open %s", *path1);
        return scope.Close(String::New(buf));
    }
    //size1 = lseek(fd, 0, 2);
	fseek(fd, 0, SEEK_END);
	size1 = ftell(fd);
    //lseek(fd, 0, 0);
	fseek(fd, 0, SEEK_SET);
    //unsigned char buf1[size1];
	unsigned char *buf1 = new unsigned char[size1];
    //if (read(fd, buf1, size1) != size1) {
	if (fread(buf1, sizeof(char), size1, fd) != size1) {
        fclose(fd);
        sprintf(buf, "Error reading %s", *path1);
        return scope.Close(String::New(buf));
    }
    fclose(fd);

    //fd = open(*path2, O_RDONLY);
	fd = fopen(*path2, "r");
    if (fd == NULL) {
        sprintf(buf, "Could not open %s", *path2);
        return scope.Close(String::New(buf));
    }
    //size2 = lseek(fd, 0, 2);
	fseek(fd, 0, SEEK_END);
	size2 = ftell(fd);
    //lseek(fd, 0, 0);
	fseek(fd, 0, SEEK_SET);
    //unsigned char buf2[size2];
	unsigned char *buf2 = new unsigned char[size2];
    //if (read(fd, buf2, size2) != size2) {
	if (fread(buf2, sizeof(char), size2, fd) != size2) {
        fclose(fd);
        sprintf(buf, "Error reading %s", *path2);
        return scope.Close(String::New(buf));
    }
    fclose(fd);

    if (size1 != size2) {
        return scope.Close(String::New("Files differ in size"));
    }

    for (long n = 0; n < size1; n++) {
        if (buf1[n] != buf2[n]) {
            sprintf(buf, "Files differ at offset %ld", n);
            return scope.Close(String::New(buf));
        }
    }
    return scope.Close(True());
}
#else
static JSVAL fs_cmp (JSARGS args) {
    HandleScope scope;
    String::Utf8Value path1(args[0]->ToString());
    String::Utf8Value path2(args[1]->ToString());
    int fd;
    long size1, size2;
    char buf[1024];

    fd = open(*path1, O_RDONLY);
    if (fd < 0) {
        sprintf(buf, "Could not open %s", *path1);
        return scope.Close(String::New(buf));
    }
    size1 = lseek(fd, 0, 2);
    lseek(fd, 0, 0);
    unsigned char buf1[size1];
    if (read(fd, buf1, size1) != size1) {
        close(fd);
        sprintf(buf, "Error reading %s", *path1);
        return scope.Close(String::New(buf));
    }
    close(fd);

    fd = open(*path2, O_RDONLY);
    if (fd < 0) {
        sprintf(buf, "Could not open %s", *path2);
        return scope.Close(String::New(buf));
    }
    size2 = lseek(fd, 0, 2);
    lseek(fd, 0, 0);
    unsigned char buf2[size2];
    if (read(fd, buf2, size2) != size2) {
        close(fd);
        sprintf(buf, "Error reading %s", *path2);
        return scope.Close(String::New(buf));
    }
    close(fd);

    if (size1 != size2) {
        return scope.Close(String::New("Files differ in size"));
    }

    for (long n = 0; n < size1; n++) {
        if (buf1[n] != buf2[n]) {
            sprintf(buf, "Files differ at offset %ld", n);
            return scope.Close(String::New(buf));
        }
    }
    return scope.Close(True());
}
#endif

void init_fs_object () {
    HandleScope scope;

    Handle<ObjectTemplate>fs = ObjectTemplate::New();

    /**
     * @constant fs.O_RDONLY - open for reading only
     * @constant fs.O_WRONLY - open for writing only
     * @constant fs.O_RDWR - open for reading and writing
     * @constant fs.O_CREAT - create file if it does not exist
     * @constant fs.O_TRUNC - truncate file
     */
    fs->Set(String::New("O_RDONLY"), Integer::New(O_RDONLY));
    fs->Set(String::New("O_WRONLY"), Integer::New(O_WRONLY));
    fs->Set(String::New("O_RDWR"), Integer::New(O_RDWR));
    fs->Set(String::New("O_CREAT"), Integer::New(O_CREAT));
    fs->Set(String::New("O_TRUNC"), Integer::New(O_TRUNC));
#ifdef WIN32
	// Look back into being able to lock files.
    fs->Set(String::New("LOCK_EX"), Integer::New(_LK_LOCK));
    fs->Set(String::New("LOCK_UN"), Integer::New(_LK_UNLCK));
    fs->Set(String::New("LOCK_SH"), Integer::New(_LK_LOCK));
    fs->Set(String::New("F_LOCK"), Integer::New(_LK_LOCK));
	fs->Set(String::New("F_ULOCK"), Integer::New(_LK_UNLCK));
    fs->Set(String::New("F_TLOCK"), Integer::New(_LK_NBRLCK));
#else
    // for flock
    /**
     * @constant fs.LOCK_EX - exclusive lock
     * @constant fs.LOCK_SH - shared lock
     * @constant fs.LOCK_UN - unlock
     */
    fs->Set(String::New("LOCK_EX"), Integer::New(LOCK_EX));
    fs->Set(String::New("LOCK_UN"), Integer::New(LOCK_UN));
    fs->Set(String::New("LOCK_SH"), Integer::New(LOCK_SH));
    // for lockf
    /**
     * @constant fs.F_LOCK - block until an exclusive lock can be acquired
     * @constant fs.F_TLOCK - acquire an exclusive lock, return error if failure
     * @constant fs.F_ULOCK - unlock the indicated section of a file
     * @constant fs.F_TEST - test if lock is unlocked or locked by this process
     */
    fs->Set(String::New("F_LOCK"), Integer::New(F_LOCK));
    fs->Set(String::New("F_TLOCK"), Integer::New(F_TLOCK));
    fs->Set(String::New("F_ULOCK"), Integer::New(F_ULOCK));
    fs->Set(String::New("F_TEST"), Integer::New(F_TEST));
#endif
    /**
     * @constant fs.S_IFMT - bit mask for the file type bit fields
     * @constant fs_S_IFSOCK - socket
     * @constant fs.S_IFLNK - symbolic link
     * @constant fs.S_IFREG - regular file
     * @constant fs.S_IFBLK - block device
     * @constant fs.S_IFHCR - character device
     * @constant fs.S_IFIFO - FIFO
     * @constant fs.S_ISUID - set UID bit
     * @constant fs.S_ISGID - set group id bit
     * @constant fs.S_ISVTX - sticky bit
     * @constant fs.S_IRMXU - mask for file owner permissions
     * @constant fs.S_IRUSR - owner has read permission
     * @constant fs.S_IWUSR - owner has write permission
     * @constant fs.S_IXUSR - owner has execute permission
     * @constant fs.S_IRMXG - mask for group permissions
     * @constant fs.S_IRGRP - group has read permission
     * @constant fs.S_IWGRP - group has write permission
     * @constant fs.S_IXGRP - group has execute permission
     * @constant fs.S_IRMXO - mask for others (not in group) permissions
     * @constant fs.S_IROTH - others have read permission
     * @constant fs.S_IWOTH - others have write permission
     * @constant fs.S_IXOTH - others have execute permission
     */
    fs->Set(String::New("S_IFMT"), Integer::New(S_IFMT));
#ifndef WIN32
    fs->Set(String::New("S_IFLNK"), Integer::New(S_IFLNK));
#endif
    fs->Set(String::New("S_IFREG"), Integer::New(S_IFREG));
#ifndef WIN32
    fs->Set(String::New("S_IFBLK"), Integer::New(S_IFBLK));
#endif
    fs->Set(String::New("S_IFDIR"), Integer::New(S_IFDIR));
    fs->Set(String::New("S_IFCHR"), Integer::New(S_IFCHR));
    fs->Set(String::New("S_IFIFO"), Integer::New(S_IFIFO));
#ifndef WIN32
    fs->Set(String::New("S_ISUID"), Integer::New(S_ISUID));
    fs->Set(String::New("S_ISGID"), Integer::New(S_ISGID));
    fs->Set(String::New("S_ISVTX"), Integer::New(S_ISVTX));
#endif
    fs->Set(String::New("S_IRWXU"), Integer::New(S_IRWXU));
    fs->Set(String::New("S_IRUSR"), Integer::New(S_IRUSR));
    fs->Set(String::New("S_IWUSR"), Integer::New(S_IWUSR));
    fs->Set(String::New("S_IXUSR"), Integer::New(S_IXUSR));
    fs->Set(String::New("S_IRWXG"), Integer::New(S_IRWXG));
    fs->Set(String::New("S_IRGRP"), Integer::New(S_IRGRP));
    fs->Set(String::New("S_IWGRP"), Integer::New(S_IWGRP));
    fs->Set(String::New("S_IRWXO"), Integer::New(S_IRWXO));
    fs->Set(String::New("S_IROTH"), Integer::New(S_IROTH));
    fs->Set(String::New("S_IWOTH"), Integer::New(S_IWOTH));
    fs->Set(String::New("S_IXOTH"), Integer::New(S_IXOTH));


    fs->Set(String::New("error"), FunctionTemplate::New(fs_error));
    fs->Set(String::New("chdir"), FunctionTemplate::New(fs_chdir));
    fs->Set(String::New("getcwd"), FunctionTemplate::New(fs_getcwd));
    fs->Set(String::New("open"), FunctionTemplate::New(fs_open));
    fs->Set(String::New("close"), FunctionTemplate::New(fs_close));
    fs->Set(String::New("flock"), FunctionTemplate::New(fs_flock));
    fs->Set(String::New("lockf"), FunctionTemplate::New(fs_lockf));
    fs->Set(String::New("rename"), FunctionTemplate::New(fs_rename));
    fs->Set(String::New("truncate"), FunctionTemplate::New(fs_truncate));
    fs->Set(String::New("chmod"), FunctionTemplate::New(fs_chmod));
    fs->Set(String::New("stat"), FunctionTemplate::New(fs_stat));
    fs->Set(String::New("lstat"), FunctionTemplate::New(fs_lstat));
    fs->Set(String::New("fstat"), FunctionTemplate::New(fs_fstat));
    fs->Set(String::New("link"), FunctionTemplate::New(fs_link));
    fs->Set(String::New("symlink"), FunctionTemplate::New(fs_symlink));
    fs->Set(String::New("readlink"), FunctionTemplate::New(fs_readlink));
    fs->Set(String::New("realpath"), FunctionTemplate::New(fs_realpath));
    fs->Set(String::New("unlink"), FunctionTemplate::New(fs_unlink));
    fs->Set(String::New("rmdir"), FunctionTemplate::New(fs_rmdir));
    fs->Set(String::New("mkdir"), FunctionTemplate::New(fs_mkdir));
    fs->Set(String::New("readDir"), FunctionTemplate::New(fs_readdir));
    fs->Set(String::New("readFile"), FunctionTemplate::New(fs_readfile));
    fs->Set(String::New("readFile64"), FunctionTemplate::New(fs_readfile64));
    fs->Set(String::New("writeFile"), FunctionTemplate::New(fs_writefile));
    fs->Set(String::New("writeFile64"), FunctionTemplate::New(fs_writefile64));
    fs->Set(String::New("copyFile"), FunctionTemplate::New(fs_copyFile));
    fs->Set(String::New("exists"), FunctionTemplate::New(fs_exists));
    fs->Set(String::New("isFile"), FunctionTemplate::New(fs_isfile));
    fs->Set(String::New("isDir"), FunctionTemplate::New(fs_isdir));
    fs->Set(String::New("fileSize"), FunctionTemplate::New(fs_filesize));
    fs->Set(String::New("fileModified"), FunctionTemplate::New(fs_mtime));

    fs->Set(String::New("md5"), FunctionTemplate::New(fs_md5));
    fs->Set(String::New("cmp"), FunctionTemplate::New(fs_cmp));

    builtinObject->Set(String::New("fs"), fs);
}
