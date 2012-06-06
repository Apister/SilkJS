/** @ignore */
#include "SilkJS.h"
#ifndef WIN32
#include <semaphore.h>
#endif

#ifdef WIN32
HANDLE hMutex;
#else
static sem_t mutex;
#endif

static JSVAL sem_Init (JSARGS args) {
    HandleScope scope;
    int ret;

#ifdef WIN32
    if ((hMutex = CreateSemaphoreA(NULL, 1, 1, NULL)) == NULL) {
#else
    if ((ret = sem_init(&mutex, 1, 1)) < 0) {
#endif
        perror("sem_init");
        return False();
    }
    return True();
}

static JSVAL sem_Destroy (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	bool bret = CloseHandle(hMutex);
    return scope.Close(Integer::New((bret)?0:-1));
#else
    return scope.Close(Integer::New(sem_destroy(&mutex)));
#endif
}

static JSVAL sem_Wait (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	DWORD ret = WaitForSingleObject(hMutex, INFINITE);
#else
    int ret = sem_wait(&mutex);
#endif
    printf("wait %d\n", ret);
    return scope.Close(Integer::New(ret));
}

static JSVAL sem_Post (JSARGS args) {
    HandleScope scope;
#ifdef WIN32
	bool bret = ReleaseSemaphore(hMutex, 1, NULL);
	return scope.Close(Integer::New((bret)?0:-1));
#else
    return scope.Close(Integer::New(sem_post(&mutex)));
#endif
}

void init_sem_object () {
    HandleScope scope;

    Handle<ObjectTemplate>sem = ObjectTemplate::New();
    sem->Set(String::New("init"), FunctionTemplate::New(sem_Init));
    sem->Set(String::New("destroy"), FunctionTemplate::New(sem_Destroy));
    sem->Set(String::New("wait"), FunctionTemplate::New(sem_Wait));
    sem->Set(String::New("post"), FunctionTemplate::New(sem_Post));

    builtinObject->Set(String::New("sem"), sem);
}
