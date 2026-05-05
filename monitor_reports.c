#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#define PID_FILE ".monitor_pid"

volatile sig_atomic_t keepRunning = 1;

void handleSigint(int signo)
{
    keepRunning = 0;
}

void handleSigusr1(int signo)
{
    char *message = "Update: A new report has been added to the system.\n";
    write(STDOUT_FILENO, message, strlen(message));
}

int main(int argc, char *argv[])
{
    // Create or overwrite the hidden .monitor_pid file
    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("Failed to create .monitor_pid");
        exit(-1);
    }

    // Get the process ID and write it to the file
    pid_t pid = getpid();
    char pidStr[32];
    int len = snprintf(pidStr, sizeof(pidStr), "%d\n", pid);

    if (write(fd, pidStr, len) != len)
    {
        perror("Failed to write to .monitor_pid");
        close(fd);
        exit(-1);
    }
    close(fd);

    char startMessage[100];
    int startLen = snprintf(startMessage, sizeof(startMessage), "Monitor started. PID: %d. Waiting for signals...\n", pid);
    write(STDOUT_FILENO, startMessage, startLen);

    // Set up the signal handlers
    struct sigaction saInt, saUsr1;

    // Set up SIGINT
    saInt.sa_handler = handleSigint;
    sigemptyset(&saInt.sa_mask);
    saInt.sa_flags = 0;
    if (sigaction(SIGINT, &saInt, NULL) < 0)
    {
        perror("Error configuring SIGINT handler");
        unlink(PID_FILE);
        exit(-1);
    }

    // Set up SIGUSR1
    saUsr1.sa_handler = handleSigusr1;
    sigemptyset(&saUsr1.sa_mask);
    saUsr1.sa_flags = SA_RESTART;       // Used to restart interrupted system calls
    if (sigaction(SIGUSR1, &saUsr1, NULL) < 0)
    {
        perror("Error configuring SIGUSR1 handler");
        unlink(PID_FILE);
        exit(-1);
    }

    // Infinite loop that suspends execution until a signal is caught
    while (keepRunning)
    {
        pause();
    }

    if (unlink(PID_FILE) < 0)
    {
        char *cleanupMessage = "Cleaned up .monitor_pid file\n";
        write(STDOUT_FILENO, cleanupMessage, strlen(cleanupMessage));
    }
    else
    {
        perror("Failed to remove .monitor_pid");
    }

    char *exitMessage = "SIGINT caught. Exiting...\n";
    write(STDOUT_FILENO, exitMessage, strlen(exitMessage));

    return EXIT_SUCCESS;
}
