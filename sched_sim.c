#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
    int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, void *args_, int cpu_index) {
    SchedRRArgs* args = (SchedRRArgs*) args_;
    if (!os->ready.first) return;   /* no ready processes */
    FakePCB* pcb = (FakePCB*) List_popFront(&os->ready);    /* get first ready process */
    os->cpus[cpu_index].running = pcb;          /* assign the currently running process to the CPU */
    assert(pcb->events.first);  /* check it has at least an event */
    ProcessEvent* e = (ProcessEvent*) pcb->events.first;
    assert(e->type == CPU);

    /* if e->duration > quantum, then prepend to the events' list of this process 
     * a CPU event of duration quantum,
     * then alter the duration of the old event subtracting quantum */
    if (e->duration > args->quantum) {
        ProcessEvent* qe = (ProcessEvent*) malloc(sizeof(ProcessEvent));
        qe->list.prev = qe->list.next = 0;
        qe->type = CPU;
        qe->duration = args->quantum;
        e->duration -= args->quantum;
        List_pushFront(&pcb->events, (ListItem*) qe);
    }
}

int main(int argc, char **argv) {
    FakeOS_init(&os);
    SchedRRArgs srr_args;
    srr_args.quantum = 5;
    os.schedule_args = &srr_args;
    os.schedule_fn = schedRR;

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
