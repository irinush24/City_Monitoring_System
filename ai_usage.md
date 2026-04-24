# AI usage within the City Management project
The agent that I used is Google Gemini. I chose to use this specific agent because I have a Gemini Pro student plan.
## The parseCondition function
### Prompt 
I have to implement a parse_condition function which splits a field:operator:value string into its three parts. I receive this condition via command line and I have this as the function signature:
int parse_condition(const char *input, char *field, char *op, char *value);
The reports structure is the following:
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

### AI Reply
It gave me two possible implementations, I chose the one that used the function sscanf, because it is more concise:

#include <stdio.h>

int parse_condition(const char *input, char *field, char *op, char *value)
{
// %[^:] means "read a string of any characters until you hit a colon"
// We expect exactly 3 parsed elements.
int items_parsed = sscanf(input, "%[^:]:%[^:]:%s", field, op, value);

    if (items_parsed == 3) {
        return 1; // Success
    }
    return 0; // Failure
}

### The changes I made to the AI generated reply
I already had the <stdio.h> library included at the top of my .c file so I removed the line
I chose to remove the items_parsed variable and directly check the result of the sscanf function inside the if statement.
I also removed the curly braces of the return 1 statement since they are not required.


## The matchCondition function
### Prompt
Knowing the structure of the report_t custom data type that I presented earlier and the parseCondition function, implement 
the function with the following signature: int match_condition(report_t *r, const char *field, const char *op, const char
*value); which returns 1 if the record satisfies the condition and 0 otherwise.

### AI Reply

#include <string.h>
#include <stdlib.h>

int match_condition(report_t *r, const char *field, const char *op, const char *value)
{
// 1. Handle Integer Fields (severity)
if (strcmp(field, "severity") == 0)
{
int num_val = atoi(value); // Convert string value to integer

        if (strcmp(op, "==") == 0) return r->severityLevel == num_val;
        if (strcmp(op, "!=") == 0) return r->severityLevel != num_val;
        if (strcmp(op, "<")  == 0) return r->severityLevel < num_val;
        if (strcmp(op, "<=") == 0) return r->severityLevel <= num_val;
        if (strcmp(op, ">")  == 0) return r->severityLevel > num_val;
        if (strcmp(op, ">=") == 0) return r->severityLevel >= num_val;
    }
    // 2. Handle Time/Long Integer Fields (timestamp)
    else if (strcmp(field, "timestamp") == 0) 
    {
        time_t time_val = (time_t)atol(value); // time_t is safely cast from long
        
        if (strcmp(op, "==") == 0) return r->timestamp == time_val;
        if (strcmp(op, "!=") == 0) return r->timestamp != time_val;
        if (strcmp(op, "<")  == 0) return r->timestamp < time_val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= time_val;
        if (strcmp(op, ">")  == 0) return r->timestamp > time_val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= time_val;
    }
    // 3. Handle String Fields (category, inspector)
    else if (strcmp(field, "category") == 0 || strcmp(field, "inspector") == 0) 
    {
        // Point to the correct string in the struct based on the field
        const char *record_str;
        if (strcmp(field, "category") == 0) {
            record_str = r->category;
        } else {
            record_str = r->inspectorName;
        }

        // strcmp returns 0 if equal, < 0 if strictly less, > 0 if strictly greater
        int cmp = strcmp(record_str, value);

        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        if (strcmp(op, "<")  == 0) return cmp < 0;
        if (strcmp(op, "<=") == 0) return cmp <= 0;
        if (strcmp(op, ">")  == 0) return cmp > 0;
        if (strcmp(op, ">=") == 0) return cmp >= 0;
    }

    // Return 0 (false) if the field or operator is unrecognized
    return 0;
}

### The changes I made to the AI generated reply
I already had the <string.h> and <stdlib.h> libraries included at the top of my .c file so I removed them from where they
are currently written. Besides that, I kept everything that the AI generated.