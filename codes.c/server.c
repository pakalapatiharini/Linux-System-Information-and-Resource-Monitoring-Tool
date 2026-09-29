#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#include "monitor.h"
#include "logger.h"

#define PORT 8080
#define BUFFER_SIZE 65536

static volatile int monitoring = 0;
static volatile int server_running = 1;

static pthread_t monitoring_thread;
static int monitoring_thread_created = 0;

static void send_response(
    int client_socket,
    const char *response
)
{
    send(
        client_socket,
        response,
        strlen(response),
        0
    );
}

static void process_control(
    int client_socket,
    const char *command
)
{
    int pid;

    char response[512];
    char action[20];

    if (sscanf(
            command,
            "%19s %d",
            action,
            &pid) != 2)
    {
        send_response(
            client_socket,
            "Invalid process control command.\n"
        );

        return;
    }

    if (strcmp(action, "STOP") == 0)
    {
        if (kill(pid, SIGSTOP) == 0)
        {
            snprintf(
                response,
                sizeof(response),
                "Process %d stopped successfully.\n",
                pid
            );

            log_event(response);

            send_response(
                client_socket,
                response
            );
        }
        else
        {
            snprintf(
                response,
                sizeof(response),
                "Unable to stop process %d: %s\n",
                pid,
                strerror(errno)
            );

            send_response(
                client_socket,
                response
            );
        }
    }
    else if (strcmp(action, "CONT") == 0)
    {
        if (kill(pid, SIGCONT) == 0)
        {
            snprintf(
                response,
                sizeof(response),
                "Process %d continued successfully.\n",
                pid
            );

            log_event(response);

            send_response(
                client_socket,
                response
            );
        }
        else
        {
            snprintf(
                response,
                sizeof(response),
                "Unable to continue process %d: %s\n",
                pid,
                strerror(errno)
            );

            send_response(
                client_socket,
                response
            );
        }
    }
    else if (strcmp(action, "TERM") == 0)
    {
        if (kill(pid, SIGTERM) == 0)
        {
            snprintf(
                response,
                sizeof(response),
                "Process %d terminated successfully.\n",
                pid
            );

            log_event(response);

            send_response(
                client_socket,
                response
            );
        }
        else
        {
            snprintf(
                response,
                sizeof(response),
                "Unable to terminate process %d: %s\n",
                pid,
                strerror(errno)
            );

            send_response(
                client_socket,
                response
            );
        }
    }
    else
    {
        send_response(
            client_socket,
            "Unknown process control command.\n"
        );
    }
}

static void *monitoring_function(void *argument)
{
    (void)argument;

    while (monitoring)
    {
        check_process_changes();
        check_resource_thresholds();

        sleep(2);
    }

    return NULL;
}

static void start_monitoring(int client_socket)
{
    if (monitoring)
    {
        send_response(
            client_socket,
            "Continuous monitoring is already running.\n"
        );

        return;
    }

    initialize_process_tracking();

    monitoring = 1;

    if (pthread_create(
            &monitoring_thread,
            NULL,
            monitoring_function,
            NULL) != 0)
    {
        monitoring = 0;

        send_response(
            client_socket,
            "Unable to start monitoring thread.\n"
        );

        return;
    }

    monitoring_thread_created = 1;

    log_event(
        "Continuous monitoring started."
    );

    send_response(
        client_socket,
        "Continuous monitoring started.\n"
    );
}

static void stop_monitoring(int client_socket)
{
    if (!monitoring)
    {
        send_response(
            client_socket,
            "Continuous monitoring is not running.\n"
        );

        return;
    }

    monitoring = 0;

    if (monitoring_thread_created)
    {
        pthread_join(
            monitoring_thread,
            NULL
        );

        monitoring_thread_created = 0;
    }

    log_event(
        "Continuous monitoring stopped."
    );

    send_response(
        client_socket,
        "Continuous monitoring stopped.\n"
    );
}

static void handle_command(
    int client_socket,
    const char *command
)
{
    if (strcmp(command, "SYSTEM") == 0)
    {
        display_system_info();

        send_response(
            client_socket,
            "System information displayed on server terminal.\n"
        );
    }
    else if (strcmp(command, "PROCESS") == 0)
    {
        display_processes();

        send_response(
            client_socket,
            "Process information displayed on server terminal.\n"
        );
    }
    else if (strncmp(
                 command,
                 "SETCPU ",
                 7) == 0)
    {
        double value;

        if (sscanf(
                command + 7,
                "%lf",
                &value) == 1)
        {
            char response[256];

            set_cpu_threshold(value);

            snprintf(
                response,
                sizeof(response),
                "CPU threshold set to %.2f%%.\n",
                get_cpu_threshold()
            );

            log_event(response);

            send_response(
                client_socket,
                response
            );
        }
        else
        {
            send_response(
                client_socket,
                "Invalid CPU threshold.\n"
            );
        }
    }
    else if (strncmp(
                 command,
                 "SETMEM ",
                 7) == 0)
    {
        double value;

        if (sscanf(
                command + 7,
                "%lf",
                &value) == 1)
        {
            char response[256];

            set_memory_threshold(value);

            snprintf(
                response,
                sizeof(response),
                "Memory threshold set to %.2f MB.\n",
                get_memory_threshold()
            );

            log_event(response);

            send_response(
                client_socket,
                response
            );
        }
        else
        {
            send_response(
                client_socket,
                "Invalid memory threshold.\n"
            );
        }
    }
    else if (strcmp(command, "CHECK") == 0)
    {
        check_resource_thresholds();

        send_response(
            client_socket,
            "Resource check completed. See server terminal for details.\n"
        );
    }
    else if (strcmp(command, "START") == 0)
    {
        start_monitoring(client_socket);
    }
    else if (strcmp(command, "MONITORSTOP") == 0)
    {
        stop_monitoring(client_socket);
    }
    else if (strcmp(command, "LOGS") == 0)
    {
        show_logs();

        send_response(
            client_socket,
            "Logs displayed on server terminal.\n"
        );
    }
    else if (
        strncmp(command, "STOP ", 5) == 0 ||
        strncmp(command, "CONT ", 5) == 0 ||
        strncmp(command, "TERM ", 5) == 0)
    {
        process_control(
            client_socket,
            command
        );
    }
    else if (strcmp(command, "EXIT") == 0)
    {
        send_response(
            client_socket,
            "Closing client connection.\n"
        );
    }
    else
    {
        send_response(
            client_socket,
            "Unknown command.\n"
        );
    }
}

int main(void)
{
    int server_socket;
    int client_socket;

    int option = 1;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_length;

    char buffer[BUFFER_SIZE];

    server_socket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (server_socket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    if (setsockopt(
            server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)) < 0)
    {
        perror("setsockopt failed");

        close(server_socket);

        return 1;
    }

    memset(
        &server_address,
        0,
        sizeof(server_address)
    );

    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(PORT);

    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) < 0)
    {
        perror("Bind failed");

        close(server_socket);

        return 1;
    }

    if (listen(
            server_socket,
            5) < 0)
    {
        perror("Listen failed");

        close(server_socket);

        return 1;
    }

    printf("\n");
    printf("============================================\n");
    printf("     LINUX RESOURCE MONITORING SERVER\n");
    printf("============================================\n");
    printf("Server started on port %d.\n", PORT);
    printf("Waiting for client connection...\n");

    while (server_running)
    {
        client_length =
            sizeof(client_address);

        client_socket =
            accept(
                server_socket,
                (struct sockaddr *)&client_address,
                &client_length
            );

        if (client_socket < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("Accept failed");
            break;
        }

        printf(
            "\nClient connected.\n"
        );

        while (1)
        {
            int bytes_received;

            memset(
                buffer,
                0,
                sizeof(buffer)
            );

            bytes_received =
                recv(
                    client_socket,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

            if (bytes_received <= 0)
            {
                printf(
                    "Client disconnected.\n"
                );

                break;
            }

            buffer[bytes_received] = '\0';

            printf(
                "\nCommand received: %s\n",
                buffer
            );

            if (strcmp(buffer, "EXIT") == 0)
            {
                handle_command(
                    client_socket,
                    buffer
                );

                break;
            }

            handle_command(
                client_socket,
                buffer
            );
        }

        close(client_socket);

        printf(
            "Waiting for next client...\n"
        );
    }

    if (monitoring)
    {
        monitoring = 0;

        if (monitoring_thread_created)
        {
            pthread_join(
                monitoring_thread,
                NULL
            );
        }
    }

    close(server_socket);

    printf(
        "\nServer stopped.\n"
    );

    return 0;
}
