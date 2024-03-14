#include "fake_process.h"
#include "linked_list.h"
#pragma once

typedef struct FakePCB {
    ListItem list;
    int pid;
    ListHead events;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void *args, int cpu_index);

typedef struct {
    FakePCB* running;
} FakeCPU;

typedef struct FakeOS {
    FakePCB *running;
    ListHead ready;             /* list of ready processes */
    ListHead waiting;           /* list of waiting processes */
    int timer;
    ScheduleFn schedule_fn;
    void *schedule_args;
    ListHead processes;         /* list of processes */
    
    FakeCPU* cpus;              /* list of cpus */
    int num_cpus;
} FakeOS;

void FakeOS_init(FakeOS *os);
void FakeOS_simStep(FakeOS *os);
void FakeOS_destroy(FakeOS *os);

int is_any_cpu_free(FakeOS* os);
