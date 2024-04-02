#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
    int quantum;
} SchedSJFArgs;

void schedSJF(FakeOS* os, void *args_, int cpu_index) {
    SchedSJFArgs* args = (SchedSJFArgs*) args_;
    FakePCB* shortest_job = findShortestJob(&os->ready);
    if (shortest_job->actual_burst == 0) return;
   
    FakePCB* running = os->cpus[cpu_index].running;
    if (running) {
        ProcessEvent* current_burst = (ProcessEvent*) running->events.first;
        //printf("\t[debug] current_burst->duration = %d\n", current_burst->duration);
        // compare shortest_job->actual_burst to the duration of the currently running event 
        if (shortest_job->actual_burst < current_burst->duration) {    
           /* perform preemption APPROACH 1
            * 1. Create a new CPU event with duration equals to current_burst->duration
            * 2. Set current_burst->duration=0 so that the preempted process 
            *    will be pushed back into the ready list
            * 3. Prepend the new CPU event to the events list of the preempted process.
            *    This step has to be performed after the popping of the old event from the list,
            *    otherwise the popped event will be the new one. */
            /*ProcessEvent* tmp = (ProcessEvent*) malloc(sizeof(ProcessEvent));
            tmp->list.prev = tmp->list.next = 0;
            tmp->type = CPU;
            tmp->duration = current_burst->duration;
            printf("\t[debug]tmp->duration = %d\n", tmp->duration);
            current_burst->duration = 0;
            printf("\t[debug]current_burst->duration after preemption = %d of [pid %d]\n",
                    current_burst->duration, running->pid);
            // THIS HAS TO BE PERFORMED AFTER THE OLD PROCESS HAS BEEN POPPED
            List_pushFront(&running->events, (ListItem*) tmp);*/
            
            /* perform preemption APPROACH 2 */
            running->predicted_burst = current_burst->duration;
            List_pushBack(&os->ready, (ListItem*) running);
            printf("\t\t\t[CPU %d] assigned to [pid %d], [pid %d] moved to ready \n",
                    cpu_index, shortest_job->pid, running->pid);
        } 
    }

    os->cpus[cpu_index].running = shortest_job;
    List_detach(&os->ready, (ListItem*) shortest_job);
    ProcessEvent* e = (ProcessEvent*)shortest_job->events.first;
    /* if e->duration > quantum, then prepend to the events list of this process 
     * a CPU event of duration quantum,
     * then alter the duration of the old event subtracting quantum */
    if (e->duration > args->quantum) {
        ProcessEvent* qe = (ProcessEvent*) malloc(sizeof(ProcessEvent));
        qe->list.prev = qe->list.next = 0;
        qe->type = CPU;
        qe->duration = args->quantum;
        e->duration -= args->quantum;
        List_pushFront(&shortest_job->events, (ListItem*) qe);
    }
}

int main(int argc, char **argv) {
    FakeOS_init(&os);
    SchedSJFArgs sjf_args;
    sjf_args.quantum = 5;
    os.schedule_args = &sjf_args;
    os.schedule_fn = schedSJF;

    for (int i=1; i < argc; ++i) {
        FakeProcess new_process;
        int num_events = FakeProcess_load(&new_process, argv[i]);
        printf("loading [%s], pid: %d, num_events: %d\n", argv[i], new_process.pid, num_events);
        if (num_events) {
            FakeProcess* new_process_ptr = (FakeProcess*) malloc(sizeof(FakeProcess));
            *new_process_ptr = new_process;
            List_pushBack(&os.processes, (ListItem*) new_process_ptr);
        }
    }
    printf("num processes in queue %d\n", os.processes.size);
    while(!is_any_cpu_free(&os) || os.ready.first || os.waiting.first || os.processes.first)
        FakeOS_simStep(&os);
}
