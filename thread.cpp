//
// Created by Abigail on 9/14/2019.
//


#include "thread.h"
#include <unistd.h>
#include <sys/time.h>
#define STACK_SIZE 4096
#define READY 0
#define RUNNING 1
#define BLOCKED 2

char stack[STACK_SIZE];


/* code of translate_address writen by course staff */ 

 #ifdef __x86_64__
 /* code for 64 bit Intel arch */

 typedef unsigned long address_t;
 #define JB_SP 6
 #define JB_PC 7

 /* A translation is required when using an address of a variable.
 Use this as a black box in your code. */
 address_t translate_address(address_t addr)
 {
     address_t ret;
     asm volatile("xor %%fs:0x30,%0\n"
     "rol $0x11,%0\n"
     : "=g" (ret)
     : "0" (addr));
     return ret;
 }

 #else
 /* code for 32 bit Intel arch */

 typedef unsigned int address_t;
 #define JB_SP 4
 #define JB_PC 5

 /* A translation is required when using an address of a variable.
 Use this as a black box in your code. */
 address_t translate_address(address_t addr)
 {
     address_t ret;
     asm volatile("xor %%gs:0x18,%0\n"
     "rol $0x9,%0\n"
     : "=g" (ret)
     : "0" (addr));
     return ret;
 }

 #endif

/* my code starts here */

 Thread::Thread(int tid, void (*f)(void)):id(tid), state(READY)
 {
     if (tid == 0) {
         stack = new char[STACK_SIZE];
         sigsetjmp(env, 1);
         sigemptyset(&env->__saved_mask);
         }
      else {
         stack = new char[STACK_SIZE];
         address_t sp, pc;
         sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
         pc = (address_t) f;
         sigsetjmp(env, 1);
         (env->__jmpbuf)[JB_SP] = translate_address(sp);
         (env->__jmpbuf)[JB_PC] = translate_address(pc);
         sigemptyset(&env->__saved_mask);
         }
 }

int Thread::get_id() {return id;}

int Thread::get_state(){return state;}

void Thread::set_state(int new_state){state = new_state;}

Thread::~Thread(){
    if (id == 0) {
        return;
    }
    delete stack;
}

 sigjmp_buf& Thread::get_env() { return env;}

 int Thread::get_quantom() { return quantoms; }

 void Thread::increase_quantom() { quantoms++; }
