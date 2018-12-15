/*
 * student.c
 * Multithreaded OS Simulation for ECE 3056
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"
#include <string.h>
/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


static void addToQueue(pcb_t *ptr);
static pcb_t* removeFromQueue(void);

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

//
static pcb_t *globalHead;
static pthread_mutex_t readyLock;
static pthread_cond_t readyCond;
unsigned int cpuCount;
int timeSlice = -1;

typedef enum {
    FIFO = 0,
    RoundRobin = 1,
    LRTF = 2
} schedulerType;

static schedulerType scheduler;
//


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&readyLock);
    pcb_t *removedProcess = removeFromQueue();
    pthread_mutex_unlock(&readyLock);
    if (removedProcess == NULL) {
        context_switch(cpu_id, NULL, -1);
    } else if (removedProcess != NULL) {
        pthread_mutex_lock(&readyLock);
        removedProcess->state = PROCESS_RUNNING;
        pthread_mutex_unlock(&readyLock);
    }
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = removedProcess;
    pthread_mutex_unlock(&current_mutex);
    if (scheduler == FIFO) {
        context_switch(cpu_id, removedProcess, -1);
    } else if (scheduler == RoundRobin) {
        context_switch(cpu_id, removedProcess, timeSlice);
    } else if (scheduler == LRTF) {
        context_switch(cpu_id, removedProcess, -1);
    }
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&readyLock);
    while (globalHead == NULL) {
        pthread_cond_wait(&readyCond, &readyLock);
    }
    pthread_mutex_unlock(&readyLock);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    //mt_safe_usleep(1000000);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t *currProcess;
    currProcess = current[cpu_id];
    current[cpu_id]->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);
    pthread_mutex_lock(&readyLock);
    addToQueue(currProcess);
    pthread_mutex_unlock(&readyLock);
    schedule(cpu_id);    
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t *currProcess;
    currProcess = current[cpu_id];
    currProcess->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t *currProcess;
    currProcess = current[cpu_id];
    currProcess->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is LRTF, wake_up() may need
 *      to preempt the CPU with lower remaining time left to allow it to
 *      execute the process which just woke up with higher reimaing time.
 * 	However, if any CPU is currently running idle,
* 	or all of the CPUs are running processes
 *      with a higher remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
    process->state = PROCESS_READY;
    pthread_mutex_lock(&readyLock);
    addToQueue(process);
    pthread_mutex_unlock(&readyLock);
    if (scheduler == LRTF) {
        pcb_t *highest;
        highest = process;
        unsigned int ind;
        ind = 0;
        pthread_mutex_lock(&current_mutex);
        for (unsigned int i = 0; i < cpuCount; i++) {
            if (current[i] == NULL) {
                highest = current[i];
                break; 
            } else if(current[i]->time_remaining < highest->time_remaining) { //////////////////
                highest = current[i];
                ind = i;
            }
        }
        pthread_mutex_unlock(&current_mutex);
        if (highest != process && highest != NULL) {
            force_preempt(ind);
        }
    }    
}



static void addToQueue(pcb_t *process) {
    if (globalHead == NULL) {
        globalHead = process;
        globalHead->next = NULL;
        pthread_cond_signal(&readyCond);
    } else {
        pcb_t *temp = globalHead;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = process;
        process->next = NULL;
    }
}

static pcb_t* removeFromQueue(void) {
    pcb_t *removedProcess = NULL;
    if (scheduler == LRTF) {
        if (globalHead == NULL){
            return removedProcess;
        }
        removedProcess = globalHead;
        pcb_t *temp;
        temp = removedProcess->next;
        while (temp != NULL) {
            if (temp->time_remaining > removedProcess->time_remaining){  ////////////////
                removedProcess = temp;
            } 
            temp = temp->next;
        }
        if (removedProcess == globalHead) {
            globalHead = globalHead->next;
        } else {
            pcb_t *restart;
            restart = globalHead;
            while (restart->next != removedProcess) {
                restart = restart->next;
            }
            restart->next = removedProcess->next;
        }
        removedProcess->next = NULL;
        return removedProcess;
    }
    if (globalHead == NULL) {
        return removedProcess;
    } else {
        removedProcess = globalHead;
        globalHead = globalHead->next;
    }
    removedProcess->next = NULL;
    return removedProcess;
}




/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -l and -r command-line parameters.
 */
int main(int argc, char *argv[])
{
    //unsigned int cpu_count;

    /* Parse command-line arguments */
    // if (argc != 2)
    // {
    //     fprintf(stderr, "ECE 3056 OS Sim -- Multithreaded OS Simulator\n"
    //         "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
    //         "    Default : FIFO Scheduler\n"
	//     "         -l : Longest Remaining Time First Scheduler\n"
    //         "         -r : Round-Robin Scheduler\n\n");
    //     return -1;
    // }
    //printf("**********************************you have %d argc********************************\n",argc);
    if (argc != 1) {
        cpuCount = strtoul(argv[1], NULL, 0);
    }
    
    
    /* FIX ME - Add support for -l and -r parameters*/
    if (argc == 2) {
        scheduler = FIFO;
    }else if (argc == 4 && strcmp(argv[2], "-r") == 0) {
        scheduler = RoundRobin;
        timeSlice = atoi(argv[3]);
    } else if (argc == 3 && strcmp(argv[2], "-l") == 0) {
        scheduler = LRTF;
    } else if (argc == 1){
        fprintf(stderr, "ECE 3056 OS Sim -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
            "    Default : FIFO Scheduler\n"
	    "         -l : Longest Remaining Time First Scheduler\n"
            "         -r : Round-Robin Scheduler\n\n");
        return -1;
    }
    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpuCount);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&readyLock, NULL);

    /* Start the simulator in the library */
    start_simulator(cpuCount);  /////////

    return 0;
}


