#pragma once
#include "linked_list.h"

#define NUM_CPU_BURSTS  25
#define NUM_IO_BURSTS   25
#define MAX_QUANTUM     10

typedef enum { CPU = 0, IO = 1 } ResourceType;

typedef struct ProcessEvent {        /* event of a process, it's in a list */
    ListItem list;
    ResourceType type;
    int duration;
} ProcessEvent;

typedef struct FakeProcess {
    ListItem list;
    int pid;            /* assigned by us */
    int arrival_time;
    ListHead events;

    int cpu_d[MAX_QUANTUM];        /* cpu bursts dataset */
    int io_d[MAX_QUANTUM];         /* io bursts dataset */
    double cpu_nd[MAX_QUANTUM];    /* cpu bursts normalized dataset */
    double io_nd[MAX_QUANTUM];     /* io bursts normalized dataset */
} FakeProcess;

int FakeProcess_load(FakeProcess*, const char *filename);
int FakeProcess_save(const FakeProcess*, const char *filename);

int generate_datasets(FakeProcess*, const char* filename);
