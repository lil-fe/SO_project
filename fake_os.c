#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"

void FakeOS_init(FakeOS* os) {
    os->running = 0;
    List_init(&os->ready);      /* list of ready processes */
    List_init(&os->waiting);    /* list of waiting processes */
    List_init(&os->processes);  /* list of processes */
    os->timer = 0;
    os->schedule_fn = 0;

    int num_cpus;
    printf("enter the number of CPUS: ");
    //scanf("%d", &(os->num_cpus));
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
    assert(new_pcb->events.first && "process without events");

    // depending on the first event's type, we put the process in either ready or waiting
    ProcessEvent* e = (ProcessEvent*) new_pcb->events.first;
    switch(e->type) {
    case CPU:
        List_pushBack(&os->ready, (ListItem*) new_pcb);   /* append e to running processes' list */
        break;
    case IO:
        List_pushBack(&os->waiting, (ListItem*) new_pcb); /* append e to waiting processes' list */
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
        if (new_process) {  /* remove new_process from processes' list and create it in the OS */
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
    
    FakeCPU* cpu = 0;
    FakePCB* running = 0;
    for (int i=0; i < os->num_cpus; ++i) {
        cpu = &os->cpus[i];
        running = cpu->running;
        printf("\trunning pid: %d [CPU %d]\n", running ? running->pid : -1, i);
        // decrement the duration of running
        // if the event is over, then destroy it and reschedule the process
        // if the ended event was the last one, then destroy the running process
        if (running) {
            ProcessEvent* e = (ProcessEvent*) running->events.first;
            assert(e->type == CPU);
            e->duration--;
            printf("\t\tremaining time: %d\n", e->duration);
            if (e->duration == 0) {     /* if the event is over */
                printf("\t\tend burst\n");
                List_popFront(&running->events);
                free(e);
                if (!running->events.first) {   /* if the ended event was the last one */
                    printf("\t\tend process\n");
                    free(running);  /* kill process */
                } else {    /* the ended event wasn't the last one */
                    e = (ProcessEvent*) running->events.first;
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
        if (os->schedule_fn && !cpu->running)
            (*os->schedule_fn)(os, os->schedule_args, i);
        if (!cpu->running && os->ready.first)
            cpu->running = (FakePCB*) List_popFront(&os->ready);
        /*if (!cpu->running && os->waiting.first) {
            FakePCB* pcb = (FakePCB*) os->waiting.first;
            ProcessEvent* e = (ProcessEvent*) pcb->events.first;
            assert(e->type == IO);
            e->duration = 0;    //skip the IO burst, since there's at least a free CPU
            List_popFront(&pcb->events);
            free(e);
            List_detach(&os->waiting, (ListItem*) pcb);
            if (!pcb->events.first) {   // no further events
                printf("\t\tend process\n"); 
                free(pcb);
            } else {    // take the next event and check it
                e = (ProcessEvent*) pcb->events.first;
                switch (e->type) {
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
        } */
    }

    ++os->timer;
}

void FakeOS_destroy(FakeOS* os) {}

int is_any_cpu_free(FakeOS* os) {
    for (int i=0; i < os->num_cpus; ++i)
        if (!os->cpus[i].running) return 1;
    return 0;
}
