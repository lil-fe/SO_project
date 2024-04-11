#include "fake_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LENGTH 1024

static void FakeProcess_init(FakeProcess* p) {
    p->cpu_d = (int*) calloc(sizeof(int), p->max_quantum);
    p->io_d = (int*) calloc(sizeof(int), p->max_quantum);
    p->cpu_nd = (double*) calloc(sizeof(double), p->max_quantum);
    p->io_nd = (double*) calloc(sizeof(double), p->max_quantum);
}

/*
 * Normalize the CPU and IO burst duration histograms of the process p.
 *
 * Transforms the CPU and IO burst duration histograms into probability
 * distributions by dividing each element by the sum of all elements,
 * ensuring that the sum of probabilities equals 1.
 */
static void normalize_datasets(FakeProcess* p) {
    double sum = 0;
    for (int i=0; i<p->max_quantum; ++i) 
        sum += p->cpu_d[i];
    for (int i=0; i<p->max_quantum; ++i) 
        p->cpu_nd[i] = (double) p->cpu_d[i] / sum;

    sum = 0;
    for (int i=0; i<p->max_quantum; ++i) 
        sum += p->io_d[i];
    for (int i=0; i<p->max_quantum; ++i) 
        p->io_nd[i] = (double) p->io_d[i] / sum;
}

/*
 * Calculates the cumulative distributions for CPU and IO burst durations
 * based on their normalized probability distributions.
 */
static void cumulative_distributions(FakeProcess* p) {
    double cpu_tmp[p->max_quantum];
    double io_tmp[p->max_quantum];
    memcpy(cpu_tmp, p->cpu_nd, sizeof(double)*p->max_quantum);
    memcpy(io_tmp, p->io_nd, sizeof(double)*p->max_quantum);

    double cumulative_sum = cpu_tmp[0];
    for (int i=1; i < p->max_quantum; ++i) {
        cumulative_sum += cpu_tmp[i];
        p->cpu_nd[i] = cumulative_sum;
    }
    cumulative_sum = io_tmp[0];
    for (int i=1; i < p->max_quantum; ++i) {
        cumulative_sum += io_tmp[i];
        p->io_nd[i] = cumulative_sum;
    }
}

static void print_histograms(const FakeProcess* p) {
    printf("CPU bursts:\n");
    for (int i=0; i < p->max_quantum; ++i) {
        printf("quantum %d:\t", i+1);
        int cnt=0;
        for (int j=0; j < p->cpu_d[i]; ++j) { printf("@"); cnt++; }
        if (cnt < 8) printf("\t");  /* additional tab */
        printf("\t%d\n", cnt);
    }
    printf("IO bursts:\n");
    for (int i=0; i < p->max_quantum; ++i) {
        printf("quantum %d:\t", i+1);
        int cnt=0;
        for (int j=0; j < p->io_d[i]; ++j) { printf("@"); cnt++; }
        if (cnt < 8) printf("\t");  /* additional tab */
        printf("\t%d\n", cnt);
    
    }
}

static void print_distributions(const FakeProcess* p) {
    printf("CPU bursts:\n");
    for (int i=0; i < p->max_quantum; ++i) {
        printf("quantum %d:\t", i+1);
        printf("%.2f\n", p->cpu_nd[i]);
    }
    printf("IO bursts:\n");
    for (int i=0; i < p->max_quantum; ++i) {
        printf("quantum %d:\t", i+1);
        printf("%.2f\n", p->io_nd[i]);
    }
}

void generate_datasets(FakeProcess* p, const char* filename) {
    FakeProcess_init(p);
    FILE* f = fopen(filename, "r");
    if (!f) return;
    char* buffer = 0;
    size_t line_length = 0;
    p->pid = -1;
    p->arrival_time = -1;
    while (getline(&buffer, &line_length, f) > 0) {
        int pid = -1;
        int arrival_time = -1;
        int num_tokens = 0;
        int duration = 0;

        num_tokens = sscanf(buffer, "PROCESS %d %d", &pid, &arrival_time);
        if (num_tokens == 2 && p->pid < 0) {
            p->pid = pid;
            p->arrival_time = arrival_time;
            goto next_round;
        }
        
        num_tokens = sscanf(buffer, "CPU_BURST %d", &duration);
        if (num_tokens == 1) {
            p->cpu_d[duration-1]++;
            goto next_round;
        }

        num_tokens = sscanf(buffer, "IO_BURST %d", &duration);
        if (num_tokens == 1) {
            p->io_d[duration-1]++;
            goto next_round;
        }
next_round:;
    }
    
    printf("********** HISTOGRAMS OF PROCESS %d **********\n", p->pid);
    print_histograms(p);

    normalize_datasets(p);
    printf("*** PROBABILITY DISTRIBUTIONS OF PROCESS %d ***\n", p->pid);
    print_distributions(p);
    
    cumulative_distributions(p);
    printf("*** CUMULATIVE PROBABILITY DISTRIBUTIONS OF PROCESS %d ***\n", p->pid);
    print_distributions(p);

    if (buffer) free(buffer);
    fclose(f);
}

int FakeProcess_load(FakeProcess* p, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return -1;
    char* buffer = 0;
    size_t line_length = 0;
    List_init(&p->events);
    p->list.prev = p->list.next = 0;
    int num_events = 0;
    while (getline(&buffer, &line_length, f) > 0) {
        int num_tokens = 0;
        int duration = -1;     
        
        num_tokens = sscanf(buffer, "CPU_BURST %d", &duration);
        if (num_tokens == 1) {
            ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
            e->list.prev = e->list.next = 0;
            e->type = CPU;
            e->duration = duration;
            List_pushBack(&p->events, (ListItem*)e); 
            ++num_events;
            goto next_round;
        }

        num_tokens = sscanf(buffer, "IO_BURST %d", &duration);
        if (num_tokens == 1) {
            ProcessEvent* e = (ProcessEvent*) malloc(sizeof(ProcessEvent));
            e->list.prev = e->list.next = 0;
            e->type = IO;
            e->duration = duration;
            List_pushBack(&p->events, (ListItem*)e);
            ++num_events;
            goto next_round;
        }
next_round:;
    }    

    if (buffer) free (buffer);
    fclose(f);
    return num_events;
}

int FakeProcess_save(const FakeProcess* p, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "PROCESS %d %d\n", p->pid, p->arrival_time);
    ListItem* aux = p->events.first;
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
 
