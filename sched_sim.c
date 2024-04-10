#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
    int quantum;
} SchedSJFArgs;

void schedSJF(FakeOS* os, void *args_, int cpu_index) {
    SchedSJFArgs* args = (SchedSJFArgs*) args_;
    if (!os->ready.first) return;   /* no ready processes */
    FakePCB* shortest_job = findShortestJob(&os->ready);
    if (!shortest_job) return;      /* no process found */
    os->cpus[cpu_index].running = shortest_job; 
    shortest_job->actual_burst = 0;
    List_detach(&os->ready, (ListItem*) shortest_job);
    ProcessEvent* e = (ProcessEvent*)shortest_job->events.first;
    /* if e->duration > quantum, then prepend to the events' list of this process 
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
    srand(time(NULL));
    
    FakeOS_init(&os);
    SchedSJFArgs sjf_args;
    sjf_args.quantum = 5;
    os.schedule_args = &sjf_args;
    os.schedule_fn = schedSJF;
     
    char flag = 'a';
    while (flag != 'y' && flag != 'n') {
        printf("generate random input files? (y/n): ");
        scanf(" %c", &flag);
    }

    for (int i=1; i < argc; ++i) {
        if (flag == 'y')
            generate_file(i, NUM_CPU_BURSTS + NUM_IO_BURSTS, argv[i]);
        FakeProcess new_process;
        int num_bursts = generate_datasets(&new_process, argv[i]);
        generate_samples(&new_process, num_bursts);
        char filename[50];
        sprintf(filename, "processes/sampled_p%d.txt", i);
        int num_events = FakeProcess_load(&new_process, filename);
        printf("loading [%s], pid: %d, num_events: %d\n",
                filename, new_process.pid, num_events);
        if (num_events) {
            FakeProcess* new_process_ptr = (FakeProcess*) malloc(sizeof(FakeProcess));
            *new_process_ptr = new_process;
            List_pushBack(&os.processes, (ListItem*) new_process_ptr); 
        }
    }
    printf("num processes in queue %d\n", os.processes.size);
    while(is_any_cpu_running(&os) || os.ready.first || os.waiting.first || os.processes.first)
        FakeOS_simStep(&os);
}
