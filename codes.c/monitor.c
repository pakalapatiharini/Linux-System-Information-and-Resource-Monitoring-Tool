#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>

#include "monitor.h"
#include "logger.h"

static double cpu_threshold = 80.0;
static double memory_threshold = 512.0;

static Process previous_processes[MAX_PROCESSES];
static int previous_process_count = 0;

static unsigned long long get_total_cpu_ticks(void)
{
    FILE *file;
    char line[512];

    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    file = fopen("/proc/stat", "r");

    if (file == NULL)
    {
        return 0;
    }

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    user = 0;
    nice = 0;
    system = 0;
    idle = 0;
    iowait = 0;
    irq = 0;
    softirq = 0;
    steal = 0;

    sscanf(
        line,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &user,
        &nice,
        &system,
        &idle,
        &iowait,
        &irq,
        &softirq,
        &steal
    );

    return user + nice + system + idle +
           iowait + irq + softirq + steal;
}

static unsigned long long get_idle_cpu_ticks(void)
{
    FILE *file;
    char line[512];

    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    file = fopen("/proc/stat", "r");

    if (file == NULL)
    {
        return 0;
    }

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    user = 0;
    nice = 0;
    system = 0;
    idle = 0;
    iowait = 0;
    irq = 0;
    softirq = 0;
    steal = 0;

    sscanf(
        line,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &user,
        &nice,
        &system,
        &idle,
        &iowait,
        &irq,
        &softirq,
        &steal
    );

    return idle + iowait;
}

double get_cpu_usage(void)
{
    unsigned long long total1;
    unsigned long long idle1;

    unsigned long long total2;
    unsigned long long idle2;

    unsigned long long total_difference;
    unsigned long long idle_difference;

    double usage;

    total1 = get_total_cpu_ticks();
    idle1 = get_idle_cpu_ticks();

    usleep(500000);

    total2 = get_total_cpu_ticks();
    idle2 = get_idle_cpu_ticks();

    total_difference = total2 - total1;
    idle_difference = idle2 - idle1;

    if (total_difference == 0)
    {
        return 0.0;
    }

    usage =
        (1.0 -
         ((double)idle_difference /
          (double)total_difference))
        * 100.0;

    if (usage < 0.0)
    {
        usage = 0.0;
    }

    if (usage > 100.0)
    {
        usage = 100.0;
    }

    return usage;
}

static double get_process_memory(int pid)
{
    char path[256];
    char line[512];

    FILE *file;

    double memory_kb = 0.0;

    snprintf(
        path,
        sizeof(path),
        "/proc/%d/status",
        pid
    );

    file = fopen(path, "r");

    if (file == NULL)
    {
        return 0.0;
    }

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(
                line + 6,
                "%lf",
                &memory_kb
            );

            break;
        }
    }

    fclose(file);

    return memory_kb / 1024.0;
}

int read_process(int pid, Process *p)
{
    char path[256];
    char line[1024];

    FILE *file;

    char process_name[256];
    char state;

    snprintf(
        path,
        sizeof(path),
        "/proc/%d/stat",
        pid
    );

    file = fopen(path, "r");

    if (file == NULL)
    {
        return 0;
    }

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    process_name[0] = '\0';
    state = '?';

    if (sscanf(
            line,
            "%*d (%255[^)]) %c",
            process_name,
            &state) != 2)
    {
        return 0;
    }

    p->pid = pid;

    strncpy(
        p->name,
        process_name,
        sizeof(p->name) - 1
    );

    p->name[sizeof(p->name) - 1] = '\0';

    p->state = state;

    /*
     * Per-process CPU percentage is not calculated
     * in this version.
     */
    p->cpu_usage = 0.0;

    p->memory_usage = get_process_memory(pid);

    return 1;
}

int get_process_list(
    Process processes[],
    int max_processes
)
{
    DIR *directory;
    struct dirent *entry;

    int count = 0;

    directory = opendir("/proc");

    if (directory == NULL)
    {
        return 0;
    }

    while ((entry = readdir(directory)) != NULL)
    {
        int is_pid = 1;
        int i;

        if (entry->d_name[0] == '\0')
        {
            continue;
        }

        for (i = 0; entry->d_name[i] != '\0'; i++)
        {
            if (!isdigit(
                    (unsigned char)entry->d_name[i]))
            {
                is_pid = 0;
                break;
            }
        }

        if (!is_pid)
        {
            continue;
        }

        if (count >= max_processes)
        {
            break;
        }

        {
            int pid = atoi(entry->d_name);

            if (read_process(pid, &processes[count]))
            {
                count++;
            }
        }
    }

    closedir(directory);

    return count;
}

void display_system_info(void)
{
    FILE *cpu_file;
    FILE *memory_file;

    char line[512];

    char model_name[256] = "Unknown";
    double memory_total = 0.0;
    double memory_available = 0.0;
    double memory_used = 0.0;
    double memory_usage_percentage = 0.0;

    double cpu_usage;

    struct statvfs disk;

    struct sysinfo system_info;

    printf("\n");
    printf("============================================\n");
    printf("             SYSTEM INFORMATION\n");
    printf("============================================\n");

    cpu_file = fopen("/proc/cpuinfo", "r");

    if (cpu_file != NULL)
    {
        while (fgets(line, sizeof(line), cpu_file))
        {
            if (strncmp(
                    line,
                    "model name",
                    10) == 0)
            {
                char *colon = strchr(line, ':');

                if (colon != NULL)
                {
                    strncpy(
                        model_name,
                        colon + 2,
                        sizeof(model_name) - 1
                    );

                    model_name[
                        sizeof(model_name) - 1
                    ] = '\0';

                    model_name[
                        strcspn(
                            model_name,
                            "\n"
                        )
                    ] = '\0';

                    break;
                }
            }
        }

        fclose(cpu_file);
    }

    printf(
        "CPU:              %s\n",
        model_name
    );

    cpu_usage = get_cpu_usage();

    printf(
        "CPU Usage:        %.2f%%\n",
        cpu_usage
    );

    memory_file = fopen("/proc/meminfo", "r");

    if (memory_file != NULL)
    {
        while (fgets(line, sizeof(line), memory_file))
        {
            if (strncmp(
                    line,
                    "MemTotal:",
                    9) == 0)
            {
                sscanf(
                    line + 9,
                    "%lf",
                    &memory_total
                );
            }
            else if (strncmp(
                         line,
                         "MemAvailable:",
                         13) == 0)
            {
                sscanf(
                    line + 13,
                    "%lf",
                    &memory_available
                );
            }
        }

        fclose(memory_file);
    }

    memory_used =
        memory_total - memory_available;

    if (memory_total > 0)
    {
        memory_usage_percentage =
            (memory_used / memory_total) * 100.0;
    }

    printf(
        "Memory Usage:     %.2f%%\n",
        memory_usage_percentage
    );

    printf(
        "Memory Used:      %.2f MB\n",
        memory_used / 1024.0
    );

    printf(
        "Memory Total:     %.2f MB\n",
        memory_total / 1024.0
    );

    if (statvfs("/", &disk) == 0)
    {
        unsigned long long total_space =
            (unsigned long long)
            disk.f_blocks *
            disk.f_frsize;

        unsigned long long free_space =
            (unsigned long long)
            disk.f_bfree *
            disk.f_frsize;

        unsigned long long used_space =
            total_space - free_space;

        double disk_usage = 0.0;

        if (total_space > 0)
        {
            disk_usage =
                ((double)used_space /
                 (double)total_space) *
                100.0;
        }

        printf(
            "Disk Usage:       %.2f%%\n",
            disk_usage
        );
    }
    else
    {
        printf(
            "Disk Usage:       Unable to read\n"
        );
    }

    if (sysinfo(&system_info) == 0)
    {
        printf(
            "System Uptime:    %ld seconds\n",
            system_info.uptime
        );
    }
    else
    {
        printf(
            "System Uptime:    Unable to read\n"
        );
    }

    printf(
        "CPU Threshold:    %.2f%%\n",
        cpu_threshold
    );

    printf(
        "Memory Threshold: %.2f MB\n",
        memory_threshold
    );

    printf(
        "============================================\n"
    );
}

void display_processes(void)
{
    Process processes[MAX_PROCESSES];

    int count;
    int i;

    count =
        get_process_list(
            processes,
            MAX_PROCESSES
        );

    printf("\n");
    printf("================================================================\n");
    printf("                    PROCESS INFORMATION\n");
    printf("================================================================\n");

    printf(
        "%-8s %-30s %-8s %-12s\n",
        "PID",
        "NAME",
        "STATE",
        "MEMORY(MB)"
    );

    printf(
        "----------------------------------------------------------------\n"
    );

    for (i = 0; i < count; i++)
    {
        printf(
            "%-8d %-30.30s %-8c %-12.2f\n",
            processes[i].pid,
            processes[i].name,
            processes[i].state,
            processes[i].memory_usage
        );
    }

    printf(
        "================================================================\n"
    );

    printf(
        "Total processes detected: %d\n",
        count
    );
}

void set_cpu_threshold(double value)
{
    if (value < 0.0)
    {
        value = 0.0;
    }

    if (value > 100.0)
    {
        value = 100.0;
    }

    cpu_threshold = value;
}

void set_memory_threshold(double value)
{
    if (value < 0.0)
    {
        value = 0.0;
    }

    memory_threshold = value;
}

double get_cpu_threshold(void)
{
    return cpu_threshold;
}

double get_memory_threshold(void)
{
    return memory_threshold;
}

void check_resource_thresholds(void)
{
    Process processes[MAX_PROCESSES];

    int count;
    int i;

    char message[512];

    double cpu_usage;

    printf("\n");
    printf("============================================\n");
    printf("             RESOURCE CHECK\n");
    printf("============================================\n");

    cpu_usage = get_cpu_usage();

    printf(
        "Current CPU Usage: %.2f%%\n",
        cpu_usage
    );

    printf(
        "CPU Threshold:     %.2f%%\n",
        cpu_threshold
    );

    if (cpu_usage > cpu_threshold)
    {
        printf(
            "WARNING: CPU usage exceeded threshold!\n"
        );

        snprintf(
            message,
            sizeof(message),
            "CPU usage %.2f%% exceeded threshold %.2f%%",
            cpu_usage,
            cpu_threshold
        );

        log_event(message);
    }
    else
    {
        printf(
            "CPU usage is within the configured limit.\n"
        );
    }

    count =
        get_process_list(
            processes,
            MAX_PROCESSES
        );

    printf("\n");

    printf(
        "Checking process memory usage...\n"
    );

    for (i = 0; i < count; i++)
    {
        if (processes[i].memory_usage >
            memory_threshold)
        {
            printf(
                "WARNING: PID %d (%s) uses %.2f MB\n",
                processes[i].pid,
                processes[i].name,
                processes[i].memory_usage
            );

            snprintf(
                message,
                sizeof(message),
                "PID %d (%s) memory %.2f MB exceeded threshold %.2f MB",
                processes[i].pid,
                processes[i].name,
                processes[i].memory_usage,
                memory_threshold
            );

            log_event(message);
        }
    }

    printf(
        "============================================\n"
    );
}

void initialize_process_tracking(void)
{
    previous_process_count =
        get_process_list(
            previous_processes,
            MAX_PROCESSES
        );
}

static int process_exists(
    Process processes[],
    int count,
    int pid
)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (processes[i].pid == pid)
        {
            return 1;
        }
    }

    return 0;
}

void check_process_changes(void)
{
    Process current_processes[MAX_PROCESSES];

    int current_count;
    int i;

    char message[512];

    current_count =
        get_process_list(
            current_processes,
            MAX_PROCESSES
        );

    for (i = 0; i < current_count; i++)
    {
        if (!process_exists(
                previous_processes,
                previous_process_count,
                current_processes[i].pid))
        {
            printf(
                "Process created: PID %d (%s)\n",
                current_processes[i].pid,
                current_processes[i].name
            );

            snprintf(
                message,
                sizeof(message),
                "Process created: PID %d (%s)",
                current_processes[i].pid,
                current_processes[i].name
            );

            log_event(message);
        }
    }

    for (i = 0; i < previous_process_count; i++)
    {
        if (!process_exists(
                current_processes,
                current_count,
                previous_processes[i].pid))
        {
            printf(
                "Process terminated: PID %d (%s)\n",
                previous_processes[i].pid,
                previous_processes[i].name
            );

            snprintf(
                message,
                sizeof(message),
                "Process terminated: PID %d (%s)",
                previous_processes[i].pid,
                previous_processes[i].name
            );

            log_event(message);
        }
    }

    memcpy(
        previous_processes,
        current_processes,
        sizeof(Process) * current_count
    );

    previous_process_count = current_count;
}
