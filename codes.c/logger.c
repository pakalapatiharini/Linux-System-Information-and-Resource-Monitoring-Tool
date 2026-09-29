#include <stdio.h>
#include <time.h>

#include "logger.h"

void log_event(const char *message)
{
    FILE *file;
    time_t current_time;
    struct tm *time_info;
    char time_string[50];

    file = fopen("logs/monitor.log", "a");

    if (file == NULL)
    {
        printf("Unable to open log file.\n");
        return;
    }

    time(&current_time);
    time_info = localtime(&current_time);

    if (time_info == NULL)
    {
        fclose(file);
        return;
    }

    strftime(
        time_string,
        sizeof(time_string),
        "%Y-%m-%d %H:%M:%S",
        time_info
    );

    fprintf(
        file,
        "[%s] %s\n",
        time_string,
        message
    );

    fclose(file);
}

void show_logs(void)
{
    FILE *file;
    char line[512];

    file = fopen("logs/monitor.log", "r");

    if (file == NULL)
    {
        printf("\nNo log file found.\n");
        return;
    }

    printf("\n");
    printf("========================================\n");
    printf("           MONITOR LOG\n");
    printf("========================================\n");

    while (fgets(line, sizeof(line), file))
    {
        printf("%s", line);
    }

    printf("========================================\n");

    fclose(file);
}
