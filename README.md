# Linux System Information and Resource Monitoring Tool

## Project Description

The Linux System Information and Resource Monitoring Tool is a C-based Linux system programming project developed on Ubuntu/Linux.

The system monitors important operating system resources and running processes through a command-line interface.

It provides system information such as:

- CPU information
- Overall CPU usage
- Memory usage
- Disk usage
- System uptime
- Running processes
- Process ID
- Process name
- Process state
- Process memory usage

The project reads system and process information from the Linux `/proc` filesystem.

## Features

1. System Information
2. Process Information
3. CPU Threshold Configuration
4. Memory Threshold Configuration
5. Resource Checking
6. Process Control
7. Continuous Monitoring
8. Monitoring Stop
9. Log Viewing
10. Client Exit

## Process Control

The system uses Linux signals for process control:

- SIGSTOP - Stop a process
- SIGCONT - Continue a stopped process
- SIGTERM - Terminate a process

## Monitoring

The system can continuously monitor process activity using a POSIX thread.

It tracks:

- Process creation
- Process termination
- CPU resource warnings
- Memory resource warnings

## Linux `/proc` Filesystem

The project uses the `/proc` filesystem to obtain system and process information.

Examples:

- `/proc/cpuinfo`
- `/proc/meminfo`
- `/proc/stat`
- `/proc/[PID]/stat`
- `/proc/[PID]/status`

## Network Communication

The project uses TCP socket communication between:

- Client
- Monitoring Server

The server uses port `8080`.

The client sends commands to the monitoring server, and the server performs the requested operation.

## Technologies

- C
- Ubuntu/Linux
- GCC
- POSIX APIs
- Linux `/proc` filesystem
- TCP sockets
- POSIX threads
- Linux signals
- File handling

## Compilation

Open the terminal and enter:

    cd ~/LinuxMonitor

Then compile:

    make

## Run the Server

Open Terminal 1:

    cd ~/LinuxMonitor
    ./server

## Run the Client

Open Terminal 2:

    cd ~/LinuxMonitor
    ./client

## Main Menu

The client provides the following options:

1. System Information
2. Process Information
3. Set CPU Threshold
4. Set Memory Threshold
5. Check Resources
6. Process Control
7. Start Continuous Monitoring
8. Stop Continuous Monitoring
9. View Logs
10. Exit

## Log File

Monitoring events are stored in:

    logs/monitor.log

## Cleaning the Project

To remove compiled executable files:

    make clean

## Platform

Ubuntu/Linux

## Language

C
