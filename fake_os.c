#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "fake_os.h"

#define A 0.3

void FakeOS_init(FakeOS* os) {
    os->running = 0;
    List_init(&os->ready);      /* list of ready processes */
    List_init(&os->waiting);    /* list of waiting processes */
    List_init(&os->processes);  /* list of processes */
    os->timer = 0;
    os->schedule_fn = 0;

    int num_cpus;
    printf("enter the number of CPUS: ");
    scanf("%d", &num_cpus);
    if (num_cpus < 1) num_cpus = 1;     /* default to 1 CPU */
    os->num_cpus = num_cpus;
    os->cpus = (FakeCPU*) malloc(sizeof(FakeCPU) * os->num_cpus);
    for (int i=0; i < num_cpus; ++i) os->cpus[i].running = 0;
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
    assert(p->arrival_time == os->timer && "time mismatch in creation");
    assert((!os->running || os->running->pid != p->pid) && "pid taken");
    
    ListItem* aux = os->ready.first;  /* traverse list of ready processes */
    while (aux) {
        FakePCB* pcb = (FakePCB*) aux;
        assert(pcb->pid != p->pid && "pid taken");
        aux = aux->next;
    }

    aux = os->waiting.first;        /* traverse list of waiting processes */
    while (aux) {
        FakePCB* pcb = (FakePCB*) aux;
        assert(pcb->pid != p->pid && "pid taken");
        aux = aux->next;
    }

    // everything's fine, no such pcb exists
    FakePCB* new_pcb = (FakePCB*) malloc(sizeof(FakePCB));
    new_pcb->list.next = new_pcb->list.prev = 0;
    new_pcb->pid = p->pid;
    new_pcb->events = p->events;
    new_pcb->actual_burst = new_pcb->predicted_burst = 0;
    assert(new_pcb->events.first && "process without events");

    // depending on the first event's type, we put the process in either ready or waiting
    ProcessEvent* e = (ProcessEvent*) new_pcb->events.first;
    switch(e->type) {
    case CPU:
        List_pushBack(&os->ready, (ListItem*) new_pcb);   /* append e to running processes list */
        break;
    case IO:
        List_pushBack(&os->waiting, (ListItem*) new_pcb); /* append e to waiting processes list */
        break;
    default:
        assert(0 && "illegal resource");
        ;
    }
}

void FakeOS_simStep(FakeOS* os) {
    printf("************** TIME: %08d **************\n", os->timer);
    
    // scan the waiting process that is to be started, and create all the processes starting now
    ListItem* aux = os->processes.first;  /* traverse the list of processes */
    while(aux) {
        FakeProcess* proc = (FakeProcess*) aux;
        FakeProcess* new_process = 0;
        if (proc->arrival_time == os->timer) new_process = proc;
        aux = aux->next;
        if (new_process) {  /* remove new_process from processes list and create it in the OS */
            printf("\tcreate pid:%d\n", new_process->pid);
            new_process = (FakeProcess*) List_detach(&os->processes, (ListItem*) new_process);
            FakeOS_createProcess(os, new_process);
            free(new_process);
        }
    }

    // scan waiting list and set ready all the processes whose event terminates
    aux = os->waiting.first;
    while(aux) {
        FakePCB* pcb = (FakePCB*) aux;  /* create PCB for the waiting process */
        aux = aux->next;
        ProcessEvent* e = (ProcessEvent*) pcb->events.first;    /* its next event */
        printf("\twaiting pid: %d\n", pcb->pid);
        assert(e->type == IO);
        e->duration--;
        printf("\t\tremaining time:%d\n", e->duration);
        if (e->duration == 0) {     /* if the event (either CPU or IO burst) ended */
            printf("\t\tend burst\n");
            List_popFront(&pcb->events);    /* remove the event from the list of the events */
            free(e);
            List_detach(&os->waiting, (ListItem*) pcb);   /* remove process from waiting list */
            if (!pcb->events.first) {   /* if no left events, then kill the process */
                printf("\t\tend process\n");
                free(pcb);
            } else {    /* handle the next event */
                e = (ProcessEvent*) pcb->events.first;
                switch(e->type) {
                case CPU:
                    printf("\t\tmove to ready\n");
                    List_pushBack(&os->ready, (ListItem*) pcb);
                    break;
                case IO:
                    printf("\t\tmove to waiting\n");
                    List_pushBack(&os->waiting, (ListItem*) pcb);
                    break;
                }
            }
        }
    }
    
    for (int i=0; i < os->num_cpus; ++i) {
        FakeCPU* cpu = &os->cpus[i];
        FakePCB* running = cpu->running;
        printf("\trunning pid: %d [CPU %d]\n", running ? running->pid : -1, i);
        // decrement the duration of running
        // if the event is over, then destroy it and reschedule the process
        // if the ended event was the last one, then destroy the running process
        if (running) {
            ProcessEvent* e = (ProcessEvent*) running->events.first;
            assert(e->type == CPU);
            e->duration--;
            ++running->actual_burst; /* when leaves running, we need it to compute the prediction */
            printf("\t\tremaining time: %d\n", e->duration);
            if (e->duration == 0) {     /* if the event is over */
                printf("\t\tend burst\n");
                List_popFront(&running->events);
                free(e);
                if (!running->events.first) {   /* if the ended event was the last one */
                    printf("\t\tend process\n");
                    free(running);  /* kill process */
                } else {    /* the ended event wasn't the last one */
                    e = (ProcessEvent*) running->events.first; /* take the next event */
                    switch(e->type) {
                    case CPU:
                        printf("\t\tmove to ready\n");
                        List_pushBack(&os->ready, (ListItem*) running);
                        break;
                    case IO:
                        printf("\t\tmove to waiting\n");
                        List_pushBack(&os->waiting, (ListItem*) running);
                        break;
                    }
                }
                cpu->running = 0;
            }
        }
        if (os->schedule_fn && os->ready.first)
            (*os->schedule_fn)(os, os->schedule_args, i);
        if (!cpu->running && os->ready.first)
            cpu->running = (FakePCB*) List_popFront(&os->ready);
    }
    
    print_ready_processes(&os->ready);
    ++os->timer;
}

void FakeOS_destroy(FakeOS* os) {}

int is_any_cpu_free(FakeOS* os) {
    for (int i=0; i < os->num_cpus; ++i)
        if (!os->cpus[i].running) return 1;
    return 0;
}

/* A: the weight given to the actual burst time;
 * actual_burst: the actual burst time of the process;
 * predicted_burst: the previously predicted burst time, namely at time t.
 * The prediction result is the predicted burst at time t+1. */
float prediction(FakePCB* pcb) {
    if (!pcb->predicted_burst)
        return A*pcb->actual_burst + (1-A)*pcb->predicted_burst;
    return pcb->predicted_burst;
}

FakePCB* findShortestJob(ListHead* ready) {
    FakePCB* shortest_job = 0;
    float shortest_burst = FLT_MAX;
    ListItem* aux = ready->first;
    while (aux) {
        FakePCB* pcb = (FakePCB*) aux;
        float predicted_burst = prediction(pcb);
        
        if (predicted_burst != 0)
            printf("\t\t[pid %d] predicted_burst: %.2f\n", pcb->pid, predicted_burst);
        
        if (predicted_burst < shortest_burst) {
            shortest_burst = predicted_burst;
            shortest_job = pcb;
            shortest_job->actual_burst = shortest_burst;
        }
        aux = aux->next;
    }

    /*printf("\t[debug] shortest_job->actual_burst = %.2f of [pid %d]\n",
            shortest_job->actual_burst, shortest_job->pid); */
    if (shortest_job->actual_burst != 0)
        printf("\t\t\tshortest job : %d\n", shortest_job->pid);
    return shortest_job;
}

void print_ready_processes(ListHead* ready) {
    printf("\t\t##### READY LIST #####\n");
    ListItem* aux = ready->first;
    while (aux) {
        FakePCB* pcb = (FakePCB*) aux;
        printf("\t\t[pid %d] ", pcb->pid);
        aux = aux->next;
    }
    printf("\n");
}
