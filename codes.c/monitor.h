#ifndef MONITOR_H
#define MONITOR_H

#define MAX_PROCESSES 1024

typedef struct
{
    int pid;
    char name[256];
    char state;
    double cpu_usage;
    double memory_usage;
} Process;

int get_process_list(Process processes[], int max_processes);
int read_process(int pid, Process *p);

void display_system_info(void);
void display_processes(void);

double get_cpu_usage(void);

void set_cpu_threshold(double value);
void set_memory_threshold(double value);

double get_cpu_threshold(void);
double get_memory_threshold(void);

void check_resource_thresholds(void);

void initialize_process_tracking(void);
void check_process_changes(void);

#endif
