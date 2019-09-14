//
// Created by Abigail on 9/14/2019.
//

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <queue>
#include "thread.h"
#include <algorithm>
#include <signal.h>
#include <sys/time.h>
#include "sleeping_threads_list.h"
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define FAIL -1
#define SUCCESS 0
#define MAIN_THREAD 0
#define EMPTY nullptr
/*
* User-Level Threads Library (uthreads)
* Author: OS, os@cs.huji.ac.il
*/

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

void free_memory();
void switch_threads();
void time_handler(int sig);
void handler(int sig);
/* External interface */
int total_quantoms;
Thread* running_thread;
int quantom_secs;
std::map<int, Thread*> threads;
std::deque<int> ready_threads;
std::set<int> blocked_threads;
std::priority_queue<int, std::vector<int>, std::greater<int>> available_indices;
sigset_t sigsetBlock;
struct sigaction sa = {0};
struct sigaction sa_time = {0};
struct itimerval timer;
struct itimerval sleep_timer{};
SleepingThreadsList* sleeping_threads = new SleepingThreadsList();
std::set<int> sleeping_ids;


timeval calc_wake_up_timeval(int usecs_to_sleep) {

  timeval now, time_to_sleep, wake_up_timeval;
  gettimeofday(&now, nullptr);
  time_to_sleep.tv_sec = usecs_to_sleep / 1000000;
  time_to_sleep.tv_usec = usecs_to_sleep % 1000000;
  timeradd(&now, &time_to_sleep, &wake_up_timeval);
  return wake_up_timeval;
  }



void block_signal(){
    if (sigprocmask(SIG_BLOCK, &sigsetBlock, nullptr) < 0)
    {
        std::cerr << "system error: block error."<< std::endl;
        free_memory();
        exit(1);
    }
}

void unblock_signal() {
    if (sigprocmask(SIG_UNBLOCK, &sigsetBlock, nullptr) < 0)
    {
        std::cerr << "system error: unblock error."<< std::endl;
        free_memory();
        exit(1);
    }
}

void switch_threads() {
    block_signal();
    //make the top of ready run
    if (sigsetjmp(running_thread->get_env(), 1) == 1)
    {
        unblock_signal();
        return;
    }

    running_thread = threads[ready_threads.front()];
    ready_threads.pop_front();
    running_thread->set_state(RUNNING);
    running_thread->increase_quantom();
    total_quantoms++;

    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr) < 0) //reset timer.
    {
        std::cerr << "system error: virtual timer error."<< std::endl;
        free_memory();
        exit(1);

    }
    unblock_signal();
    siglongjmp(running_thread->get_env(), 1);
}

/*
* Description: This function initializes the thread library.setiti
* You may assume that this function is called before any other thread library
* function, and that it is called exactly once. The input to the function is
* the length of a quantum in micro-seconds. It is an error to call this
* function with non-positive quantum_usecs.
* Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    if (quantum_usecs <= 0) {
       std::cerr << "thread library error: invalid quantom length"<< std::endl;
       return FAIL;
       }
    if (sigaddset(&sigsetBlock, SIGVTALRM) < 0)  {
        std::cerr << "system error: sigset error."<< std::endl;
        exit(1);
       }
    // init main thread
    auto main_thread = new Thread(0, nullptr);
    main_thread->set_state(RUNNING);
    main_thread->increase_quantom();
    threads[0] = main_thread;
    total_quantoms++;
    running_thread = main_thread;
    quantom_secs = quantum_usecs;

    sa.sa_handler = &handler;
    if (sigaction(SIGVTALRM, &sa, nullptr) < 0) {
        std::cerr << "system error: sigaction error."<< std::endl;
        exit(1);
    }

    if (sigaddset(&sigsetBlock, SIGALRM) < 0) {
        std::cerr << "system error: sigset error."<< std::endl;
        exit(1);
    }
    sa_time.sa_handler = &time_handler;
    if (sigaction(SIGALRM, &sa_time, nullptr) < 0) {
        std::cerr << "system error: sigaction error."<< std::endl;
        exit(1);
        }

    timer.it_value.tv_sec = quantum_usecs / 1000000; // first time interval, seconds part
    timer.it_value.tv_usec = quantum_usecs % 1000000; // first time interval, microseconds part

    timer.it_interval.tv_sec = quantum_usecs / 1000000; // following time intervals, seconds part
    timer.it_interval.tv_usec = quantum_usecs % 1000000; // following time intervals, microseconds part

    if (setitimer (ITIMER_VIRTUAL, &timer, nullptr) < 0){
        std::cerr << "system error: virtual timer error."<< std::endl;
        exit(1);
        }
    for (int i = 1; i <= MAX_THREAD_NUM-1; i++)
    {
        available_indices.push(i);
    }
    return SUCCESS;
}

/*
* Description: This function creates a new thread, whose entry point is the
* function f with the signature void f(void). The thread is added to the end
* of the READY threads list. The uthread_spawn function should fail if it
* would cause the number of concurrent threads to exceed the limit
* (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
* STACK_SIZE bytes.
* Return value: On success, return the ID of the created thread.
* On failure, return -1.
*/
int uthread_spawn(void (*f)(void)) {
    block_signal();
    if (available_indices.empty()) {
        std::cerr << "thread library error: exceeded maximum thread number." << std::endl;
        unblock_signal ();
        return FAIL;
    }
    int tid = available_indices.top();
    available_indices.pop();
    auto new_thread = new Thread(tid, f);
    threads[tid] = new_thread;
    ready_threads.push_back(tid);
    unblock_signal();
    return tid;
}


/*
* Description: This function terminates the thread with ID tid and deletes
* it from all relevant control structures. All the resources allocated by
* the library for this thread should be released. If no thread with ID tid
* exists it is considered an error. Terminating the main thread
* (tid == 0) will result in the termination of the entire process using
* exit(0) [after releasing the assigned library memory].
* Return value: The function returns 0 if the thread was successfully
* terminated and -1 otherwise. If a thread terminates itself or the main
* thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    block_signal();
    if (threads.find(tid) == threads.end()){
        std::cerr << "thread library error: terminating non-existing id."<< std::endl;
        unblock_signal();
        return FAIL;
    }
    if (tid == MAIN_THREAD){
        free_memory();
        unblock_signal();
        exit(0);
    }
    available_indices.push(tid);
    if (threads[tid]->get_state() == BLOCKED) {
        delete threads[tid];
        threads.erase(tid);
        if (sleeping_ids.find(tid) == sleeping_ids.end())
        {
            blocked_threads.erase(tid);
        }
    }
    else if (threads[tid]->get_state() == READY){
         delete threads[tid];
         threads.erase(tid);
         ready_threads.erase(std::remove(ready_threads.begin(), ready_threads.end(), tid), ready_threads.end());
    }
    else if (threads[tid]->get_state() == RUNNING) {
        delete threads[tid];
        threads.erase(tid);
        switch_threads();
    }
    unblock_signal();
    return SUCCESS;
}


 /*
* Description: This function blocks the thread with ID tid. The thread may
* be resumed later using uthread_resume. If no thread with ID tid exists it
* is considered as an error. In addition, it is an error to try blocking the
* main thread (tid == 0). If a thread blocks itself, a scheduling decision
4 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
 */
 int uthread_block(int tid) {
     block_signal();
     if (threads.find(tid) == threads.end())
     {
         std::cerr << "thread library error: blocking non-existing id."<< std::endl;
         unblock_signal();
         return FAIL;
     }
     if (tid == MAIN_THREAD)
     {
         std::cerr << "thread library error: trying to block main thread."<< std::endl;
         unblock_signal();
         return FAIL;
     }
     blocked_threads.insert(tid);
     threads[tid]->set_state(BLOCKED);
     if (running_thread->get_id() == tid)
     {
         switch_threads();
     }
     else if (std::find(ready_threads.begin(), ready_threads.end(), tid) != ready_threads.end())
     {
         ready_threads.erase(std::remove(ready_threads.begin(), ready_threads.end(), tid), ready_threads.end());
     }
     unblock_signal();
     return SUCCESS;
 }


 /*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
 */
 int uthread_resume(int tid)
 {
     block_signal();
     if (threads.find(tid) == threads.end())
     {
         std::cerr << "thread library error: resuming non-existing id."<< std::endl;
         unblock_signal();
         return FAIL;
     }

     if (blocked_threads.find(tid) != blocked_threads.end())
     {
         blocked_threads.erase(blocked_threads.find(tid));
         ready_threads.push_back(tid);
         if (sleeping_ids.find(tid) == sleeping_ids.end()) //we don't want a sleeping thread to move to ready before time
         {
             threads[tid]->set_state(READY);
         }
     }

     unblock_signal();
     return SUCCESS;
     }

/*
* Description: This function blocks the RUNNING thread for user specified micro-seconds.
* It is considered an error if the main thread (tid==0) calls this function.
* Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision
* should be made.
* Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec)
{
    block_signal();
    if (running_thread->get_id() == MAIN_THREAD)
    {
        std::cerr << "thread library error: invalid sleep call. main thread can't sleep."<< std::endl;
        unblock_signal();
        return FAIL;
    }
    timeval wake_up_time = calc_wake_up_timeval(usec);
    if (sleeping_threads->peek() == EMPTY)
    {
        sleep_timer.it_value.tv_sec = usec / 1000000;
        sleep_timer.it_value.tv_usec = usec % 1000000;
        if (setitimer (ITIMER_REAL, &sleep_timer, nullptr) < 0)
        {
            std::cerr << "system error: real timer error."<< std::endl;
            exit(1);
        }
    }
    else
    {
        timeval top = sleeping_threads->peek()->awaken_tv;
        if (timercmp(&wake_up_time, &top, <))
        {
            sleep_timer.it_value.tv_sec = usec / 1000000;
            sleep_timer.it_value.tv_usec = usec % 1000000;
            if (setitimer (ITIMER_REAL, &sleep_timer, nullptr) < 0)
            {
                std::cerr << "system error: real timer error."<< std::endl;
                exit(1);
            }
        }
    }
    sleeping_threads->add(running_thread->get_id(), wake_up_time);
    sleeping_ids.insert(running_thread->get_id());
    threads[running_thread->get_id()]->set_state(BLOCKED);
    switch_threads();
    unblock_signal();
    return SUCCESS;
}
/*
* Description: This function returns the thread ID of the calling thread.
* Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
    return running_thread->get_id();
}
/*
* Description: This function returns the total number of quantums since
* the library was initialized, including the current quantum.
* Right after the call to uthread_init, the value should be 1.
* Each time a new quantum starts, regardless of the reason, this number
* should be increased by 1.
* Return value: The total number of quantums.
*/
 uthread_get_total_quantums()
{
    return total_quantoms;
}

/*
* Description: This function returns the number of quantums the thread with
* ID tid was in RUNNING state. On the first time a thread runs, the function
* should return 1. Every additional quantum that the thread starts should
* increase this value by 1 (so if the thread with ID tid is in RUNNING state
* when this function is called, include also the current quantum). If no
* thread with ID tid exists it is considered an error.
* Return value: On success, return the number of quantums of the thread with ID tid.
* On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    if (threads.find(tid) == threads.end())
    {
        std::cerr << "thread library error: invalid id."<< std::endl;
        return FAIL;
    }
    return threads[tid]->get_quantom();
}

void free_memory()
{
    for (auto& thread: threads)
    {
        delete thread.second;
    }
    threads.clear();
    blocked_threads.clear();
    ready_threads.clear();
    while (!available_indices.empty())
    {
        available_indices.pop();
    }
    delete running_thread;
    while (sleeping_threads->peek() != EMPTY)
    {
        sleeping_threads->pop();
    }
    delete sleeping_threads;
    sleeping_ids.clear();
}

void handler(int sig)
{
    block_signal();
    ready_threads.push_back(running_thread->get_id());
    running_thread->set_state(READY);
    switch_threads();
    unblock_signal();
}

void time_handler(int sig)
{
    block_signal();
    int tid = sleeping_threads->peek()->id;
    sleeping_threads->pop();
    if (blocked_threads.find(tid) == blocked_threads.end() && threads.find(tid) != threads.end())
    {
        ready_threads.push_back(tid);
        threads[tid]->set_state(READY);
    }
    sleeping_ids.erase(tid);
    if (sleeping_threads->peek() != EMPTY)
    {
        timeval now, next_wakeup;
        gettimeofday(&now, nullptr);
        timersub(&sleeping_threads->peek()->awaken_tv, &now, &next_wakeup);
        sleep_timer.it_value.tv_sec = next_wakeup.tv_sec;
        sleep_timer.it_value.tv_usec = next_wakeup.tv_usec;
        if (setitimer (ITIMER_REAL, &sleep_timer, nullptr) < 0)
        {
            std::cerr << "system error: real timer error."<< std::endl;
            exit(1);
        }
    }
    unblock_signal();
}

