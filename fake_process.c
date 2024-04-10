#include "fake_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LENGTH 1024

static void datasets_init(FakeProcess* p) {
    memset(p->cpu_d, 0, MAX_QUANTUM*sizeof(int));
    memset(p->io_d, 0, MAX_QUANTUM*sizeof(int));
    memset(p->cpu_nd, 0, MAX_QUANTUM*sizeof(double));
    memset(p->io_nd, 0, MAX_QUANTUM*sizeof(double));
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
    for (int i=0; i<MAX_QUANTUM; ++i) 
        sum += p->cpu_d[i];
    for (int i=0; i<MAX_QUANTUM; ++i) 
        p->cpu_nd[i] = (double) p->cpu_d[i] / sum;

    sum = 0;
    for (int i=0; i<MAX_QUANTUM; ++i) 
        sum += p->io_d[i];
    for (int i=0; i<MAX_QUANTUM; ++i) 
        p->io_nd[i] = (double) p->io_d[i] / sum;
}

/*
 * Calculates the cumulative distributions for CPU and IO burst durations
 * based on their normalized probability distributions.
 */
static void cumulative_distributions(FakeProcess* p) {
    double cpu_tmp[MAX_QUANTUM];
    double io_tmp[MAX_QUANTUM];
    memcpy(cpu_tmp, p->cpu_nd, sizeof(double)*MAX_QUANTUM);
    memcpy(io_tmp, p->io_nd, sizeof(double)*MAX_QUANTUM);

    double cumulative_sum = cpu_tmp[0];
    for (int i=1; i < MAX_QUANTUM; ++i) {
        cumulative_sum += cpu_tmp[i];
        p->cpu_nd[i] = cumulative_sum;
    }
    cumulative_sum = io_tmp[0];
    for (int i=1; i < MAX_QUANTUM; ++i) {
        cumulative_sum += io_tmp[i];
        p->io_nd[i] = cumulative_sum;
    }
}

static void print_histogram(const void* array, char type) {
    if (type == 'i') {
        int* arr = (int*) array;
        for (int i=0; i<MAX_QUANTUM; ++i) {
            printf("quantum %d:\t", i+1);
            int cnt=0;
            for (int j=0; j<arr[i]; ++j) { printf("@"); cnt++; }
            if (cnt < 8) printf("\t");
            printf("\t%d\n", cnt);
        }
    } else if (type == 'd') {
        double* arr = (double*) array;
        for (int i=0; i<MAX_QUANTUM; ++i) {
            printf("quantum %d:\t", i+1);
            printf("%.2f\n", arr[i]);
        }
    } else {
        printf("could not print histogram of type %c\n", type);
        printf("only available types: 'i' for int, 'd' for double\n");
    }
}

int generate_datasets(FakeProcess* p, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return -1;
    char* buffer = 0;
    size_t line_length = 0;
    p->pid = -1;
    p->arrival_time = -1;
    datasets_init(p);
    int num_bursts = 0;
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
            num_bursts++;
            goto next_round;
        }

        num_tokens = sscanf(buffer, "IO_BURST %d", &duration);
        if (num_tokens == 1) {
            p->io_d[duration-1]++;
            num_bursts++;
            goto next_round;
        }
next_round:;
    }
    
    printf("********** HISTOGRAMS OF PROCESS %d **********\n", p->pid);
    printf("cpu_d:\n"); print_histogram(p->cpu_d, 'i');
    printf("io_d:\n"); print_histogram(p->io_d, 'i');
    
    normalize_datasets(p);
    printf("*** PROBABILITY DISTRIBUTIONS OF PROCESS %d ***\n", p->pid);
    printf("cpu_nd:\n"); print_histogram(p->cpu_nd, 'd');
    printf("io_nd:\n"); print_histogram(p->io_nd, 'd');
    
    cumulative_distributions(p);
    printf("*** CUMULATIVE PROBABILITY DISTRIBUTIONS OF PROCESS %d ***\n", p->pid);
    printf("cpu_nd:\n"); print_histogram(p->cpu_nd, 'd');
    printf("io_nd:\n"); print_histogram(p->io_nd, 'd');

    if (buffer) free(buffer);
    fclose(f);
    return num_bursts;
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
            //p->cpu_d[duration-1]++;
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
            //p->io_d[duration-1]++;
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
 
