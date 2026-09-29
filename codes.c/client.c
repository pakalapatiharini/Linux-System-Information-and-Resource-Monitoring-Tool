#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 65536

void receive_response(int sock)
{
    char buffer[BUFFER_SIZE];

    memset(
        buffer,
        0,
        sizeof(buffer)
    );

    int bytes =
        recv(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    if (bytes > 0)
    {
        buffer[bytes] = '\0';

        printf(
            "\n%s\n",
            buffer
        );
    }
    else
    {
        printf(
            "\nNo response received from server.\n"
        );
    }
}

void send_command(
    int sock,
    const char *command
)
{
    send(
        sock,
        command,
        strlen(command),
        0
    );

    receive_response(sock);
}

void process_control_menu(int sock)
{
    int pid;
    int choice;

    char command[100];

    printf("\n");
    printf("====================================\n");
    printf("          PROCESS CONTROL\n");
    printf("====================================\n");

    printf("Enter PID: ");
    scanf("%d", &pid);

    printf("\n");
    printf("1. Stop Process\n");
    printf("2. Continue Process\n");
    printf("3. Terminate Process\n");
    printf("4. Back\n");

    printf("Enter choice: ");
    scanf("%d", &choice);

    if (choice == 1)
    {
        snprintf(
            command,
            sizeof(command),
            "STOP %d",
            pid
        );

        send_command(
            sock,
            command
        );
    }
    else if (choice == 2)
    {
        snprintf(
            command,
            sizeof(command),
            "CONT %d",
            pid
        );

        send_command(
            sock,
            command
        );
    }
    else if (choice == 3)
    {
        snprintf(
            command,
            sizeof(command),
            "TERM %d",
            pid
        );

        send_command(
            sock,
            command
        );
    }
    else if (choice == 4)
    {
        printf(
            "Returning to main menu.\n"
        );
    }
    else
    {
        printf(
            "Invalid choice.\n"
        );
    }
}

int main(void)
{
    int sock;

    struct sockaddr_in server_addr;

    sock =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (sock < 0)
    {
        perror(
            "Socket creation failed"
        );

        return 1;
    }

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_port =
        htons(PORT);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &server_addr.sin_addr) <= 0)
    {
        perror(
            "Invalid server address"
        );

        close(sock);

        return 1;
    }

    if (connect(
            sock,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0)
    {
        perror(
            "Connection failed"
        );

        printf(
            "Make sure the server is running first.\n"
        );

        close(sock);

        return 1;
    }

    printf("\n");
    printf("============================================\n");
    printf("      LINUX RESOURCE MONITORING CLIENT\n");
    printf("============================================\n");
    printf(
        "Connected to monitoring server.\n"
    );

    while (1)
    {
        int choice;

        printf("\n");
        printf("============================================\n");
        printf("              MAIN MENU\n");
        printf("============================================\n");

        printf(
            "1. System Information\n"
        );

        printf(
            "2. Process Information\n"
        );

        printf(
            "3. Set CPU Threshold\n"
        );

        printf(
            "4. Set Memory Threshold\n"
        );

        printf(
            "5. Check Resources\n"
        );

        printf(
            "6. Process Control\n"
        );

        printf(
            "7. Start Continuous Monitoring\n"
        );

        printf(
            "8. Stop Continuous Monitoring\n"
        );

        printf(
            "9. View Logs\n"
        );

        printf(
            "10. Exit\n"
        );

        printf(
            "============================================\n"
        );

        printf(
            "Enter choice: "
        );

        scanf(
            "%d",
            &choice
        );

        if (choice == 1)
        {
            send_command(
                sock,
                "SYSTEM"
            );
        }
        else if (choice == 2)
        {
            send_command(
                sock,
                "PROCESS"
            );
        }
        else if (choice == 3)
        {
            double value;

            char command[100];

            printf(
                "Enter CPU threshold (1-100): "
            );

            scanf(
                "%lf",
                &value
            );

            snprintf(
                command,
                sizeof(command),
                "SETCPU %.2f",
                value
            );

            send_command(
                sock,
                command
            );
        }
        else if (choice == 4)
        {
            double value;

            char command[100];

            printf(
                "Enter memory threshold in MB: "
            );

            scanf(
                "%lf",
                &value
            );

            snprintf(
                command,
                sizeof(command),
                "SETMEM %.2f",
                value
            );

            send_command(
                sock,
                command
            );
        }
        else if (choice == 5)
        {
            send_command(
                sock,
                "CHECK"
            );
        }
        else if (choice == 6)
        {
            process_control_menu(
                sock
            );
        }
        else if (choice == 7)
        {
            send_command(
                sock,
                "START"
            );
        }
        else if (choice == 8)
        {
            send_command(
                sock,
                "MONITORSTOP"
            );
        }
        else if (choice == 9)
        {
            send_command(
                sock,
                "LOGS"
            );
        }
        else if (choice == 10)
        {
            send_command(
                sock,
                "EXIT"
            );

            break;
        }
        else
        {
            printf(
                "\nInvalid choice. Please enter 1-10.\n"
            );
        }
    }

    close(sock);

    printf(
        "\nClient stopped.\n"
    );

    return 0;
}
