/*
 * Author : David Cohen, 
 * Date   : 8 January 2018
 * Copyright (C) 2018 David Cohen
 *
 * Equal Logs code
 * Use Mutex to synchronize multiple threads to write in the same log file during WANTED_RUNTIME seconds
 * 
 */
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
using namespace std;

#define MAX_INSTANCE 20
#define THREADCOUNT 20
#define WANTED_RUNTIME 30
#define LOG_FILE_PATH "log.txt"
#define LOG_MSG "Hello World! I'm thread nb %d which slept during %d milliseconds \r\n"
#define CANT_RELEASE_MUTEX_MSG "Ohhh Grrrr, can't release mutex ?!"
#define WRITE_FAILED "Terminal failure: Unable to write to file.\n"

HANDLE mutex;

/* Function to write in a file */
int writeToFile(LPCTSTR logMessage, LPCTSTR logFilePath) {
    HANDLE fp = CreateFile(logFilePath, FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD writtenBytes = 0;
    char* message = (char*) logMessage;
    BOOL bErrorFlag = WriteFile(fp, logMessage, strlen(message), &writtenBytes, NULL);
    if (FALSE == bErrorFlag)
    {
        printf(WRITE_FAILED);
    }
    CloseHandle(fp);

    return 0;
}

/* Synchronization + Write to file */
BOOL logWrite(LPCTSTR logMessage, LPCTSTR logFilePath) {
    DWORD dwWaitResult = WaitForSingleObject(mutex, INFINITE);

    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0:
            __try {
                writeToFile(logMessage, logFilePath);
            }

            __finally {
                if (!ReleaseMutex(mutex))
                {
                    printf(CANT_RELEASE_MUTEX_MSG);
                }
            }
            break;

        case WAIT_ABANDONED:
            return FALSE;
    }
}

/* Thread function */
DWORD WriteToLogFile(LPVOID lpParam) {
    int threadNb = (int) lpParam;
    char buffer[1024];
    while (1) 
    {
        srand((unsigned)time(NULL));
        int rand_sleep = (rand() % 1800) + 200;

        sprintf(buffer, LOG_MSG, threadNb, rand_sleep);
        logWrite(buffer,
            LOG_FILE_PATH);
        Sleep(rand_sleep);
    }
    return 0;
}

/* Main that create the mutex and the thread */
int main(void) {
    HANDLE aThread[THREADCOUNT];
    DWORD ThreadID;
    int i;
    srand(time(NULL));

    // Create mutex
    mutex = CreateMutex(NULL, FALSE, NULL);

    if (mutex == NULL) {
        printf("CreateMutex error : %d\n", GetLastError());
        return 1;
    }

    // Create threads
    for (i = 0; i < THREADCOUNT; i++)
    {
        aThread[i] = CreateThread(
            NULL,       // default security attributes
            0,          // default stack size
            (LPTHREAD_START_ROUTINE)WriteToLogFile,
            (LPVOID)i,  // thread number as a parameter
            0,          // default creation flags
            &ThreadID); // receive thread identifier

        if (aThread[i] == NULL)
        {
            printf("CreateThread error: %d\n", GetLastError());
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
            runtime_passed = TRUE;
    }
}
