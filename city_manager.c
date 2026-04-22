#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>

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


void modeToString(mode_t mode, char *str)
{
    strcpy(str, "---------");

    // User permissions
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';

    // Group permissions
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';

    // Others permissions
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

void add(char const *districtName, char const *user, char const *role)
{
    struct stat st;

    // Create district directory if it doesn't exist
    if (stat(districtName, &st) == -1)
    {
        if (mkdir(districtName, 0750) < 0)
        {
            perror("Error creating district directory");
            return;
        }
    }

    // Open/create reports file and check permissions
    char path[150];
    snprintf(path, sizeof(path), "%s/reports.dat", districtName);

    if (stat(path, &st) == 0)
    {
        int canWrite = 0;
        if (strcmp(role, "manager") == 0 && (st.st_mode & S_IWUSR) == 0)
            canWrite = 1;
        if (strcmp(role, "inspector") == 0 && (st.st_mode & S_IRUSR) == 0)
            canWrite = 1;

        if (!canWrite)
        {
            char *err = "Error: Your role does not have writing privileges\n";
            write(STDOUT_FILENO, err, strlen(err));
            exit(-1);
        }
    }

    int reports = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (reports < 0)
    {
        perror("Error opening report file");
        return;
    }
    chmod(path, 0644);      // Ensure explicit permissions after creation

    // Create active reports symlink
    char symPath[150];
    snprintf(symPath, sizeof(symPath), "active_reports-%s", districtName);
    symlink(path, symPath);

    // Fill report data
    char buffer[BUF_SIZE];
    report_t report;

    report.reportID = rand() % INT_MAX;
    report.timestamp = time(NULL);

    char *message = "X: ";
    write(STDOUT_FILENO, message, strlen(message));
    ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report.latitude = strtof(buffer, NULL);
    }

    message = "Y: ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE-1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report.longitude = strtof(buffer, NULL);
    }

    message = "Category (road/lighting/flooding/other): ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        if (buffer[bytesRead - 1] == '\n')
            buffer[bytesRead - 1] = '\0';
        else
            buffer[bytesRead] = '\0';

        strncpy(report.category, buffer, 19);
        report.category[10] = '\0';
    }

    message = "Severity level (1/2/3): ";
    write(STDOUT_FILENO, message, strlen(message));
    bytesRead = read(STDIN_FILENO, buffer, BUF_SIZE - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        report.severityLevel = atoi(buffer);
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
        strncpy(report.description, buffer, 139);
        report.description[139] = '\0';
    }

    strncpy(report.inspectorName, user, 19);
    report.inspectorName[19] = '\0';

    int bytesWritten = write(reports, &report, sizeof(report_t));
    if (bytesWritten < sizeof(report_t))
        perror("Incomplete write to file\n");
    close(reports);

    char logPath[150];
    snprintf(logPath, sizeof(logPath), "%s/logged_district.txt", districtName);

    if (strcmp(role, "inspector") == 0)
    {
        message = "Inspector restriction: cannot write to logged_district.txt\n";
        write(STDERR_FILENO, message, strlen(message));
    }
    else
    {
        if (stat(logPath, &st) == 0 && !(st.st_mode & S_IWUSR))
        {
            message = "Manager lacks write access to logged_district.txt \n";
            write(STDERR_FILENO, message, strlen(message));
        }
        else
        {
            int logFd = open(logPath, O_WRONLY | O_APPEND | O_CREAT, 0644);
            chmod(logPath, 0644);
            if (logFd >= 0)
            {
                char logBuf[256];
                int len = snprintf(logBuf, sizeof(logBuf), "%ld\n%s\nmanager add\n", time(NULL), user);
                write(logFd, logBuf, len);
                close(logFd);
            }
        }
    }
}


void list(char const *districtName, char const *role)
{
    char path[150];
    snprintf(path, sizeof(path), "%s/reports.dat", districtName);

    char *message;

    struct stat st;
    if (stat(path, &st) == -1)
    {
        message = "District or report file does not exist\n";
        write(STDERR_FILENO, message, strlen(message));
        return;
    }

    int canRead = 0;
    if (strcmp(role, "manager") == 0 && (st.st_mode & S_IRUSR)) canRead = 1;
    if (strcmp(role, "inspector") == 0 && (st.st_mode & S_IRGRP)) canRead = 1;

    if (!canRead)
    {
        message = "Declared role does not have reading privileges for reports.dat\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    char perms[10];
    modeToString(st.st_mode, perms);

    char metaData[256];
    int len = snprintf(metaData, sizeof(metaData),
                       "File Info: Permissions: %s | Size: %ld bytes | Last Modified: %ld\n\n", perms, st.st_size,
                       st.st_mtime);
    write(STDOUT_FILENO, metaData, len);

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return;

    report_t report;
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t))
    {
        char output[512];
        len = snprintf(output, sizeof(output),
                       "ID: %d | Inspector: %s | Coordinates: %.2f %.2f | Category: %s | Severity: %d\nDescription: %s\n---\n",
                       report.reportID, report.inspectorName, report.latitude, report.longitude, report.category,
                       report.severityLevel, report.description);
        write(STDOUT_FILENO, output, len);
    }
    close(fd);
}


void view(char const *districtName, int const soughtReportID, char const *role)
{
    char path[150];
    snprintf(path, sizeof(path), "%s/reports.dat", districtName);

    char *message;

    struct stat st;
    if (stat(path, &st) == -1)
    {
        message = "District or report file does not exist\n";
        write(STDERR_FILENO, message, strlen(message));
        return;
    }

    int canRead = 0;
    if (strcmp(role, "manager") == 0 && (st.st_mode & S_IRUSR)) canRead = 1;
    if (strcmp(role, "inspector") == 0 && (st.st_mode & S_IRGRP)) canRead = 1;

    if (!canRead)
    {
        message = "Declared role does not have reading privileges for reports.dat\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return;

    report_t report;
    bool found = false;
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t))
    {
        if (report.reportID == soughtReportID)
        {
            found = true;
            char output[512];
            int len = snprintf(output, sizeof(output),
                           "ID: %d | Inspector: %s | Coordinates: %.2f %.2f | Category: %s | Severity: %d\nDescription: %s\n---\n",
                           report.reportID, report.inspectorName, report.latitude, report.longitude, report.category,
                           report.severityLevel, report.description);
            write(STDOUT_FILENO, output, len);
        }
    }
    if (!found)
    {
        message = "The report that you are looking for doesn't exist\n";
        write(STDERR_FILENO, message, strlen(message));
    }
    close(fd);
}

void removeReport(char const *districtName, int const IDtoRemove, char const *role)
{
    char path[150];
    snprintf(path, sizeof(path), "%s/reports.dat", districtName);

    char *message;

    struct stat st;
    if (stat(path, &st) == -1)
    {
        message = "District or report file does not exist\n";
        write(STDERR_FILENO, message, strlen(message));
        return;
    }

    int canRead = 0;
    if (strcmp(role, "manager") != 0)
    {
        message = "Error: Only the manager role can remove reports\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    if (stat (path, &st) == -1)
    {
        message = "District or report file does not exist\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    int fd = open(path, O_RDWR);
    if (fd < 0)
        return;

    report_t report;
    bool found = false;
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t))
    {
        if (report.reportID == IDtoRemove)
        {
            found = true;

            // Compute the starting offset
            off_t readOffset = lseek(fd, 0, SEEK_CUR);  // where the NEXT record begins
            off_t writeOffset = readOffset - sizeof(report_t);        // where the DELETED record began

            report_t temp;

            // Shifting logic
            lseek(fd, readOffset, SEEK_SET);
            while (read(fd, &temp, sizeof(report_t)) == sizeof(report_t))
            {
                // Move to overwrite the older record
                lseek(fd, writeOffset, SEEK_SET);
                write(fd, &temp, sizeof(report_t));

                // Update offsets for the next iteration
                writeOffset += sizeof(report_t);
                readOffset += sizeof(report_t);

                // Advance to the next block
                lseek(fd, readOffset, SEEK_SET);
            }

            // Truncate the file at the final writeOffset
            if (ftruncate(fd, writeOffset) == -1)
                perror("Error truncating file");
            else
            {
                message = "Successfully removed report\n";
                write(STDOUT_FILENO, message, strlen(message));
            }
            break;  // Exiting the initial search loop
        }
    }
    if (!found)
    {
        message = "The report that you are looking for doesn't exist\n";
        write(STDERR_FILENO, message, strlen(message));
    }

    close(fd);
}

void updateThreshold(char const *districtName, int const newLimit, char const *role)
{
    char path[150];
    snprintf(path, sizeof(path), "%s/district.cfg", districtName);

    char *message;

    if (strcmp(role, "manager") != 0)
    {
        message = "Error: Only the manager role can update severity threshold\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    struct stat st;
    if (stat(path, &st) == -1)
    {
        message = "District or configuration file does not exist\n";
        write(STDERR_FILENO, message, strlen(message));
        return;
    }

    if ((st.st_mode & 0777) != 0640)
    {
        message = "Diagnostic: district.cfg permissions have been altered\n";
        write(STDERR_FILENO, message, strlen(message));
        exit(-1);
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0)
    {
        perror("Error opening configuration file");
        return;
    }

    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", newLimit);
    if (write(fd, buffer, len) != len)
    {
        perror("Incomplete write to configuration file\n");
        close(fd);
        exit(-1);
    }

    close(fd);

    message = "Threshold updated successfully\n";
    write(STDOUT_FILENO, message, strlen(message));
}

int main(int argc, char *argv[])
{
    if (argc < 7)
    {
        char *message = "Too few command line arguments\n";
        write(STDOUT_FILENO, message, strlen(message));
        exit(-2);
    }
    char role[100], command[20], districtName[50], user[20];
    strcpy(role, argv[2]);
    strcpy(user, argv[4]);
    strcpy(command, argv[5]);
    strcpy(districtName, argv[6]);

    if (strcmp(role, "inspector") != 0 && strcmp(role, "manager") != 0)
    {
        char *error = "Invalid role\n";
        write(STDERR_FILENO, error, strlen(error));
        exit(-1);
    }

    if (strcmp(command, "--add") == 0)
    {
        add(districtName, user, role);
    }
    else if (strcmp(command, "--list") == 0)
    {
        list(districtName, role);
    }
    else if (strcmp(command, "--view") == 0)
    {
        view(districtName, atoi(argv[7]), role);
    }
    else
    {
        char *error = "Invalid command\n";
        write(STDERR_FILENO, error, strlen(error));
        exit(-1);
    }
    return 0;
}