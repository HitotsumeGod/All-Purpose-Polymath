#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "app_util.h"

char *perftask(char *log, char **logs, int *nptr) {

        FILE* checkf;
        clock_t start, end;
        float timespent;
        if ((log = malloc(sizeof(char) * 90)) == NULL) {
                perror("malloc err");
                exit(1);
        }
        start = clock();
        system("ping -c 1 www.google.com > ping_info");
        if ((checkf = fopen("ping_info", "r")) == NULL)
               log = "Failed execution of ping task on target 'www.google.com'.";
        end = clock();
        timespent = (float) 1000 * ((end - start) / CLOCKS_PER_SEC);    //execution time in milliseconds
        if (snprintf(log, 90, "Completed execution of ping task on target 'www.google.com' in %f milliseconds.", timespent) == -1) {
                perror("string formatting err");
                exit(1);
        }
        *nptr = logalloc(logs, *nptr);
        return log;

}

int logalloc(char **logs, int n) {

        if ((*(logs + n) = malloc(sizeof(char) * MAXLOG_LEN)) == NULL) {
                perror("malloc err");
                exit(1);
        }
        return ++n;

}
