#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>

void startMonitor()
{
    pid_t hubMonPID = fork();
    if(hubMonPID < 0)
    {
        perror("Failed to create monitor process");
        exit(-1);
    }
    if(hubMonPID == 0)
    {
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            perror("Failed to create pipe");
            exit(-1);
        }

        pid_t monitorPID = fork();
        if (monitorPID == 0)
        {
            // Child process 2: Execute the monitor_reports program
            // Redirect STDOUT to the write end of a pipe
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]); 
            
            execl(" ./monitor_reports", "monitor_reports", NULL);
            exit(EXIT_FAILURE);
        }
         else
        {
            // Child process 1: Hub monitor
            close(pipefd[1]); // Close the write end of the pipe, we only need to read

            char buffer[256];
            ssize_t bytesRead;
            while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
            {
                buffer[bytesRead] = '\0'; // Null-terminate the strin
                write(STDOUT_FILENO, "\n[HUB_MON]", 11); // Write the monitor's output to the hub's STDOUT
                write(STDOUT_FILENO, buffer, bytesRead); // Write the monitor's output to the hub's STDOUT

                if(strstr(buffer, "ERROR|") != NULL || strstr (buffer, "EXIT|") != NULL)
                {
                    char *endMessage = "[HUB_MON] Monitor process has ended or failed to start. Stopping listener\n";
                    write(STDOUT_FILENO, endMessage, strlen(endMessage));
                    break;
                }
            }
            close(pipefd[0]); // Close the read end of the pipe
            exit(EXIT_SUCCESS); // Kill the background listener when monitor dies
        }
    }
    else
    {
        char *message = "Background hub_mon is created. Monitor is starting...\n";
        write(STDOUT_FILENO, message, strlen(message));
    }
   
}

void calculateScores(char args[][50], int argCount)
{
    for(int i = 0; i < argCount; i++)
    {
        char *district  = args[i];
        
        char header[100];
        int headerLen = snprintf(header, sizeof(header), "Scores for district: %s\n", district);
        write(STDOUT_FILENO, header, headerLen);

        int pfd[2];
        if (pipe(pfd) < 0)
        {
            continue;
        }

        pid_t pid = fork();
        if(pid == 0)
        {
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[0]);
            close(pfd[1]);

            execl("./scorer", "scorer", district, NULL);
            exit(EXIT_FAILURE);
        }

        close(pfd[1]);
        char buffer[256];
        int bytesRead;
        while((bytesRead = read(pfd[0], buffer, sizeof(buffer)))>0)
            write(STDOUT_FILENO, buffer, bytesRead);
        close(pfd[0]);

        waitpid(pid, NULL, 0);
    }

}

int main()
{
    char input[256];
    while(1)
    {
        write(STDOUT_FILENO, "city_hub: ", 10);

        int bytes = read(STDIN_FILENO, input, sizeof(input) - 1);
        if (bytes <= 0)
            break;
        
        input[bytes - 1] = '\0'; // Remove newline character

        if(strlen(input) == 0)
            continue;

        char args[10][50];
        int argCount = 0;
        char *token = strtok(input, " ");

        while(token != NULL && argCount < 10)
        {
            strcpy(args[argCount++], token);
            token = strtok(NULL, " ");
        }

        if(strcmp(args[0], "start-monitor") == 0)
        {
            startMonitor();
        }
        else if(strcmp(args[0], "calculate-scores") == 0)
        {
            if(argCount > 1)    
                calculateScores(&args[1], argCount - 1);
            else
            {
                char *message = "No districts provided for score calculation\n";
                write(STDOUT_FILENO, message, strlen(message));
            }
        }
        else if(strcmp(args[0], "exit") == 0)
        {
            char *message = "Exiting city hub...\n";
            write(STDOUT_FILENO, message, strlen(message));
            break;
        }
        else
        {
            char *message = "Unknown command. Available commands: start-monitor, calculate-scores <district1> <district2> ...\n";
            write(STDOUT_FILENO, message, strlen(message));
        }
    }
    return 0;
}