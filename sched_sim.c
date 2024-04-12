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
    
    /*
    char flag = 'a';
    while (flag != 'y' && flag != 'n') {
        printf("generate random input files? (y/n): ");
        scanf(" %c", &flag);
    }*/

    int max_quantum;
    printf("enter the maximum duration of each burst: ");
    scanf(" %d", &max_quantum);
    assert(max_quantum>=1 && "negative or null value has been entered");
    
    SchedSJFArgs sjf_args;
    int quantum;
    printf("enter the quantum: ");
    scanf(" %d", &quantum);
    assert(quantum>=1 && "negative or null value has been entered");
    sjf_args.quantum = quantum;
    os.schedule_args = &sjf_args;
    os.schedule_fn = schedSJF; 

    for (int i=1; i < argc; ++i) {
        FakeProcess new_process;
        new_process.max_quantum = max_quantum;

        //if (flag == 'y')
        //    generate_file(argv[i], i, os.num_bursts, new_process.max_quantum);
        generate_file(argv[i], i, os.num_bursts, new_process.max_quantum);
        generate_datasets(&new_process, argv[i]);
        generate_samples(&new_process, os.num_bursts);
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
    FakeOS_destroy(&os);
}
