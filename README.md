# WinApi

This repository contains some interesting and basic usage of Win32 api.

You'll learn how to use : 

1.  Use **Mutex** : in EqualLogs repository, you will find a good example of synchronization of thread using Mutex. The goal is to write in a same log file. So, you'll see how to handle mutex in order to prevent threads from writing at the same time in this logfile.
2.  Use **NamedPipe** : in EqualPipes repository, you will find a good example of synchronization of thread to write in a log file using Pipes !!! so, no more mutex needed. 