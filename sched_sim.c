#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  int quantum;
} SchedRRArgs;

typedef struct {
  int quantum;
} SchedDebugArgs;

void schedRR(FakeOS* os, void* args_){
  SchedRRArgs* args=(SchedRRArgs*)args_;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;

  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running=pcb;
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
};

/**
 * Just a debugging schedule_fn.
*/
void sched_debug(FakeOS* os, void* args_){
  SchedDebugArgs* args = (SchedDebugArgs*)args_;

  // Look for the first process in ready. If none, return
  if (!os->ready.first)
    return;

  // Look for a free CPU. If none, return
  int i;
  for (i=0; i<os->num_cpus; ++i) {
    if (!os->cpus[i].running_process) {
      fprintf(stdout, "Good News!. CPU %d is free!\n", i); // debug
      break;
    }
  }
  if (i == os->num_cpus) {
    fprintf(stdout, "None of the CPUs is free.\n"); //debug
    return;
  }

  FakePCB* pcb = (FakePCB*)List_popFront(&os->ready);
  os->cpus[i].running_process = pcb;

  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type == CPU);

  if (e->duration > args->quantum) {
    ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev = qe->list.next = 0;
    qe->type = CPU;
    qe->duration = args->quantum;
    e->duration -= args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
}

int main(int argc, char** argv) {
  FakeOS_init(&os);
  //SchedRRArgs srr_args;
  //srr_args.quantum=5;
  //os.schedule_args=&srr_args;
  //os.schedule_fn=schedRR;
  SchedDebugArgs sdb_args;
  sdb_args.quantum=5;
  os.schedule_args=&sdb_args;
  os.schedule_fn=sched_debug;
  
  for (int i=1; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);
  while(os.running
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
}
