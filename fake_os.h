#include "fake_process.h"
#include "linked_list.h"
#pragma once

#define A 0.5

typedef struct {
  ListItem list;
  int pid;
  ListHead events;

  int is_predicted;
  double predicted_quantum;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args);

typedef struct {
  int cpu_id; // Unique CPU identifier (debug)
  FakePCB* running_process; // Pointer to the currently running process on the CPU
} FakeCPU;

typedef struct FakeOS{
  FakePCB* running;
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;

  FakeCPU* cpus; // Array of CPUs
  int num_cpus; // Number of CPUs in the system
  ListHead ready_tmp;
} FakeOS;

void FakeOS_init(FakeOS* os);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);

int is_any_cpu_free(FakeOS* os);