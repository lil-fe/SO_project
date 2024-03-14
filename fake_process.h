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
    int pid;            /* assigned by us */
    int arrival_time;
    ListHead events;
} FakeProcess;

int FakeProcess_load(FakeProcess*, const char *filename);
int FakeProcess_save(const FakeProcess*, const char *filename);
