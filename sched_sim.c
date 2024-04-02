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

    // handle the event when it exceeds the quantum
    ProcessEvent* e = (ProcessEvent*)shortest_job->events.first;
    if (e->duration > args->quantum) {
        ProcessEvent* qe = (ProcessEvent*) malloc(sizeof(ProcessEvent));
        qe->list.prev = qe->list.next = 0;
        qe->type = CPU;
        qe->duration = args->quantum;
        e->duration -= args->quantum;
        List_pushFront(&shortest_job->events, (ListItem*) qe);
    }

    FakePCB* running = os->cpus[cpu_index].running;
    if (running) {
        ProcessEvent* current_burst = (ProcessEvent*) running->events.first;
        /* if a ready job shorter than the current running CPU burst is found,
         * then perform preemption */
        if (shortest_job->actual_burst < current_burst->duration
                && shortest_job->actual_burst > 0) {
            List_pushBack(&os->ready, (ListItem*) running);
            os->cpus[cpu_index].running = shortest_job;
            List_detach(&os->ready, (ListItem*) shortest_job);
            printf("\t\t\t[CPU %d] assigned to [pid %d], [pid %d] moved to ready \n",
                    cpu_index, shortest_job->pid, running->pid);
        }
        /* could not find any shorter job */
        return;
    }
    
    // if no processes are running on this CPU, just assign it to the shortest job.
    os->cpus[cpu_index].running = shortest_job;
    List_detach(&os->ready, (ListItem*) shortest_job);
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
