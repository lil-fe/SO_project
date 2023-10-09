#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  int quantum;
} SchedSJFArgs;


int is_any_cpu_free(FakeOS* os) {
  for (int i=0; i<os->num_cpus; i++) {
    if (!os->cpus[i].running_process) {
      return 1;
    }
  }
  return 0;
}

void schedSJF(FakeOS* os, void* args_) {
  SchedSJFArgs* args = (SchedSJFArgs*)args_;
  FakePCB* shortestJob = (FakePCB*)os->ready.first;

  // Controllo del processo con predicted_quantum più basso
  FakePCB* pcb = NULL;
  ListItem* aux = os->ready.first;
  while (aux) {
    pcb = (FakePCB*) aux;

    // Se il predicted quantum è già stato calcolato, non calcolarlo di nuovo
    if (!pcb->is_predicted) {
      pcb->is_predicted=1;
      pcb->predicted_quantum = A*args->quantum * (1-A)*pcb->predicted_quantum;
    }
    if (!shortestJob->is_predicted) {
      shortestJob->is_predicted=1;
      shortestJob->predicted_quantum = A*args->quantum * (1-A)*shortestJob->predicted_quantum;
    }

    if (pcb->predicted_quantum < shortestJob->predicted_quantum)
      shortestJob = pcb;
    aux = aux->next;
  }

  ProcessEvent* e = (ProcessEvent*)shortestJob->events.first;
  if (e->duration>args->quantum) {
    ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&shortestJob->events,(ListItem*)qe);
  }
  
  shortestJob->is_predicted=0;
  shortestJob->predicted_quantum=0;
}

int main(int argc, char** argv) {
  FakeOS_init(&os);
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

  SchedSJFArgs args;
  printf("\nEnter the quantum: ");
  scanf("%d", &args.quantum);
  os.schedule_args=&args;
  os.schedule_fn=schedSJF;

  printf("num processes in queue %d\n", os.processes.size);
  while(!is_any_cpu_free(&os)
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
}
