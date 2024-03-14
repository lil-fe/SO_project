#include "fake_process.h"
#include <stdio.h>
#include <stdlib.h>

#define LINE_LENGTH 1024

int FakeProcess_load(FakeProcess* p, const char* filename) {
    FILE* f = fopen(filename, "r");       /* open the file stream */
    if (!f) return -1;
    char* buffer = 0;                   /* buffer for storing getline */
    size_t line_length = 0;
    p->pid = -1;                            /* initialize FakeProcess */
    p->arrival_time = -1;
    List_init(&p->events);
    p->list.prev = p->list.next = 0;
    int num_events = 0;
    while (getline(&buffer, &line_length, f) > 0) {     /* reading lines, there are 3 types */
        int pid = -1;
        int arrival_time = -1;
        int num_tokens = 0;
        int duration = -1;
        // reading first line from buffer through sscanf, storing pid and arrival_time
        num_tokens = sscanf(buffer, "PROCESS %d %d", &pid, &arrival_time);
        if (num_tokens == 2 && p->pid < 0) {    /* set pid and arrival_time for this process */
            p->pid = pid;
            p->arrival_time = arrival_time;
            goto next_round;                    /* skip to the next round */
        }
        // reading second line, storing duration of CPU_BURST
        num_tokens = sscanf(buffer, "CPU_BURST %d", &duration);
        if (num_tokens == 1) {
            // create a new event of type CPU_BURST
            ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
            e->list.prev = e->list.next = 0;
            e->type = CPU;
            e->duration = duration;
            List_pushBack(&p->events, (ListItem*)e);  /* append the event to the list */
            ++num_events;
            goto next_round;
        }
        // reading third line, storing duration of IO_BURST
        num_tokens = sscanf(buffer, "IO_BURST %d", &duration);
        if (num_tokens == 1) {
            // create a new event of type IO_BURST
            ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
            e->list.prev = e->list.next = 0;
            e->type = IO;
            e->duration = duration;
            List_pushBack(&p->events, (ListItem*)e);  /* append the event to the list */
            ++num_events;
            goto next_round;
        }
next_round:
        //printf("%stokens: %d\n", buffer, num_tokens);
        ;
    }
    if (buffer) free (buffer);
    fclose(f);
    return num_events;
}

int FakeProcess_save(const FakeProcess* p, const char* filename) {
    FILE* f = fopen(filename, "w");   /* open file stream to write */
    if (!f) return -1;
    fprintf(f, "PROCESS %d %d\n", p->pid, p->arrival_time); /* write pid and arrival_time to p */
    ListItem* aux = p->events.first;  /* traverse list of events */
    int num_events;
    while(aux) {
        ProcessEvent* e = (ProcessEvent*) aux;
        switch(e->type) {
        case CPU:
            fprintf(f, "CPU_BURST %d\n", e->duration);
            ++num_events;
            break;
        case IO:
            fprintf(f, "IO_BURST %d\n", e->duration);
            ++num_events;
            break;
        default:;
        }
        aux = aux->next;
    }
    fclose(f);
    return num_events;
}
