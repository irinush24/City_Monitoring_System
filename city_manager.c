#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define BUF_SIZE 300

typedef struct report
{
    int reportID;
    char inspectorName[20];
    float latitude, longitude;
    char category[20];
    int severityLevel;
    time_t timestamp;
    char description[140];
} report_t;

typedef struct district
{
    int districtID;
    char name[30];
} district_t;

int add(int districtID, char const *user, char const *role)
{
    int districts = open("logged_district.txt", O_RDWR | O_CREAT, 0664);
    if (districts < 0)
    {
        perror("Error opening districts file\n");
        return -1;
    }
    int reports = open("reports.dat", O_WRONLY | O_APPEND | O_CREAT, 0664);
    if(reports < 0)
    {
        perror("Error opening report file\n");
        close(districts);
        return -1;
    }

    char buffer[BUF_SIZE];
    sprintf(buffer, "%d\t%s\t%s %s\n", districtID, user, role, "add");

    ssize_t bytesWritten = write(districts, buffer, strlen(buffer));
    if (bytesWritten < strlen(buffer))
    {
        perror("Incomplete write to file");
        close(districts);
        return -3;
    }

    report_t *report = malloc(sizeof(report_t));
    if(report == NULL)
    {
        perror("Error allocating report\n");
        close(reports);
        return -2;
    }
    report->reportID =  rand() % INT_MAX;
    report -> timestamp = time(NULL);
    char *message = "X: ";
    write(STDOUT_FILENO, message, strlen(message));
    ssize_t bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report -> latitude = strtof(buffer, NULL);
    }
    message = "Y: ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report -> longitude = strtof(buffer, NULL);
    }
    message = "Category (road/lightning/flooding/other): ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        if (buffer[bytesRead - 1] == '\n')
            buffer[bytesRead - 1] = '\0';
        else
            buffer[bytesRead] = '\0';
        strcpy(report -> category, buffer);
    }
    message = "Severity level: (1/2/3): ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report -> severityLevel = atoi(buffer);
    }
    message = "Description: ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        if (buffer[bytesRead - 1] == '\n')
            buffer[bytesRead - 1] = '\0';
        else
            buffer[bytesRead] = '\0';
        strcpy(report -> description, buffer);
    }
    strcpy(report -> inspectorName, user);
    bytesWritten = write(reports, report, sizeof(report_t));
    if (bytesWritten < (ssize_t)sizeof(report_t))
    {
        perror("Incomplete write to file\n");
        free(report);
        close(reports);
        return -3;
    }
    free(report);
    close(reports);
    close(districts);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 7)
    {
        char *message = "Too few command line arguments";
        write(STDOUT_FILENO, message, strlen(message));
        exit(-2);
    }
    char role[100], command[20], districtName[50], user[20];
    strcpy(role, argv[2]);
    strcpy(user, argv[4]);
    strcpy(command, argv[5]);
    printf("Command: %s\n", command);
    strcpy(districtName, argv[6]);
    if (strcmp(role, "inspector") == 0)
    {
        if (strcmp(command, "--add") == 0)
        {
            if (add(1, user, role) != 1)
            {
                perror("Error adding report\n");
                exit(-2);
            }
        }
    }
    else if (strcmp(role, "manager") == 0)
    {
        if (strcmp(command, "--add") == 0)
        {
            if (add(1, user, role) != 1)
            {
                perror("Error adding report\n");
                exit(-2);
            }
        }
    }
    else
    {
        perror("Invalid role");
        exit(-1);
    }
    return 0;
}