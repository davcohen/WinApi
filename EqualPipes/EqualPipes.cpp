/*
* Author : David Cohen,
* Date   : 8 January 2018
* Copyright (C) 2018 David Cohen
*
* Equal Pipes code
* Use Pipes to synchronize multiple threads to write in the same log file during WANTED_RUNTIME seconds
*/

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
using namespace std;

#define THREAD_COUNT 10 /* number of thread to instanciate */
#define THREAD_SLEEP_TIME_START 5 /* Time between each createThread */
#define WANTED_RUNTIME 30 /* total runtime */

#define LOG_FILE_PATH "log.txt" /* path of the log file */
#define LOG_MSG "Hello World! I'm thread nb %d which slept during %d milliseconds \r\n" /* Message to write in log file */
#define CANT_RELEASE_MUTEX_MSG "Ohhh Grrrr, can't release mutex ?!" /* Messge in case of failure */
#define WRITE_FAILED "Terminal failure: Unable to write to file.\n" /* Write failure */

#define PIPE_PATH "\\\\.\\pipe\\log_path" /* pipe path */
#define PIPE_MAX_INSTANCE 20 /* max instance of the pipes */
#define PIPE_CREATION_FAILURE "Could not create the pipe\n"

#define INSTANCE_NOT_FOUND_ERROR "WaitNamedPipe failed. Instance not found error. error=%d\n"
#define INSTANCE_OK "ThreadNB %d uses InstanceNB %d of Pipe\n"

#define CREATE_FILE_NOT_FOUND "CreateFile failed with error %d\n"
#define CREATE_MUTEX_ERROR "CreateMutex error : %d\n"
#define CREATE_THREAD_ERROR "CreateThread error: %d\n"

HANDLE mutex; /* mutex to synchronize thread creation and server creation - first Server and after clients can be created */

/* Function to write in a file */
int writeToFile(LPCTSTR logMessage, LPCTSTR logFilePath, DWORD bytesToWrite) {
    HANDLE fp = CreateFile(logFilePath, FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD writtenBytes = 0;
    char* message = (char*)logMessage;
    BOOL bErrorFlag = WriteFile(fp, logMessage, bytesToWrite, &writtenBytes, NULL);
    if (FALSE == bErrorFlag)
    {
        printf(WRITE_FAILED);
    }
    CloseHandle(fp);

    return 0;
}

/* Server side */
DWORD serverLogFile(LPVOID lpParam) {
    // Create pipe that will receive all the log message to write in the file
    HANDLE serverPipe[PIPE_MAX_INSTANCE];
    DWORD dwBytesRead;
    char buf[1024];
    for (int i = 0; i < PIPE_MAX_INSTANCE; i++)
    {
        serverPipe[i] = CreateNamedPipe(PIPE_PATH, // Name
            PIPE_ACCESS_DUPLEX,                 // OpenMode
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // PipeMode
            PIPE_MAX_INSTANCE, // MaxInstances
            1024, // OutBufferSize
            1024, // InBuffersize
            2000, // TimeOut
            NULL); // Security

        if (serverPipe[i] == INVALID_HANDLE_VALUE)
        {
            printf(PIPE_CREATION_FAILURE);
            exit(1);
        }
    }

    // Release Mutex to advance process
    ReleaseMutex(mutex);

    // Wait for connection
    ConnectNamedPipe(serverPipe[0], NULL);

    // Read until EOF
    for (;;)
    {
        for (int i = 0; i < PIPE_MAX_INSTANCE; i++) {
            if (ReadFile(serverPipe[i], buf, sizeof(buf), &dwBytesRead, NULL))
            {
                writeToFile(buf, LOG_FILE_PATH, dwBytesRead);
            }
        }
    }

    // Close handle
    for (int i = 0; i < PIPE_MAX_INSTANCE; i++)
        CloseHandle(serverPipe[i]);

    return 0;
}

/* Thread function */
DWORD threadLogGenerator(LPVOID lpParam) {
    DWORD       threadNb = (DWORD)lpParam;
    HANDLE		clientSidePipe;
    char		buf[1024];
    DWORD		len;
    DWORD		dwWritten;
    // Wait for free pipe instance
    DWORD       instanceNb = WaitNamedPipe(PIPE_PATH, NMPWAIT_WAIT_FOREVER);

    if (instanceNb == 0)
    {
        printf(INSTANCE_NOT_FOUND_ERROR, GetLastError());
        return -1;
    }
    else 
    {
        printf(INSTANCE_OK, threadNb, instanceNb);
    }
    
    /* Connect to an instance of the pipe */
    clientSidePipe = CreateFile(PIPE_PATH,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (clientSidePipe == INVALID_HANDLE_VALUE)
    {
        printf(CREATE_FILE_NOT_FOUND, GetLastError());
        return -1;
    }

    while (1)
    {
        srand(GetTickCount());
        int rand_sleep = (rand() % 1800) + 200;

        sprintf(buf, LOG_MSG, threadNb, rand_sleep);
        len = lstrlen(buf);
        if (!WriteFile(clientSidePipe, buf, len, &dwWritten, NULL))
        {
            printf(WRITE_FAILED);
        }

        Sleep(rand_sleep);
    }

    CloseHandle(clientSidePipe);

    return 0;
}

/* Main that create the mutex and the thread */
int main(void) {
    HANDLE aThread[THREAD_COUNT];
    DWORD ThreadID;
    DWORD i;

    // Create mutex
    mutex = CreateMutex(NULL, TRUE, NULL);

    if (mutex == NULL) {
        printf(CREATE_MUTEX_ERROR, GetLastError());
        return 1;
    }

    // Create threads : server + client
    for (i = 0; i < THREAD_COUNT; i++)
    {
        if (i == 0) 
        {
            aThread[i] = CreateThread(
                NULL,       // default security attributes
                0,          // default stack size
                (LPTHREAD_START_ROUTINE)serverLogFile,
                (LPVOID)i,  // thread number as a parameter
                0,          // default creation flags
                &ThreadID); // receive thread identifier

            WaitForSingleObject(mutex, INFINITE);
        }
        else 
        {
            aThread[i] = CreateThread(
                NULL,       // default security attributes
                0,          // default stack size
                (LPTHREAD_START_ROUTINE)threadLogGenerator,
                (LPVOID)i,  // thread number as a parameter
                0,          // default creation flags
                &ThreadID); // receive thread identifier

            Sleep(THREAD_SLEEP_TIME_START); // sleep to get different random afterward
        }
        
        if (aThread[i] == NULL)
        {
            printf(CREATE_THREAD_ERROR, GetLastError());
            return 1;
        }
    }

    /* Manage run time */
    double runtime = WANTED_RUNTIME;
    clock_t begin = clock();
    BOOL runtime_passed = FALSE;
    while (!runtime_passed) {
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        if (time_spent > runtime)
        {
            runtime_passed = TRUE;

            // Close thread and mutex handles
            for (i = 0; i < THREAD_COUNT; i++)
                CloseHandle(aThread[i]);

            CloseHandle(mutex);
        }

    }
}