#include <stdlib.h>
#include <time.h>
#include <sys/fcntl.h>
#include <string.h>

typedef struct report {
    int reportID;
    char inspectorName[100];
    float latitude, longitude;
    char category[100];
    int severityLevel;
    time_t timestamp;
    char description[100];
} report_t;

typedef struct district {
    int districtID;
    char name[100];
    report_t *reports;
} district_t;

int main(int argc, char *argv[]) {
    char role[100], command[20], districtName[100];
    strcpy(role, argv[2]);
    strcpy(command, argv[3]);
    strcpy(districtName, argv[4]);
    if (strcmp(command, "--remove_report") == 0) {
        int reportNumber = atoi(argv[5]);
    }
    return 0;
}