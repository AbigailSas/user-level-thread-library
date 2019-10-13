# user-level-thread-library
OS university course exercise in which I created a static user level thread library

In this program I created a functional static library, that creates and manages user level threads.
A potential user can include this library and use it according to the package’s public interface: the uthreads.h header file.
The schedualing algorithm implemented for choosing the threads is a Round Robin schedualing algorithm. 

Assumptions: 
- Initially, a program is comprised of the default main thread, whose ID is 0. All other threads will be explicitly
created. 
- Each thread can be in one of the following states: RUNNING, BLOCKED, or READY. 
- All threads end with uthread_terminate before returning, either by terminating themselves or due
to a call by some other thread.
- The stack space of each spawned thread isn't exceeded during its execution.
- The main thread and the threads spawned using the uthreads library will not send timer signals
themselves (specifically SIGVTALRM), mask them or set interval timers that do so.

Any code\file that was not writen by me is specified as writen by OS course staff with comments on the code itself. 
