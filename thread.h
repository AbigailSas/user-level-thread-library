//
// Created by Abigail on 9/14/2019.
//

#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <csetjmp>
#include <csignal>
class Thread
{
    public:
        Thread(int tid, void (*f)(void));
        int get_id();
        int get_state();
        void set_state(int new_state);
        sigjmp_buf& get_env();
        int get_quantom();
        void increase_quantom();
        ~Thread();
    private:
        int id;
        int state;
        void (*f)(void);
        char* stack;
        sigjmp_buf env;
        int quantoms;
};
#endif //EX2_THREAD_H