#pragma once
#include "linked_list.h"

typedef enum { CPU = 0, IO = 1 } ResourceType;

typedef struct ProcessEvent {        /* event of a process, it's in a list */
    ListItem list;
    ResourceType type;
    int duration;
} ProcessEvent;

typedef struct FakeProcess {
    ListItem list;
    int pid;
    int arrival_time;
    ListHead events;

    int* cpu_d;         /* cpu bursts dataset */
    int* io_d;          /* io bursts dataset */
    double* cpu_nd;     /* cpu bursts normalized dataset */
    double* io_nd;      /* io bursts normalized dataset */
    // max_quantum determines the "size" of the histogram,
    // namely how many durations (quantum of duration 1, 2, ...)
    int max_quantum;
    int num_bursts;     /* number of bursts to be generated */
} FakeProcess;

int FakeProcess_load(FakeProcess*, const char *filename);
int FakeProcess_save(const FakeProcess*, const char *filename);

void generate_datasets(FakeProcess*, const char* filename);
